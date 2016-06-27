// Test CSR setters/getters
//
// Original author: Colm Ryan
// Copyright 2016 Raytheon BBN Technologies

#include <string>
#include <random>
using std::string;

#include "catch.hpp"
#include "libaps2.h"
#include "constants.h"

#include "APS2Connector.h"

extern string ip_addr; //ip address from run_tests

TEST_CASE("CSR", "CSR") {

	set_logging_level(logDEBUG1);
	APS2Connector connection(ip_addr);
	APS2_STATUS status;

	std::random_device rd;
	std::default_random_engine engine(rd());
	std::uniform_int_distribution<int> channel_dist(0, 1);

	SECTION("direct CSR register read"){
		//Direct memory reads
		uint32_t data = 0xdeadbeef;
		status = read_memory(ip_addr.c_str(), WFA_OFFSET_ADDR, &data, 1);
		REQUIRE( status == APS2_OK );
		REQUIRE( data == WFA_OFFSET);
		status = read_memory(ip_addr.c_str(), WFB_OFFSET_ADDR, &data, 1);
		REQUIRE( status == APS2_OK );
		REQUIRE( data == WFB_OFFSET);
		status = read_memory(ip_addr.c_str(), SEQ_OFFSET_ADDR, &data, 1);
		REQUIRE( status == APS2_OK );
		REQUIRE( data == SEQ_OFFSET);
	}

	SECTION("direct CSR register write"){
		//direct memory writes Read/write the trigger interval
		uint32_t cur_val{0}, test_val{0}, check_val{0};
		status = read_memory(ip_addr.c_str(), TRIGGER_INTERVAL_ADDR, &cur_val, 1);
		REQUIRE( status == APS2_OK );
		test_val = cur_val ^ 0xdeadbeef;
		status = write_memory(ip_addr.c_str(), TRIGGER_INTERVAL_ADDR, &test_val, 1);
		REQUIRE( status == APS2_OK );
		status = read_memory(ip_addr.c_str(), TRIGGER_INTERVAL_ADDR, &check_val, 1);
		REQUIRE( status == APS2_OK );
		REQUIRE( check_val == test_val);
		//Set it back
		status = write_memory(ip_addr.c_str(), TRIGGER_INTERVAL_ADDR, &cur_val, 1);
		REQUIRE( status == APS2_OK );
	}

	SECTION("fpga temperature") {
		//Expect chip temperature to be in a reasonable range
		float temp;
		status = get_fpga_temperature(ip_addr.c_str(), &temp);
		REQUIRE( status == APS2_OK );
		REQUIRE ( temp > 0 );
		REQUIRE ( temp < 75 );
	}

	SECTION("setters/getters") {
		//trigger interval
		double trigger_interval{0}, test_val_d, check_val_d;
		//get current value
		status = get_trigger_interval(ip_addr.c_str(), &trigger_interval);
		REQUIRE( status == APS2_OK );
		std::uniform_real_distribution<double> trigger_dist(0, 1.0);
		test_val_d = trigger_dist(engine);
		status = set_trigger_interval(ip_addr.c_str(), test_val_d);
		REQUIRE( status == APS2_OK );
		status = get_trigger_interval(ip_addr.c_str(), &check_val_d);
		REQUIRE( status == APS2_OK );
		REQUIRE( check_val_d == Approx( test_val_d ).epsilon( 3.33e-9) );
		//set it back
		status = set_trigger_interval(ip_addr.c_str(), trigger_interval);
		REQUIRE( status == APS2_OK );

		//run mode
		//TODO when we have a getter
		// APS2_RUN_MODE run_mode;

		//waveform frequency
		float waveform_freq, test_val_f, check_val_f;
		//get current value
		status = get_waveform_frequency(ip_addr.c_str(), &waveform_freq);
		REQUIRE( status == APS2_OK );
		std::uniform_real_distribution<float> freq_dist(-600e6, 600e6);
		test_val_f = freq_dist(engine);
		status = set_waveform_frequency(ip_addr.c_str(), test_val_f);
		REQUIRE( status == APS2_OK );
		status = get_waveform_frequency(ip_addr.c_str(), &check_val_f);
		REQUIRE( status == APS2_OK );
		REQUIRE( check_val_f == Approx( test_val_f ).epsilon( 1.0 ) );
		//set it back
		status = set_waveform_frequency(ip_addr.c_str(), waveform_freq);
		REQUIRE( status == APS2_OK );

		//channel offset
		int chan = channel_dist(engine);
		float offset;
		//get current value
		status = get_channel_offset(ip_addr.c_str(), chan, &offset);
		REQUIRE( status == APS2_OK );
		std::uniform_real_distribution<float> offset_dist(-1,1);
		test_val_f = offset_dist(engine);
		status = set_channel_offset(ip_addr.c_str(), chan, test_val_f);
		REQUIRE( status == APS2_OK );
		status = get_channel_offset(ip_addr.c_str(), chan, &check_val_f);
		REQUIRE( status == APS2_OK );
		REQUIRE( check_val_f == Approx( test_val_f ).epsilon( 0.001 ) );
		//set it back
		status = set_channel_offset(ip_addr.c_str(), chan, offset);
		REQUIRE( status == APS2_OK );

		//channel scale
		chan = channel_dist(engine);
		float scale;
		//get current value
		status = get_channel_scale(ip_addr.c_str(), chan, &scale);
		REQUIRE( status == APS2_OK );
		std::uniform_real_distribution<float> scale_dist(-2,2);
		test_val_f = scale_dist(engine);
		status = set_channel_scale(ip_addr.c_str(), chan, test_val_f);
		REQUIRE( status == APS2_OK );
		status = get_channel_scale(ip_addr.c_str(), chan, &check_val_f);
		REQUIRE( status == APS2_OK );
		REQUIRE( check_val_f == Approx( test_val_f ).epsilon( 0.001 ) );
		//set it back
		status = set_channel_scale(ip_addr.c_str(), chan, scale);
		REQUIRE( status == APS2_OK );

		//mixer amplitude imbalance
		float imbalance;
		//get current value
		status = get_mixer_amplitude_imbalance(ip_addr.c_str(), &imbalance);
		REQUIRE( status == APS2_OK );
		std::uniform_real_distribution<float> imbalance_dist(-1,1);
		test_val_f = imbalance_dist(engine);
		status = set_mixer_amplitude_imbalance(ip_addr.c_str(), test_val_f);
		REQUIRE( status == APS2_OK );
		status = get_mixer_amplitude_imbalance(ip_addr.c_str(), &check_val_f);
		REQUIRE( status == APS2_OK );
		REQUIRE( check_val_f == Approx( test_val_f ) );
		//set it back
		status = set_mixer_amplitude_imbalance(ip_addr.c_str(), imbalance);
		REQUIRE( status == APS2_OK );

		//mixer phase skew
		float phase_skew;
		//get current value
		status = get_mixer_phase_skew(ip_addr.c_str(), &phase_skew);
		REQUIRE( status == APS2_OK );
		std::uniform_real_distribution<float> phase_skew_dist(-1,1);
		test_val_f = phase_skew_dist(engine);
		status = set_mixer_phase_skew(ip_addr.c_str(), test_val_f);
		REQUIRE( status == APS2_OK );
		status = get_mixer_phase_skew(ip_addr.c_str(), &check_val_f);
		REQUIRE( status == APS2_OK );
		REQUIRE( check_val_f == Approx( test_val_f ) );
		//set it back
		status = set_mixer_phase_skew(ip_addr.c_str(), phase_skew);
		REQUIRE( status == APS2_OK );

	}

}
