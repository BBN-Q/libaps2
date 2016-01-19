// Ethernet communications with APS2 boards
//
// Original authors: Colm Ryan, Blake Johnson, Brian Donovan
// Copyright 2016, Raytheon BBN Technologies

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
	uint16_t seqNum = 0;
	bool supports_tcp = false;
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
	vector<APSEthernetPacket> receive(string serial, size_t numPackets = 1, size_t timeoutMS = 2000);

private:
	APSEthernet(APSEthernet const &) = delete;

	MACAddr srcMAC_;

	//Keep track of all the device info with a map from I.P. addresses to devInfo structs
	unordered_map<string, EthernetDevInfo> devInfo_;

	unordered_map<string, queue<APSEthernetPacket>> msgQueues_;

	vector<std::pair<string,string>> get_local_IPs();

	void reset_maps();

	//ASIO service and sockets
	asio::io_service ios_;
	udp::socket udp_socket_old_;
	udp::socket udp_socket_;

	void setup_udp_receive();
	void setup_udp_receive_old();

	void sort_packet(const vector<uint8_t> &, const udp::endpoint &);

	int send_chunk(string, vector<APSEthernetPacket>, bool);

	// storage for received packets
 	uint8_t receivedData_[2048];
	udp::endpoint senderEndpoint_;

	std::thread receiveThread_;
	std::mutex mLock_;
};


#endif
