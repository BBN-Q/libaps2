#ifndef APSETHERNET_H
#define APSETHERNET_H

#include "headings.h"
#include "MACAddr.h"
#include "APSEthernetPacket.h"
#include "APS2_errno.h"

#include "asio.hpp"

using asio::ip::udp;

struct EthernetDevInfo {
	MACAddr macAddr;
	udp::endpoint endpoint;
	uint16_t seqNum;
};

class APSEthernet {
public:

	APSEthernet& operator=(APSEthernet &rhs)  { return rhs; };

	APSEthernet();
	~APSEthernet();
	void init();
	set<string> enumerate();
	void connect(string serial);
	void disconnect(string serial);
	int send(string serial, APSEthernetPacket msg, bool checkResponse=true);
	int send(string serial, vector<APSEthernetPacket> msg, unsigned ackEvery=1);
	vector<APSEthernetPacket> receive(string serial, size_t numPackets = 1, size_t timeoutMS = 10000);

private:
	APSEthernet(APSEthernet const &) = delete;

	MACAddr srcMAC_;

	//Keep track of all the device info with a map from I.P. addresses to devInfo structs
	unordered_map<string, EthernetDevInfo> devInfo_;

	unordered_map<string, queue<APSEthernetPacket>> msgQueues_;

	vector<std::pair<string,string>> get_local_IPs();

	void reset_maps();

	void setup_receive();
	void sort_packet(const vector<uint8_t> &, const udp::endpoint &);

	int send_chunk(string, vector<APSEthernetPacket>, bool);

	asio::io_service ios_;
	udp::socket socket_;

	// storage for received packets
 	uint8_t receivedData_[2048];
	udp::endpoint senderEndpoint_;

	std::thread receiveThread_;
	std::mutex mLock_;
};


#endif
