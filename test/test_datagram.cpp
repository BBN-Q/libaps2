#include "catch.hpp"

#include "APS2Datagram.h"

TEST_CASE("APS2Datagram chunking", "[datagram]") {

    APS2Command cmd;
    cmd.packed = 0xbaad0000;

    vector<APS2Datagram> chunks = APS2Datagram::chunk(cmd, 0xdeadbeef, {1,2,3}, 1);
    unsigned ct = 0;
    for (auto dg : chunks) {
      REQUIRE( dg.cmd.packed == 0xbaad0001);
      REQUIRE( dg.addr == 0xdeadbeef);
      REQUIRE( dg.payload == vector<uint32_t>{ct+1});
      ct++;
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
