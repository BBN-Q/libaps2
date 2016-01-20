#include "catch.hpp"

#include "APS2Datagram.h"


TEST_CASE("APS2Datagram chunking", "[datagram]") {

    APS2Datagram datagram{{.packed=0xdeadbeef}, 0xbaadbaad, {1,2,3}};

    vector<APS2Datagram> chunks = datagram.chunk(1);
    unsigned ct = 0;
    for (auto dg : chunks) {
      REQUIRE( dg.cmd.packed == 0xdeadbeef);
      REQUIRE( dg.addr == 0xbaadbaad);
      REQUIRE( dg.payload == vector<uint32_t>{ct+1});
      ct++;
    }
}
