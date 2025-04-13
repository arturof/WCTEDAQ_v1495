#include <iostream>

#include <cstdio>
#include <cstring>
#include <iomanip>
#include <vector>
#include <string>

#include <getopt.h>

#include <caen++/v1495.hpp>

#include "common.hpp"

void usage(const char* argv0) {
  std::cout
    << "This program reads counter registers from a V1495\n"
       "Usage: " << argv0 << " [options]\n"
       "Allowed options:\n"
       "  --help or -h:              print this message.\n"
       "  --link or -l <string>:     CAEN connection link type. Pass `list' to see supported links.\n"
       "  --arg or -a <number>:      CAEN connection argument. USB device number for USB links, A4818 PID (see on the back) for A4818 links, IP address for ethernet links.\n"
       "  --conet or -c <number>:    CAEN Conet connection daisy chain number\n"
       "  --vme or -v <hexadecimal>: 16 most significant bits of the VME address (the value set by the rotary switches on the board).\n"
       "  --reset or -r:             reset the counters after reading\n"
       "The output is two columns: the name and the value of a counter\n"
  ;
};

struct Counter {
  const char* name;
  const char* description;
  uint16_t address;
};

#include "counters.hpp"

int main(int argc, char** argv) {
  try {
    caen::Device::Connection connection {
      .link  = CAENComm_USB,
      .arg   = 0,
      .conet = 0,
      .vme   = 0
    };

    const char* arg   = nullptr;
    bool        reset = false;

    while (true) {
      static option options[] = {
        { "arg",     required_argument, nullptr, 'a' },
        { "conet",   required_argument, nullptr, 'c' },
        { "help",    no_argument,       nullptr, 'h' },
        { "link",    required_argument, nullptr, 'l' },
        { "reset",   no_argument,       nullptr, 'r' },
        { "vme",     required_argument, nullptr, 'v' }
      };

      int c = getopt_long(argc, argv, "a:c:hl:rv:", options, nullptr);
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
        case 'r':
          reset = true;
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

    caen::V1495 v1495(connection);

    // read counters
    std::vector<std::string> counters_name;
    std::vector<uint32_t> counters_vals;
    for (int i = 1; i < sizeof(counters) / sizeof(*counters); i++) {
      counters_name.push_back(counters[i].name);
      uint32_t val = v1495.read32(counters[i].address);
      counters_vals.push_back(val);
    }

    if (reset) v1495.write32(0x3002, 1);

    // print counters
    for (int i = 0; i < counters_name.size(); i++) {
      std::cout
        << std::left
        << std::setw(4)
        << i
        << std::setw(7)
        << counters_name[i]
        << ": "
        << std::setw(12)
        << counters_vals[i];
      if ((i+1)%4==0) std::cout << std::endl;
      if ((i+1)%32==0) std::cout << std::endl;
    }
    std::cout << std::endl;

    return 0;

  } catch (std::exception& e) {
    std::cerr << argv[0] << ": " << e.what() << std::endl;
    return 1;
  };
};
