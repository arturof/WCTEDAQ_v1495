#include <fstream>
#include <iostream>
#include <vector>
#include <sstream>
#include <stdexcept>

#include <cstring>

#include <getopt.h>

#include <json/json.h>

#include <caen++/v1495.hpp>

#include "common.hpp"

void usage(const char* argv0) {
  std::cout
    << "This program writes values to registers of a CAEN V1495 VME module\n"
       "Usage: " << argv0 << " [options] [filename]\n"
       "Allowed options:\n"
       "  --help or -h:              print this message.\n"
       "  --link or -l <string>:     CAEN connection link type. Pass `list' to see supported links.\n"
       "  --arg or -a <number>:      CAEN connection argument. USB device number for USB links, A4818 PID (see on the back) for A4818 links, IP address for ethernet links.\n"
       "  --conet or -c <number>:    CAEN Conet connection daisy chain number\n"
       "  --vme or -v <hexadecimal>: 16 most significant bits of the VME address (the value set by the rotary switches on the board).\n"
       "Filename is a file with a single JSON object mapping register address to values to be stored in the register. Both the address and the value must be strings containing a hexadecimal number. If the filename is not provided or is `-', the object is read from standard input.\n"
   ;
};

int main(int argc, char** argv) {
  try {
    caen::Device::Connection connection {
      .link  = CAENComm_USB,
      .arg   = 0,
      .conet = 0,
      .vme   = 0
    };

    const char* arg = nullptr;

    while (true) {
      static option options[] = {
        { "arg",     required_argument, nullptr, 'a' },
        { "conet",   required_argument, nullptr, 'c' },
        { "help",    no_argument,       nullptr, 'h' },
        { "link",    required_argument, nullptr, 'l' },
        { "vme",     required_argument, nullptr, 'v' }
      };

      int c = getopt_long(argc, argv, "a:c:hl:v:", options, nullptr);
      if (c == -1) break;

      switch (c) {
        case 'a':
          arg = optarg;
          break;
        case 'c':
          connection.conet = str_to_uint32(optarg);
          break;
        case 'h':
          usage(argv[0]);
          return 0;
        case 'l':
          if (strcmp(optarg, "list") == 0) {
            list_caen_links();
            return 0;
          };
          connection.link = str_to_link(optarg);
          break;
        case 'v':
          connection.vme = str_to_uint32(optarg, 16) << 16;
          break;
        case '?':
          return 1;
      };
    };

    if (arg)
      if (connection.is_ethernet())
        connection.ip = arg;
      else
        connection.arg = str_to_uint32(arg);

    std::istream* input;
    if (optind == argc || strcmp(argv[optind], "-") == 0)
      input = &std::cin;
    else
      input = new std::ifstream(argv[optind]);

    Json::Value json;
    *input >> json;

    if (input != &std::cin) delete input;

    std::vector<std::pair<uint16_t, uint32_t>> values(json.size());
    auto i = values.begin();
    for (auto j = json.begin(); j != json.end(); ++j) {
      i->first  = str_to_uint16(j.key().asCString(), 16);
      i->second = str_to_uint32(j->asCString(), 16);
    };

    caen::V1495 v1495(connection);

    for (auto [ reg, value ] : values)
      v1495.write32(reg, value);

    return 0;

  } catch (std::exception& e) {
    std::cerr << argv[0] << ": " << e.what() << std::endl;
    return 1;
  };
};
