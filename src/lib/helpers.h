#pragma once

#include <iomanip>

#include <plog/Log.h>

// N-wide hex output with 0x
template <unsigned int N> std::ostream &hexn(std::ostream &out) {
  return out << "0x" << std::hex << std::setw(N) << std::setfill('0');
}

inline int mymod(int a, int b) {
  int c = a % b;
  if (c < 0)
    c += b;
  return c;
}


