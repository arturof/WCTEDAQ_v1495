#include <iostream>
#include <limits>
#include <stdexcept>

#include <cstring>

#include "common.hpp"

static unsigned long str_to_ulong(const char* string, int base = 10) {
  char* end;
  unsigned long result = std::strtoul(string, &end, base);
  if (*end)
    throw std::runtime_error(concat("Invalid unsigned integer: ", string));
  return result;
};

uint16_t str_to_uint16(const char* string, int base) {
  unsigned long result = str_to_ulong(string, base);
  if (result > std::numeric_limits<uint16_t>().max())
    throw std::runtime_error(concat("uint16_t overflow: ", string));
  return result;
};

uint32_t str_to_uint32(const char* string, int base) {
  unsigned long result = str_to_ulong(string, base);
  if (result > std::numeric_limits<uint32_t>().max())
    throw std::runtime_error(concat("uint32_t overflow: ", string));
  return result;
};

static const struct {
  const char* name;
  CAENComm_ConnectionType type;
} caen_links[] = {
  { "usb",       CAENComm_USB         },
  { "optical",   CAENComm_OpticalLink },
  { "a4818",     CAENComm_USB_A4818   },
  { "ethernet",  CAENComm_ETH_V4718   },
  { "usb-v4718", CAENComm_USB_V4718   }
};

void list_caen_links() {
  for (int i = 0; i < sizeof(caen_links) / sizeof(*caen_links); ++i)
    std::cout << caen_links[i].name << '\n';
};

CAENComm_ConnectionType str_to_link(const char* string) {
  for (int i = 0; i < sizeof(caen_links) / sizeof(*caen_links); ++i)
    if (strcmp(string, caen_links[i].name) == 0)
      return caen_links[i].type;
  throw std::runtime_error(concat("Unknown connection link type: ", string));
};

