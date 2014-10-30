/*
Runs a read/write test to confirm communications integrity. 
*/

#include <iostream>
#include <sstream>
#include <iomanip>
#include <random>
#include <functional>
#include <chrono>
#include <vector>

#include "libaps2.h"
#include "../C++/helpers.h"

using std::cout;
using std::endl;
using std::string;
using std::vector;

// N-wide hex output with 0x
template <unsigned int N>
std::ostream& hexn(std::ostream& out)
{
    return out << "0x" << std::hex << std::setw(N) << std::setfill('0');
}


void run_test(string deviceSerial, string memArea, uint32_t memStartAddr, uint32_t memHighAddr, size_t testLength){
	//Generate a random vector to write
	std::default_random_engine generator;
	std::uniform_int_distribution<uint32_t> wordDistribution;

	vector<uint32_t> testVec(testLength/4);
	for (size_t ct=0; ct < (testLength/4); ct++){
		testVec[ct] = wordDistribution(generator);
	}

	//Choose a random starting point 
	std::uniform_int_distribution<uint32_t> addrDistribution(memStartAddr, memHighAddr-testLength );
	uint32_t testStartAddr = addrDistribution(generator);
	testStartAddr &= ~(0x3); //align address to 32 bit word

	cout << endl;
	cout << concol::CYAN << "Testing " << memArea << " memory:" << concol::RESET << endl;
	cout << "writing " << testLength/1024 << " kB starting at " << hexn<8> << testStartAddr << " ..... ";
	cout.flush();
	auto start = std::chrono::steady_clock::now();
	write_memory(deviceSerial.c_str(), testStartAddr, testVec.data(), testVec.size());
	auto stop = std::chrono::steady_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop-start).count();
	cout << " at " << static_cast<double>(testLength)/duration << " MB/s." << concol::RESET << endl;

	//Check a few 1kB count entries for correctness
	addrDistribution = std::uniform_int_distribution<uint32_t>(testStartAddr, testStartAddr+testLength-1024);
	for (size_t ct=0; ct < 10; ct++){
		vector<uint32_t> checkVec(256);
		uint32_t checkAddr = addrDistribution(generator);
		checkAddr &= ~(0x3); //align address to 32 bit word
		cout << "reading 1 kB starting at " << hexn<8> << checkAddr << " ..... ";
		read_memory(deviceSerial.c_str(), checkAddr, checkVec.data(), 256);
		bool passed = true;
		for (auto val : checkVec){
			if (val != testVec[(checkAddr-testStartAddr)/4]) {
				passed &= false;
			}
			checkAddr += 4;
		}
		if (passed) {
			cout << concol::GREEN << " passed" << concol::RESET << endl;
		}
		else {
			cout << concol::RED << " failed" << concol::RESET << endl;
		}
	}

}


int main(int argc, char const *argv[])
{
	cout << endl;
	cout << concol::BOLDMAGENTA << "APS2 Communications Test" << concol::RESET << endl;
	cout << endl;

	//Poll for which device to test
	string deviceSerial = get_device_id();
	if (deviceSerial.empty()){
		cout << concol::RED << "No APS2 devices connected! Exiting..." << concol::RESET << endl;
		return 0;
	}

	set_logging_level(logDEBUG1);

	connect_APS(deviceSerial.c_str());
	stop(deviceSerial.c_str());

	// force initialize device
	initAPS(deviceSerial.c_str(), 1);

	run_test(deviceSerial, "sequence", 0, 0x1FFFFFFFu, 4*(1<<20));
	run_test(deviceSerial, "waveform", 0x20000000u, 0x3FFFFFFFu, 4*(1<<20));
	run_test(deviceSerial, "sequence cache", 0xC2000000u, 0xC2007FFF, 8*(1<<10));
	run_test(deviceSerial, "waveform A cache", 0xC4000000u, 0xC403FFFF, 64*(1<<10));
	run_test(deviceSerial, "waveform B cache", 0xC6000000u, 0xC603FFFF, 64*(1<<10));

	disconnect_APS(deviceSerial.c_str());

	return 0;
}
