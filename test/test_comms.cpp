#include <string>
using std::string;

#include <iostream>
using std::cout;
using std::endl;

#include "catch.hpp"
#include "asio.hpp"

#include <random>

#include "libaps2.h"
#include "constants.h"
#include "concol.h"
#include "RandomHelpers.h"
#include "APS2Connector.h"

extern string ip_addr; //ip address from run_tests

// N-wide hex output with 0x
template <unsigned int N>
std::ostream& hexn(std::ostream& out)
{
	return out << "0x" << std::hex << std::setw(N) << std::setfill('0');
}

bool test_mem_write_read(uint32_t memStartAddr, uint32_t memHighAddr, size_t testLength, uint32_t alignmentMask){
	/*
	Helper function to test writing and reading to a range of memory with a particular alignment.
	*/

	auto test_vec = RandomHelpers::random_data(testLength/4);
	auto start_addr = RandomHelpers::random_address(memStartAddr, memHighAddr-testLength, alignmentMask);

	cout << "writing " << std::dec << testLength/(1 << 10) << " kB starting at " << hexn<8> << start_addr << "..... ";
	cout.flush();
	auto start = std::chrono::steady_clock::now();
	write_memory(ip_addr.c_str(), start_addr, test_vec.data(), test_vec.size());
	auto stop = std::chrono::steady_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop-start).count();
	cout << "at " << static_cast<double>(testLength)/duration << " MB/s." << concol::RESET << endl;

	//Check a few 1kB count entries for correctness
	std::uniform_int_distribution<uint32_t> addr_distribution(start_addr, start_addr+testLength-1024);
	bool passed = true;
	for (size_t ct=0; ct < 10; ct++){
		vector<uint32_t> checkVec(256, 0);
		uint32_t checkAddr = addr_distribution(generator);
		checkAddr &= ~(alignmentMask);
		read_memory(ip_addr.c_str(), checkAddr, checkVec.data(), 256);
		for (auto val : checkVec){
			if (val != test_vec[(checkAddr-start_addr)/4]) {
				passed &= false;
			}
			checkAddr += 4;
		}
	}

	return passed;

}

TEST_CASE("enumeration", "[enumerate]"){

	unsigned numDevices = 0;
	get_numDevices(&numDevices);
	REQUIRE( numDevices >= 1);

	const char ** serialBuffer = new const char*[numDevices];

	SECTION("get_device_IPs returns a list of valid IP addresses"){
			get_device_IPs(serialBuffer);

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

	set_logging_level(logDEBUG3);
	APS2Connector connection(ip_addr);

	SECTION("CSR register read"){
		//Read the memory offsets
		uint32_t data = 0xdeadbeef;
		read_memory(ip_addr.c_str(), WFA_OFFSET_ADDR, &data, 1);
		REQUIRE( data == WFA_OFFSET);
		read_memory(ip_addr.c_str(), WFB_OFFSET_ADDR, &data, 1);
		REQUIRE( data == WFB_OFFSET);
		read_memory(ip_addr.c_str(), SEQ_OFFSET_ADDR, &data, 1);
		REQUIRE( data == SEQ_OFFSET);
	}

	SECTION("CSR register write"){
		//Read/write the trigger interval
		uint32_t cur_val{0}, test_val{0}, check_val{0};
		read_memory(ip_addr.c_str(), TRIGGER_INTERVAL_ADDR, &cur_val, 1);
		test_val = cur_val ^ 0xdeadbeef;
		write_memory(ip_addr.c_str(), TRIGGER_INTERVAL_ADDR, &test_val, 1);
		read_memory(ip_addr.c_str(), TRIGGER_INTERVAL_ADDR, &check_val, 1);
		REQUIRE( check_val == test_val);
		//Set it back
		write_memory(ip_addr.c_str(), TRIGGER_INTERVAL_ADDR, &cur_val, 1);
	}

	SECTION("fpga temperature") {
		float temp;
		APS2_STATUS status;
		status = get_fpga_temperature(ip_addr.c_str(), &temp);
		REQUIRE( status == APS2_OK );
		REQUIRE ( temp > 0 );
		REQUIRE ( temp < 70 );
	}

	SECTION("variable write sizes") {
		//Variable write sizes from 4 to 256 words
		vector<uint32_t> outData, inData;
		size_t idx=0;
		std::default_random_engine generator;
		std::uniform_int_distribution<uint32_t> wordDistribution;
		for (unsigned writeSize=0; writeSize<=256; writeSize+=4){
			for (unsigned ct=0; ct<writeSize; ct++){
				outData.push_back(wordDistribution(generator));
			}
			APS2_STATUS status;
			status = write_memory(ip_addr.c_str(), idx*4, outData.data()+idx, writeSize);
			REQUIRE( status == APS2_OK );
			idx += writeSize;
		}

		//Read data back in 128 words at a time to support legacy firmware
		inData.resize(idx);
		idx = 0;
		while (idx < inData.size()){
			read_memory(ip_addr.c_str(),	idx*4, inData.data()+idx, 128);
			idx += 128;
		}
		REQUIRE( outData == inData );
	}

	SECTION("CACHE BRAMs write/read") {
		bool passed;
		//sequence
		cout << "seq. cache: ";
		passed = test_mem_write_read(0xc2000000, 0xc2003fff, 1 << 11, 0x3);
		REQUIRE(passed);
		// wfA
		cout << "wf. a cache: ";
		passed = test_mem_write_read(0xc4000000, 0xc403ffff, 1 << 14, 0x3);
		REQUIRE(passed);
		//wfB
		cout << "wf. b cache: ";
		passed = test_mem_write_read(0xc6000000, 0xc603ffff, 1 << 14, 0x3);
		REQUIRE(passed);
	}

}

TEST_CASE("sequencer SDRAM write/read", "[sequencer SDRAM]") {

	set_logging_level(logDEBUG3);
	APS2Connector connection(ip_addr);

	SECTION("basic write/read") {
		//Older firmware is too slow for long write tests
		uint32_t firmware_version;
		get_firmware_version(ip_addr.c_str(), &firmware_version, nullptr, nullptr, nullptr);
		size_t testLength = ( (firmware_version & 0x00000fff) >= 0x300) ? (1 << 25) : (1 << 21);
		bool passed;
		//sequence
		cout << "seq. memory: ";
		passed = test_mem_write_read(SEQ_OFFSET, SEQ_OFFSET + (1 << 29), testLength, 0xf);
		REQUIRE(passed);
		//waveform
		cout << "wf. memory: ";
		passed = test_mem_write_read(WFA_OFFSET, WFA_OFFSET + (1 << 29), testLength, 0xf);
		REQUIRE(passed);
	}

}


TEST_CASE("configuration SDRAM writing and reading", "[configuration SDRAM]") {

	APS2Connector connection(ip_addr);

	//Older firmware is too slow for long write tests
	uint32_t firmware_version;
	get_firmware_version(ip_addr.c_str(), &firmware_version, nullptr, nullptr, nullptr);
	size_t testLength = ( (firmware_version & 0x00000fff) >= 0x300) ? (1 << 22) : (1 << 18);
	//write to configuration memory
	auto test_vec = RandomHelpers::random_data(testLength/4);
	uint32_t alignment = 0x7; //8byte
	auto start_addr = RandomHelpers::random_address(0, 0x07ffffff-testLength, alignment); //128MB
	cout << "config. memory: writing " << std::dec << testLength/(1 << 10) << " kB starting at " << hexn<8> << start_addr << "..... ";
	cout.flush();
	auto start = std::chrono::steady_clock::now();
	write_configuration_SDRAM(ip_addr.c_str(), start_addr, test_vec.data(), test_vec.size());
	auto stop = std::chrono::steady_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop-start).count();
	cout << "at " << static_cast<double>(testLength)/duration << " MB/s." << concol::RESET << endl;

	//Check a few 1kB chunks
	bool passed = true;
	std::uniform_int_distribution<uint32_t> addr_distribution(start_addr, start_addr+testLength-1024);
	for (size_t ct = 0; ct < 10; ct++) {
		uint32_t check_addr = addr_distribution(generator);
		check_addr &= ~(alignment);
		vector<uint32_t> check_vec(256, 0xdeadbeef);
		read_configuration_SDRAM(ip_addr.c_str(), check_addr, check_vec.size(), check_vec.data());
		for (auto val : check_vec){
			if (val != test_vec[(check_addr-start_addr)/4]) {
				passed &= false;
			}
			check_addr += 4;
		}
	}
	REQUIRE(passed);

}

TEST_CASE("eprom read/write", "[eprom]") {

	set_logging_level(logDEBUG3);
	APS2Connector connection(ip_addr);

	SECTION("basic write/read") {
		//test writing/reading 128kB
		size_t test_length = 128*(1<<10);
		auto test_vec = RandomHelpers::random_data(test_length/4);
		//backup image starts at 16MB or 0x01000000 and should be no more than 10MB
		//so test between 28 and 32MB
		uint32_t alignment = 0xffff; //64kB aligned for erase
		auto start_addr = RandomHelpers::random_address(0x01c00000, 0x01ffffff-test_length, alignment);
		cout << "ERPOM: writing " << std::dec << test_length/(1 << 10) << " kB starting at " << hexn<8> << start_addr << "..... ";
		cout.flush();
		auto start = std::chrono::steady_clock::now();
		APS2_STATUS status;
		status = write_flash(ip_addr.c_str(), start_addr, test_vec.data(), test_vec.size());
		REQUIRE(status == APS2_OK);
		auto stop = std::chrono::steady_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop-start).count();
		cout << "at " << static_cast<double>(test_length)/duration << " MB/s." << concol::RESET << endl;

		//Check the data
		uint32_t check_addr{start_addr};
		vector<uint32_t> check_vec(test_vec.size(), 0xdeadbeef);
		auto it = check_vec.begin();
		while (check_addr < start_addr + test_length) {
			read_flash(ip_addr.c_str(), check_addr, 256, &*it); //ugly maybe std::pointer_from will exist some day
			std::advance(it, 256);
			check_addr += 256*4;
		}
		REQUIRE( check_vec == test_vec );
	}

}

TEST_CASE("SPI", "[SPI]") {

	set_logging_level(logDEBUG3);
	APS2Connector connection(ip_addr);

	SECTION("get_sampleRate") {
		unsigned sample_rate;
		APS2_STATUS status;
		status = get_sampleRate(ip_addr.c_str(), &sample_rate);
		REQUIRE( status == APS2_OK );
		//expect 1200 for now
		REQUIRE( sample_rate == 1200 );
	}

}
