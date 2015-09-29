#include <string>
using std::string;

#include <iostream>
using std::cout;
using std::endl;

#include "catch.hpp"
#include "asio.hpp"

#include "libaps2.h"
#include "constants.h"

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

	disconnect_APS(testDevIP.c_str());

}
