#include <string>
using std::string;

#include <iostream>
using std::cout;
using std::endl;

#include "catch.hpp"
#include "asio.hpp"

#include "libaps2.h"
#include "constants.h"


void run_test(string deviceSerial, string memArea, uint32_t memStartAddr, uint32_t memHighAddr, size_t testLength, uint32_t alignmentMask){
	/*
	Helper function to test writing and reading to a range of memory with a particular alignment.
	*/

	//Generate a random vector to write
	std::default_random_engine generator;
	std::uniform_int_distribution<uint32_t> wordDistribution;

	vector<uint32_t> testVec(testLength/4);
	for (size_t ct=0; ct < (testLength/4); ct++){
		testVec[ct] = wordDistribution(generator);
	}

	//Choose a random starting point
	std::uniform_int_distribution<uint32_t> addrDistribution(memStartAddr, memHighAddr-testLength);
	uint32_t testStartAddr = addrDistribution(generator);
	testStartAddr &= ~(alignmentMask); //align address

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
		vector<uint32_t> checkVec(256, 0);
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


TEST_CASE("enumeration", "[enumeration]"){

	unsigned numDevices = 0;
	get_numDevices(&numDevices);
	REQUIRE( numDevices >= 1);

	const char ** serialBuffer = new const char*[numDevices];

	SECTION("get_deviceSerials returns a list of valid IP addresses"){
	    get_deviceSerials(serialBuffer);

		for (size_t ct = 0; ct < numDevices; ct++) {
			string ipAddr = string(serialBuffer[ct]);
	        asio::error_code ec;
	        asio::ip::address_v4::from_string(ipAddr, ec);
			CAPTURE( ipAddr );
			REQUIRE( !ec );
		}
	}

	delete[] serialBuffer;
}

TEST_CASE("memory writing and reading", "[read_memory,write_memory]") {

	unsigned numDevices = 0;
	get_numDevices(&numDevices);
	const char ** serialBuffer = new const char*[numDevices];
	get_deviceSerials(serialBuffer);
	string testDevIP = string(serialBuffer[1]);
	delete[] serialBuffer;

	connect_APS(testDevIP.c_str());
	init_APS(testDevIP.c_str(), 1);

	SECTION("CSR register read"){
		//Read the memory offsets
		uint32_t data = 0xbaadbaad;
		read_memory(testDevIP.c_str(), WFA_OFFSET_ADDR, &data, 1);
		REQUIRE( data == WFA_OFFSET);
		read_memory(testDevIP.c_str(), WFB_OFFSET_ADDR, &data, 1);
		REQUIRE( data == WFB_OFFSET);
		read_memory(testDevIP.c_str(), SEQ_OFFSET_ADDR, &data, 1);
		REQUIRE( data == SEQ_OFFSET);
	}

	SECTION("CSR register read"){
		//Read the memory offsets
		uint32_t data = 0xbaadbaad;
		read_memory(testDevIP.c_str(), WFA_OFFSET_ADDR, &data, 1);
		REQUIRE( data == WFA_OFFSET);
		read_memory(testDevIP.c_str(), WFB_OFFSET_ADDR, &data, 1);
		REQUIRE( data == WFB_OFFSET);
		read_memory(testDevIP.c_str(), SEQ_OFFSET_ADDR, &data, 1);
		REQUIRE( data == SEQ_OFFSET);
	}


	disconnect_APS(testDevIP.c_str());

}
