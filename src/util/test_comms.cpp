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

int main(int argc, char const *argv[])
{

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

	//Generate a random 128MB vector to write
	std::default_random_engine generator;
	std::uniform_int_distribution<uint32_t> wordDistribution;

	vector<uint32_t> testVec;
	for (size_t ct=0; ct < (128*1048576/4); ct++){
		testVec.push_back(wordDistribution(generator));
	}

	//Choose a random starting point in the lower half of sequence memory
	std::uniform_int_distribution<uint32_t> addrDistribution(0, 0x20000000U);
	uint32_t startAddr = addrDistribution(generator);
	startAddr &= ~(0x3); //align address to 32 bit word

	cout << concol::RED << "Testing sequence memory:" << concol::RESET << endl;
	cout << concol::RED << "writing 128MB starting at " << hexn<8> << startAddr << " ..... ";
	auto start = std::chrono::steady_clock::now();
	write_memory(deviceSerial.c_str(), startAddr, testVec.data(), testVec.size());
	auto stop = std::chrono::steady_clock::now();
	cout << " took " << std::chrono::duration_cast<std::chrono::seconds>(stop-start).count() << " seconds." << concol::RESET << endl;

	//Check a few 256 count entries for correctness
	addrDistribution = std::uniform_int_distribution<uint32_t>(0, 0x20000000U-1024);
	for (size_t ct=0; ct < 10; ct++){
		vector<uint32_t> checkVec(256);
		uint32_t checkAddr = addrDistribution(generator);
		checkAddr &= !(0x3); //align address to 32 bit word
		cout << concol::RED << "reading 1024 bytes starting at " << hexn<8> << checkAddr << " ..... " << concol::RESET;
		read_memory(deviceSerial.c_str(), checkAddr, checkVec.data(), 256);
		bool passed = true;
		for (auto val : checkVec){
			if (val != testVec[checkAddr/4]) {
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

	disconnect_APS(deviceSerial.c_str());

	return 0;
}
