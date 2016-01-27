#include <string>
using std::string;

#include <iostream>
using std::cout;
using std::endl;

#include "catch.hpp"
#include "asio.hpp"

#include "libaps2.h"
#include "constants.h"
#include "concol.h"

extern string ip_addr; //ip address from run_tests

static std::default_random_engine generator(std::random_device{}());

// N-wide hex output with 0x
template <unsigned int N>
std::ostream& hexn(std::ostream& out)
{
  return out << "0x" << std::hex << std::setw(N) << std::setfill('0');
}

vector<uint32_t> random_data(size_t length) {
  //Create random data to write/read
	std::uniform_int_distribution<uint32_t> word_distribution;
	vector<uint32_t> data(length);
	for (auto & val : data){
    val = word_distribution(generator);
	}
  return data;
}

uint32_t random_address(uint32_t mem_low, uint32_t mem_high, uint32_t alignment) {
  //Create a random starting address aligned appropriately
  //Choose a random starting point
	std::uniform_int_distribution<uint32_t> addr_distribution(mem_low, mem_high);
	uint32_t start_addr = addr_distribution(generator);
	start_addr &= ~(alignment); //align address
  return start_addr;
}

bool test_mem_write_read(uint32_t memStartAddr, uint32_t memHighAddr, size_t testLength, uint32_t alignmentMask, bool printSpeed){
	/*
	Helper function to test writing and reading to a range of memory with a particular alignment.
	*/

  auto test_vec = random_data(testLength/4);
  auto start_addr = random_address(memStartAddr, memHighAddr-testLength, alignmentMask);

  if ( printSpeed ) {
    cout << "writing " << std::dec << testLength/(1 << 10) << " kB starting at " << hexn<8> << start_addr << "..... ";
    cout.flush();
    auto start = std::chrono::steady_clock::now();
    write_memory(ip_addr.c_str(), start_addr, test_vec.data(), test_vec.size());
    auto stop = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop-start).count();
    cout << "at " << static_cast<double>(testLength)/duration << " MB/s." << concol::RESET << endl;
  } else {
    write_memory(ip_addr.c_str(), start_addr, test_vec.data(), test_vec.size());
  }

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

	connect_APS(ip_addr.c_str());
	// init_APS(ip_addr.c_str(), 1);

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

  SECTION("variable write sizes") {
    //Variable write sizes from 4 to 256 words
    vector<uint32_t> outData, inData;
  	size_t idx=0;
  	std::default_random_engine generator;
  	std::uniform_int_distribution<uint32_t> wordDistribution;
  	for (unsigned writeSize=4; writeSize<=256; writeSize+=4){
  		for (unsigned ct=0; ct<writeSize; ct++){
  			outData.push_back(wordDistribution(generator));
  		}
  		write_memory(ip_addr.c_str(), idx*4, outData.data()+idx, writeSize);
  		idx += writeSize;
  	}
    //Read data back in 128 words at a time to support legacy firmware
  	inData.resize(idx);
  	idx = 0;
  	while (idx < inData.size()){
  		read_memory(ip_addr.c_str(),  idx*4, inData.data()+idx, 128);
  		idx += 128;
  	}

  	bool passed = true;
  	for (size_t ct=0; ct<inData.size(); ct++){
  		passed &= (inData[ct] == outData[ct]);
  	}
    REQUIRE(passed);
  }

  SECTION("CACHE BRAMs write/read") {
    bool passed;
    //sequence
    cout << "seq. cache: ";
    passed = test_mem_write_read(0xc2000000, 0xc2003fff, 1 << 12, 0x3, true);
    REQUIRE(passed);
    // wfA
    cout << "wf. a cache: ";
    passed = test_mem_write_read(0xc4000000, 0xc403ffff, 1 << 14, 0x3, true);
    REQUIRE(passed);
    //wfB
    cout << "wf. b cache: ";
    passed = test_mem_write_read(0xc6000000, 0xc603ffff, 1 << 14, 0x3, true);
    REQUIRE(passed);
  }

  SECTION("sequencer SDRAM write/read") {
    //Older firmware is too slow for long write tests
    uint32_t firmware_version;
    get_firmware_version(ip_addr.c_str(), &firmware_version);
    size_t testLength = ( (firmware_version & 0x00000fff) >= 0x300) ? (1 << 25) : (1 << 21);
    bool passed;
    //sequence
    cout << "seq. memory: ";
    passed = test_mem_write_read(SEQ_OFFSET, SEQ_OFFSET + (1 << 29), testLength, 0xf, true);
    REQUIRE(passed);
    //waveform
    cout << "wf. memory: ";
    passed = test_mem_write_read(WFA_OFFSET, WFA_OFFSET + (1 << 29), testLength, 0xf, true);
    REQUIRE(passed);
  }

  // SECTION("bitfile configuration SDRAM write/read") {
  //   //Older firmware is too slow for long write tests
  //   uint32_t firmware_version;
  //   get_firmware_version(ip_addr.c_str(), &firmware_version);
  //   size_t testLength = ( (firmware_version & 0x00000fff) >= 0x300) ? (1 << 22) : (1 << 20);
  //   testLength = 32*4;
  //   //write to configuration memory
  //
  //
  //   bool passed;
  //   //sequence
  //   passed = test_mem_write_read(SEQ_OFFSET, SEQ_OFFSET + (1 << 29), testLength, 0xf, true);
  //   REQUIRE(passed);
  //   //waveform
  //   passed = test_mem_write_read(WFA_OFFSET, WFA_OFFSET + (1 << 29), testLength, 0xf, true);
  //   REQUIRE(passed);
  // }

	disconnect_APS(ip_addr.c_str());

}
