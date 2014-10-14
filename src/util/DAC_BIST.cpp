/*
Runs the DAC BIST test to confirm analog output data integrity. 
*/

#include <iostream>
#include <random>
#include <functional>

#include "libaps2.h"
#include "../C++/helpers.h"

using std::cout;
using std::endl;
using std::string;
using std::vector;

int main(int argc, char const *argv[])
{

	//Poll for which device to test
	string deviceSerial = get_device_id();

	set_logging_level(4);

	connect_APS(deviceSerial.c_str());

	// force initialize device
	initAPS(deviceSerial.c_str(), 1);

	//Generate random bit values
	std::default_random_engine generator;
	std::uniform_int_distribution<int> bitDistribution(0,1);
	std::uniform_int_distribution<int16_t> wordDistribution(-8192, 8191);
	auto randbit = std::bind(bitDistribution, generator);
	auto randWord = std::bind(wordDistribution, generator);

	vector<int16_t> testVec;
	vector<uint32_t> results(8);
	string FPGAPhase1;
	string FPGAPhase2;
	string LVDSPhase1;
	string LVDSPhase2;
	string SYNCPhase1;
	string SYNCPhase2;

	//Loop through DACs
	for (int dac = 0; dac < 2; ++dac)
	{
		cout << "DAC " << dac << endl;

		for (int bit = 0; bit < 14; ++bit)
		{
			cout << "Testing bit " << bit << ": ";
			testVec.clear();
			for (int ct = 0; ct < 100; ++ct)
			{
				testVec.push_back( (randbit() << bit) * (bit == 13 ? -1 : 1) );
			}
			run_DAC_BIST(deviceSerial.c_str(), dac, testVec.data(), testVec.size(), results.data());

			FPGAPhase1 = results[2] == results[0] ? "pass" : "fail";
			FPGAPhase2 = results[3] == results[1] ? "pass" : "fail";
			LVDSPhase1 = results[4] == results[0] ? "pass" : "fail";
			LVDSPhase2 = results[5] == results[1] ? "pass" : "fail";
			SYNCPhase1 = results[6] == results[4] ? "pass" : "fail";
			SYNCPhase2 = results[7] == results[5] ? "pass" : "fail";

			cout << "FPGA: " << FPGAPhase1 << " / " << FPGAPhase2 << " LVDS: " << LVDSPhase1 << " / " << LVDSPhase2 << " SYNC: " << SYNCPhase1 << " / " << SYNCPhase2 << endl;

		}
		cout << endl;
		cout << "Testing random waveform... ";
		testVec.clear();
		for (int ct = 0; ct < 8000; ++ct)
		{
			testVec.push_back(randWord());
		}
		run_DAC_BIST(deviceSerial.c_str(), dac, testVec.data(), testVec.size(), results.data());

		FPGAPhase1 = results[2] == results[0] ? "pass" : "fail";
		FPGAPhase2 = results[3] == results[1] ? "pass" : "fail";
		LVDSPhase1 = results[4] == results[0] ? "pass" : "fail";
		LVDSPhase2 = results[5] == results[1] ? "pass" : "fail";
		SYNCPhase1 = results[6] == results[4] ? "pass" : "fail";
		SYNCPhase2 = results[7] == results[5] ? "pass" : "fail";

		cout << "FPGA: " << FPGAPhase1 << " / " << FPGAPhase2 << " LVDS: " << LVDSPhase1 << " / " << LVDSPhase2 << " SYNC: " << SYNCPhase1 << " / " << SYNCPhase2 << endl;
		cout << endl;
		cout << endl;
	}

	disconnect_APS(deviceSerial.c_str());

	return 0;
}