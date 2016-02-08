#include "catch.hpp"

#include "APS2Datagram.h"

TEST_CASE("APS2Datagram chunking", "[datagram]") {

	APS2Command cmd;
	cmd.packed = 0xbaad0000;

	SECTION("small chunks") {
		vector<APS2Datagram> chunks = APS2Datagram::chunk(cmd, 0xdeadbeef, {1,2,3}, 1);
		unsigned ct = 0;
		for (auto dg : chunks) {
			REQUIRE( dg.cmd.packed == 0xbaad0001);
			REQUIRE( dg.addr == 0xdeadbeef + 4*ct);
			REQUIRE( dg.payload == vector<uint32_t>({ct+1}));
			ct++;
		}
	}

	SECTION("chunk less than max") {
		vector<APS2Datagram> chunks = APS2Datagram::chunk(cmd, 0xdeadbeef, {1,2,3}, 0xfffc);
		unsigned ct = 0;
		REQUIRE( chunks.size() == 1);
		REQUIRE( chunks[0].cmd.packed == 0xbaad0003);
		REQUIRE( chunks[0].addr == 0xdeadbeef);
		REQUIRE( chunks[0].payload == vector<uint32_t>({1,2,3}));
	}


}
TEST_CASE("APS2Datagram data", "[datagram]") {

	APS2Command cmd;
	cmd.packed = 0xbaad0000;
	uint32_t addr{0xdeadbeef};
	vector<uint32_t> payload;
	for (size_t ct = 0; ct < 100; ct++) {
		payload.push_back(ct);
	}
	cmd.cnt = payload.size();
	APS2Datagram dg{cmd, addr, payload};

	auto data = dg.data();
	REQUIRE( data[0] == (0xbaad0000 | (payload.size() & 0x0000ffff)) );
	REQUIRE( data[1] == 0xdeadbeef );
	REQUIRE( vector<uint32_t>(data.begin()+2,data.end()) == payload );

}
