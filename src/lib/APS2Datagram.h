// Datagrams that are sent/received from the APS2
// Consists of a vector of 32 bit words
// 1. command word - see APS2Command
// 2. address
// 3. payload
//
// Original author: Colm Ryan
// Copyright 2016 Raytheon BBN Technologies

#ifndef APS2DATAGRAM_H_
#define APS2DATAGRAM_H_

#include <vector>
using std::vector;
#include <cstdint>
#include <string>
using std::string;

//APS Command Protocol
// ACK SEQ SEL R/W CMD<3:0> MODE/STAT<7:0> CNT<15:0>
// 31 30 29 28 27..24 23..16 15..0
// ACK .......Acknowledge Flag. Set in the Acknowledge Packet returned in response to a
// Command Packet. Must be zero in a Command Packet.
// SEQ............Set for Sequence Error. MODE/STAT = 0x01 for skip and 0x00 for duplicate.
// SEL........Channel Select. Selects target for commands with more than one target. Zero
// if not used. Unmodified in the Acknowledge Packet.
// R/W ........Read/Write. Set for read commands, cleared for write commands. Unmodified
// in the Acknowledge Packet.
// CMD<3:0> ....Specifies the command to perform when the packet is received by the APS
// module. Unmodified in the Acknowledge Packet. See section 3.8 for
// information on the supported commands.
// MODE/STAT<7:0> ....Command Mode or Status. MODE bits modify the operation of some
// commands. STAT bits are returned in the Acknowledge Packet to indicate
// command completion status. A STAT value of 0xFF indicates an invalid or
// unrecognized command. See individual command descriptions for more
// information.
// CNT<15:0> ...Number of 32-bit data words to transfer for a read or a write command. Note
// that the length does NOT include the Address Word. CNT must be at least 1.
// To meet Ethernet packet length limitations, CNT must not exceed 366.

union APS2Command {
	struct {
	uint32_t cnt : 16;
	uint32_t mode_stat : 8;
	uint32_t cmd : 4;
	uint32_t r_w : 1;
	uint32_t sel : 1;
	uint32_t seq : 1;
	uint32_t ack : 1;
	};
	uint32_t packed;
	//TODO: sort out the default initialization and whether this is necessary
	APS2Command() {this->packed = 0;};
	string to_string();
};

class APS2Datagram {
private:
  /* data */
public:
  APS2Command cmd;
  uint32_t addr;
	vector<uint32_t> payload;
	vector<uint32_t> data() const;

  static vector<APS2Datagram> chunk(APS2Command, uint32_t, const vector<uint32_t>&, uint16_t);

	void check_ack(const APS2Datagram &, bool legacy_firmware) const;

};

#endif //APS2DATAGRAM_H_
