
#ifndef APS2ETHERNETPACKET_H_
#define APS2ETHERNETPACKET_H_

#include "headings.h"
#include "MACAddr.h"
#include "APS2Datagram.h"

struct APS2EthernetHeader {
	MACAddr  dest;
	MACAddr  src;
	uint16_t frameType;
	uint16_t seqNum;
	APS2Command command;
	uint32_t addr;
};

class APS2EthernetPacket{
public:
	APS2EthernetHeader header;
	vector<uint32_t> payload;

	APS2EthernetPacket();

	APS2EthernetPacket(APS2Command, const uint32_t & addr=0);
	APS2EthernetPacket(const MACAddr &, const MACAddr &, APS2Command, const uint32_t &);

	APS2EthernetPacket(const vector<uint8_t> &);

	static const size_t NUM_HEADER_BYTES = 24;

	vector<uint8_t> serialize() const ;
	size_t numBytes() const;

	static APS2EthernetPacket create_broadcast_packet();

	static vector<APS2EthernetPacket> chunk(uint32_t, const vector<uint32_t> &, APS2Command);

};

#endif //APS2ETHERNETPACKET_H_
