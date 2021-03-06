// Helper functions for generating random test data
//
// Original author: Colm Ryan
// Copyright 2016, Raytheon BBN Technologies

#include <random>
#include <vector>

static std::default_random_engine generator(std::random_device{}());

namespace RandomHelpers {

uint32_t random_address(uint32_t, uint32_t, uint32_t);

std::vector<uint32_t> random_data(size_t);

std::vector<int16_t> random_waveform(size_t);
}
