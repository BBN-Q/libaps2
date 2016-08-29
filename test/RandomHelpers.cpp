// Helper functions for generating random test data
//
// Original author: Colm Ryan
// Copyright 2016, Raytheon BBN Technologies

#include <random>
#include <vector>
using std::vector;

#include "RandomHelpers.h"

uint32_t RandomHelpers::random_address(uint32_t mem_low, uint32_t mem_high,
                                       uint32_t alignment) {
  // Create a random starting address aligned appropriately
  // Choose a random starting point
  std::uniform_int_distribution<uint32_t> addr_distribution(mem_low, mem_high);
  uint32_t start_addr = addr_distribution(generator);
  start_addr &= ~(alignment); // align address
  return start_addr;
}

vector<uint32_t> RandomHelpers::random_data(size_t length) {
  // Create random data to write/read
  std::uniform_int_distribution<uint32_t> word_distribution;
  vector<uint32_t> data(length);
  for (auto &val : data) {
    val = word_distribution(generator);
  }
  return data;
}

vector<int16_t> RandomHelpers::random_waveform(size_t length){
  //Create random waveform data
  std::uniform_int_distribution<int16_t> wf_distribution(-8192, 8191);
  vector<int16_t> data(length);
  for (auto &val : data){
    val = wf_distribution(generator);
  }
  return data;
}
