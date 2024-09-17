#pragma once

#include <sstream>

#include <caen++/caen.hpp>

template <typename... T> std::string concat(T... x) {
  std::stringstream ss;
  (ss << ... << x);
  return ss.str();
};

uint16_t str_to_uint16(const char*, int base = 10);
uint32_t str_to_uint32(const char*, int base = 10);

void list_caen_links();
CAENComm_ConnectionType str_to_link(const char*);
