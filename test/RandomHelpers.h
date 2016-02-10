// Helper functions for generating random test data
//
// Original author: Colm Ryan
// Copyright 2016, Raytheon BBN Technologies

static std::default_random_engine generator(std::random_device{}());

namespace RandomHelpers {

  uint32_t random_address(uint32_t, uint32_t, uint32_t);

  vector<uint32_t> random_data(size_t length);

}
