#include "catch.hpp"

#include <vector>
using std::vector;

#include <plog/Log.h>

#include "libaps2.h"

#include "APS2Connector.h"
#include "RandomHelpers.h"

extern string ip_addr; // ip address from run_tests

TEST_CASE("DAC BIST", "[DAC BIST]") {

	APS2Connector connection(ip_addr);
	init_APS(ip_addr.c_str(), 0);

	set_file_logging_level(plog::debug);
  	set_console_logging_level(plog::debug);

	for (size_t dac = 0; dac < 2; dac++) {
		auto test_wf = RandomHelpers::random_waveform( 1 << 17 ); // 128 ksamples
		vector<uint32_t> results(8);
		auto passed = run_DAC_BIST(ip_addr.c_str(), dac, test_wf.data(), test_wf.size(),
                 results.data());
		REQUIRE( passed == 1 );
	}
}
