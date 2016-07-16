#include "catch.hpp"

#include <string>
using std::string;
#include <random>
#include <thread>

#include "constants.h"
#include "libaps2.h"

#include "APS2Connector.h"
#include "RandomHelpers.h"

extern string ip_addr; // ip address from run_tests

TEST_CASE("cache initial state", "[cache]") {

  set_logging_level(logDEBUG3);
  APS2Connector connection(ip_addr);

  SECTION("initial state") {
    // disable the cache
    uint32_t cache_control = 0;
    write_memory(ip_addr.c_str(), CACHE_CONTROL_ADDR, &cache_control, 1);

    // 128k sample random waveform data
    // 2 samples per 32 bit word
    auto test_wf_a = RandomHelpers::random_data(128 * (1 << 10) / 2);
    auto test_wf_b = RandomHelpers::random_data(128 * (1 << 10) / 2);
    APS2_STATUS status;
    status = write_memory(ip_addr.c_str(), WFA_OFFSET, test_wf_a.data(),
                          test_wf_a.size());
    REQUIRE(status == APS2_OK);
    status = write_memory(ip_addr.c_str(), WFB_OFFSET, test_wf_b.data(),
                          test_wf_b.size());
    REQUIRE(status == APS2_OK);

    // each sequence cache line is 128 instructions
    // 2 words per instruction; 4 cache lines
    auto test_seq = RandomHelpers::random_data(4 * 128 * 2);
    status = write_memory(ip_addr.c_str(), SEQ_OFFSET, test_seq.data(),
                          test_seq.size());
    REQUIRE(status == APS2_OK);

    // enable the cache
    cache_control = 0x1;
    write_memory(ip_addr.c_str(), CACHE_CONTROL_ADDR, &cache_control, 1);

    // wait for cache to update
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    // Check the waveform cache
    // For legacy hardware breakup into 1kB chunks
    vector<uint32_t> check_vec(test_wf_a.size(), 0xdeadbeef);
    uint32_t check_addr = 0xc4000000;
    auto it = check_vec.begin();
    while (check_addr < 0xc4000000 + (1 << 18)) {
      read_memory(ip_addr.c_str(), check_addr, &*it,
                  256); // ugly maybe std::pointer_from will exist some day
      std::advance(it, 256);
      check_addr += 256 * 4;
    }
    REQUIRE(check_vec == test_wf_a);

    check_addr = 0xc6000000;
    it = check_vec.begin();
    while (check_addr < 0xc6000000 + (1 << 18)) {
      read_memory(ip_addr.c_str(), check_addr, &*it,
                  256); // ugly maybe std::pointer_from will exist some day
      std::advance(it, 256);
      check_addr += 256 * 4;
    }
    REQUIRE(check_vec == test_wf_b);

    // Check the sequence cache
    // Should have first two lines filled
    check_addr = 0xc2000000;
    check_vec.resize(2 * 128 * 2);
    it = check_vec.begin();
    while (check_addr < 0xc2000000 + (1 << 11)) {
      read_memory(ip_addr.c_str(), check_addr, &*it,
                  256); // ugly maybe std::pointer_from will exist some day
      std::advance(it, 256);
      check_addr += 256 * 4;
    }
    test_seq.resize(check_vec.size());
    REQUIRE(check_vec == test_seq);
  }
}
