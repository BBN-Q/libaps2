// Ethernet communications with APS2 boards
//
// Original authors: Colm Ryan, Blake Johnson, Brian Donovan
// Copyright 2016, Raytheon BBN Technologies


#ifdef _WIN32
#include "iphlpapi.h"
#else
#include <ifaddrs.h>
#endif
#include <unordered_map>
using std::unordered_map;
#include <queue>
using std::queue;

#include "APS2Ethernet.h"
#include "helpers.h"
#include "logger.h"
#include "constants.h"

APS2Ethernet::APS2Ethernet() : udp_socket_old_(ios_), udp_socket_(ios_) {
	FILE_LOG(logDEBUG) << "APS2Ethernet::APS2Ethernet";

	//Move the io_service to a background thread
	//Give it something to chew on so that it doesn't return from the background thread
	asio::io_service::work work(ios_);
	receiveThread_ = std::thread([this](){ ios_.run(); });

	//Bind the old UDP socket at local port bb4e
	try {
		udp_socket_old_.open(udp::v4());
		udp_socket_old_.bind(udp::endpoint(udp::v4(), UDP_PORT_OLD));
	} catch (...) {
		throw APS2_SOCKET_FAILURE;
	}

	//Bind UDP socket at local port bb4f
	try {
		udp_socket_.open(udp::v4());
		udp_socket_.bind(udp::endpoint(udp::v4(), UDP_PORT));
	} catch (...) {
		throw APS2_SOCKET_FAILURE;
	}

	//enable broadcasting for enumerating
	udp_socket_old_.set_option(asio::socket_base::broadcast(true));
	udp_socket_.set_option(asio::socket_base::broadcast(true));

	setup_udp_receive(udp_socket_old_, received_udp_data_old_, remote_udp_endpoint_old_);
	setup_udp_receive(udp_socket_, received_udp_data_, remote_udp_endpoint_);

};

APS2Ethernet::~APS2Ethernet() {
	FILE_LOG(logDEBUG) << "Cleaning up ethernet interface";
	ios_.stop();
	receiveThread_.join();
}

void APS2Ethernet::setup_udp_receive(udp::socket & sock, uint8_t* buf, udp::endpoint & remote_endpoint) {
	//Receive UDP packets and pass off to sorter
	sock.async_receive_from(
		asio::buffer(buf, 2048), remote_endpoint,
		[this, &sock, buf, &remote_endpoint](std::error_code ec, std::size_t bytesReceived)
		{
			//If there is anything to look at hand it off to the sorter
			if (!ec && bytesReceived > 0)
			{
				vector<uint8_t> packetData(buf, buf + bytesReceived);
				sorter_lock_.lock();
				sort_packet(packetData, remote_endpoint);
				sorter_lock_.unlock();
			}

			//Start the receiver again
			setup_udp_receive(sock, buf, remote_endpoint);
	});
}

void APS2Ethernet::sort_packet(const vector<uint8_t> & packetData, const udp::endpoint & sender) {
	//If we have the endpoint address then add it to the queue otherwise check to see if it is an enumerate response
	string senderIP = sender.address().to_string();
	if (msgQueues_.find(senderIP) == msgQueues_.end()) {
		//Are seeing an enumerate status response
		//Old UDP port sends status response
		if ((sender.port() == 0xbb4e) && (packetData.size() == 84)) {
			devInfo_[senderIP].endpoint = sender;
			//Turn the byte array into a packet to extract the MAC address and firmware version
			//MAC not strictly necessary as we could just use the broadcast MAC address
			APS2EthernetPacket packet = APS2EthernetPacket(packetData);
			devInfo_[senderIP].macAddr = packet.header.src;
			APSStatusBank_t statusRegs;
			std::copy(packet.payload.begin(), packet.payload.end(), statusRegs.array);
			FILE_LOG(logDEBUG1) << "Adding device info for IP " << senderIP <<
				" ; MAC addresss " << devInfo_[senderIP].macAddr.to_string() <<
				" ; firmware version " << hexn<4> << statusRegs.userFirmwareVersion;
		}
		//New UDP port sends "I am an APS2"
		else if ((sender.port() == 0xbb4f) && (packetData.size() == 12)) {
		  string response = string(packetData.begin(), packetData.end());
		  FILE_LOG(logDEBUG2) << "Enumerate response string " << response;
		  if (response.compare("I am an APS2") == 0) {
				devInfo_[senderIP] = EthernetDevInfo();
				devInfo_[senderIP].supports_tcp = true;
				FILE_LOG(logDEBUG1) << "Adding device info for IP " << senderIP;
		  }
		}
	}
	else {
		//Turn the byte array into an APS2EthernetPacket
		FILE_LOG(logDEBUG4) << "Recevied UDP packet to be sorted from IP " << senderIP;
		APS2EthernetPacket packet = APS2EthernetPacket(packetData);
		//Grab a lock and push the packet into the message queue
		msgQueue_lock_.lock();
		msgQueues_[senderIP].emplace(packet);
		msgQueue_lock_.unlock();
	}
}

/* PUBLIC methods */

void APS2Ethernet::init() {
	reset_maps();
}

vector<std::pair<string,string>> APS2Ethernet::get_local_IPs() {

	FILE_LOG(logDEBUG) << "APS2Ethernet::get_local_IPs";

	vector<std::pair<string,string>> IPs;

	#ifdef _WIN32

	DWORD dwRetVal = 0;
	ULONG family = AF_INET; // we only care about IPv4 for now
	LONG flags = GAA_FLAG_INCLUDE_PREFIX; // not sure what the address prefix is
	PIP_ADAPTER_ADDRESSES pAddresses = nullptr;
	PIP_ADAPTER_ADDRESSES pCurAddresses = nullptr;
	PIP_ADAPTER_UNICAST_ADDRESS pUnicast = nullptr;

	ULONG outBufLen = 15000; //MSDN recommends preallocating 15KB
	pAddresses = (IP_ADAPTER_ADDRESSES *) malloc(outBufLen);
	dwRetVal = GetAdaptersAddresses(family, flags, nullptr, pAddresses, &outBufLen);

	if (dwRetVal == NO_ERROR) {
		for (pCurAddresses = pAddresses; pCurAddresses != nullptr; pCurAddresses = pCurAddresses->Next) {
			FILE_LOG(logDEBUG1) << "Found IPv4 interface.";
			FILE_LOG(logDEBUG3) << "IfIndex (IPv4 interface): " << pCurAddresses->IfIndex;
			FILE_LOG(logDEBUG2) << "Adapter name: " << pCurAddresses->AdapterName;
			FILE_LOG(logDEBUG2) << "Friendly adapter name: " << pCurAddresses->FriendlyName; //TODO figure out unicode printing

			//TODO: since we only support 1GigE should check that here
			FILE_LOG(logDEBUG2) << "Transmit link speed: " << pCurAddresses->TransmitLinkSpeed;
			FILE_LOG(logDEBUG2) << "Receive link speed: " << pCurAddresses->ReceiveLinkSpeed;

			for (pUnicast = pCurAddresses->FirstUnicastAddress; pUnicast != nullptr; pUnicast = pUnicast->Next) {
				char IPV4Addr[16]; //should be LPTSTR; 16 is maximum length of null terminated xxx.xxx.xxx.xxx
				DWORD addrSize = 16;
				int val;
				val = WSAAddressToString(pUnicast->Address.lpSockaddr, pUnicast->Address.iSockaddrLength, nullptr, IPV4Addr, &addrSize);
				if ( val != 0) {
				  val = WSAGetLastError();
				  FILE_LOG(logERROR) << "WSAAddressToString error code: " << val;
				  continue;
				}
				//Create a sock_addr for the netmask string prefix to string conversion
				struct sockaddr_in sin;
				memset(&sin, 0, sizeof(sin));
				sin.sin_family = AF_INET;
				sin.sin_addr.s_addr = htonl(0xffffffff << (32 - pUnicast->OnLinkPrefixLength));

				IPs.push_back(std::pair<string,string>(IPV4Addr, inet_ntoa(sin.sin_addr)));
				FILE_LOG(logDEBUG1) << "IPv4 address: " << IPs.back().first <<
										"; Prefix length: " << (unsigned) pUnicast->OnLinkPrefixLength <<
										"; Netmask: " << IPs.back().second;
			}
		}
		free(pAddresses);
		pAddresses = nullptr;
	}
	else {
		FILE_LOG(logERROR) << "Call to GetAdaptersAddresses failed.";
		free(pAddresses);
		pAddresses = nullptr;
	}

	#else

	struct ifaddrs *ifap, *ifa;

	getifaddrs(&ifap);
	for (ifa = ifap; ifa != nullptr; ifa = ifa->ifa_next) {
		//We only care about IPv4 addresses
		if ( ifa->ifa_addr == nullptr) continue; //According to Linux Programmers Manual "This field may contain a null pointer."
		if ( ifa->ifa_addr->sa_family == AF_INET ) {
			struct sockaddr_in* sa = (struct sockaddr_in *) ifa->ifa_addr;
			string addr = string(inet_ntoa(sa->sin_addr));
			string netmask = "255.255.255.255";
			if (ifa->ifa_netmask != nullptr) {
				sa = (struct sockaddr_in *) ifa->ifa_netmask;
				netmask = string(inet_ntoa(sa->sin_addr));
			}
			IPs.push_back(std::pair<string,string>(addr, netmask));
			FILE_LOG(logDEBUG1) << "Found network interface: " << ifa->ifa_name << "; Address: " << IPs.back().first << "; Netmask: " << IPs.back().second;
		}
	}
	freeifaddrs(ifap);
	#endif //_WIN32

	return IPs;
}

set<string> APS2Ethernet::enumerate() {
	/*
	 * Look for all APS units that respond to the broadcast packet
	 */

	FILE_LOG(logDEBUG) << "APS2Ethernet::enumerate";

	reset_maps();

	vector<std::pair<string,string>> localIPs = get_local_IPs();

	for (auto IP : localIPs) {
		//Skip the loop-back
		if ( IP.first.compare("127.0.0.1") == 0 ) continue;

		//Make sure it is a valid IP
		typedef asio::ip::address_v4 addrv4;
		asio::error_code ec;
		addrv4::from_string(IP.first, ec);
		if (ec) {
			FILE_LOG(logERROR) << "Invalid IP address: " << ec.message();
			continue;
		}
		//Put together the broadcast status request
		APS2EthernetPacket broadcastPacket = APS2EthernetPacket::create_broadcast_packet();
		addrv4 broadcastAddr = addrv4::broadcast(addrv4::from_string(IP.first), addrv4::from_string(IP.second));
		FILE_LOG(logDEBUG1) << "Sending enumerate broadcasts out on: " << broadcastAddr.to_string();
		//Try the old port
		udp::endpoint broadCastEndPoint(broadcastAddr, UDP_PORT_OLD);
		udp_socket_old_.send_to(asio::buffer(broadcastPacket.serialize()), broadCastEndPoint);
		//And the new
		broadCastEndPoint.port(UDP_PORT);
		uint8_t enumerate_request = 0x01;
		udp_socket_.send_to(asio::buffer(&enumerate_request, 1), broadCastEndPoint);
	}

	//Wait 100ms for response
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	set<string> deviceSerials;
	for (auto kv : devInfo_) {
		FILE_LOG(logINFO) << "Found device: " << kv.first << " with" << (kv.second.supports_tcp ? "" : "out") << " TCP support";
		deviceSerials.insert(kv.first);
	}
	return deviceSerials;
}

void APS2Ethernet::reset_maps() {
	devInfo_.clear();
	msgQueues_.clear();
}

void APS2Ethernet::connect(string serial) {
	FILE_LOG(logDEBUG) << "APS2Ethernet::connect";

	//Check whether we have device info and if not send a ping
	if (devInfo_.find(serial) == devInfo_.end()) {
		FILE_LOG(logDEBUG) << "No device info for " << serial << " ; sending enumerate request";

		//Make sure it is a valid IP
		typedef asio::ip::address_v4 addrv4;
		asio::error_code ec;
		addrv4 ip_addr = addrv4::from_string(serial, ec);
		if (ec) {
			FILE_LOG(logERROR) << "Invalid IP address: " << ec.message();
			throw APS2_INVALID_IP_ADDR;
		}
		//Try the old port
		//Put together the status request for old firmware devices
		APS2EthernetPacket broadcastPacket = APS2EthernetPacket::create_broadcast_packet();
		udp::endpoint endpoint(ip_addr, UDP_PORT_OLD);
		udp_socket_old_.send_to(asio::buffer(broadcastPacket.serialize()), endpoint);
		//And the new
		endpoint.port(UDP_PORT);
		uint8_t enumerate_request = 0x01;
		udp_socket_.send_to(asio::buffer(&enumerate_request, 1), endpoint);

		//Wait 100ms for response
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		//Check again
		if (devInfo_.find(serial) == devInfo_.end()) {
			FILE_LOG(logERROR) << "APS2 failed to respond at " << serial;
			throw APS2_NO_DEVICE_FOUND;
		}
  }

  if (devInfo_[serial].supports_tcp) {
		// C++14
		// tcp_sockets_.insert(serial, std::make_unique<tcp::socket>(ios_));
		// lowly C++11
		std::unique_ptr<tcp::socket> sock(new tcp::socket(ios_));
		FILE_LOG(logDEBUG1) << "Trying to connect to TCP port";
		std::future<void> connect_result = sock->async_connect(tcp::endpoint(asio::ip::address_v4::from_string(serial), TCP_PORT), asio::use_future);
		if ( connect_result.wait_for(COMMS_TIMEOUT) == std::future_status::timeout) {
			FILE_LOG(logERROR) << "Timed out trying to connect to " << serial;
			throw APS2_FAILED_TO_CONNECT;
		}
		try {
			connect_result.get();
		} catch(std::system_error e) {
			FILE_LOG(logERROR) << "Failed to connect to " << serial << " with error: " << e.what();
			throw APS2_FAILED_TO_CONNECT;
		}

		tcp_sockets_.insert(std::make_pair(serial, std::move(sock)));
	} else {
		msgQueue_lock_.lock();
		msgQueues_[serial] = queue<APS2EthernetPacket>();
		msgQueue_lock_.unlock();
	}
}

void APS2Ethernet::disconnect(string serial) {
  if (devInfo_[serial].supports_tcp) {
	if (tcp_sockets_.find(serial) != tcp_sockets_.end()) {
		tcp_sockets_[serial]->cancel();
		tcp_sockets_[serial]->close();
		tcp_sockets_.erase(serial);
	}
  } else {
	msgQueue_lock_.lock();
	msgQueues_.erase(serial);
	msgQueue_lock_.unlock();
  }
}

void APS2Ethernet::reset_tcp(const string & ip_addr_str) {
	//Make sure it is a valid IP
	typedef asio::ip::address_v4 addrv4;
	asio::error_code ec;
	addrv4 ip_addr = addrv4::from_string(ip_addr_str, ec);
	if (ec) {
		FILE_LOG(logERROR) << "Invalid IP address: " << ec.message();
		throw APS2_INVALID_IP_ADDR;
	}

	udp::endpoint endpoint(ip_addr, UDP_PORT);
	uint8_t reset_tcp_byte = 0x02;
	udp_socket_.send_to(asio::buffer(&reset_tcp_byte, 1), endpoint);
}

void APS2Ethernet::send(string ipAddr, const vector<APS2Datagram> & datagrams) {
	FILE_LOG(logDEBUG2) << "APS2Ethernet::send";
	if (devInfo_[ipAddr].supports_tcp) {
		FILE_LOG(logDEBUG2) << "Sending " << datagrams.size() << " datagram" << (datagrams.size() > 1 ? "s" : "") << " over TCP";
		// If we have TCP just send it out
		for (const auto & dg : datagrams){
			auto data = dg.data();
			for (auto & val : data) {
				val = htonl(val);
			}
			FILE_LOG(logDEBUG3) << "Sending datagram with command word " << hexn<8> << dg.cmd.packed <<
			" to address " << hexn<8> << dg.addr << " with payload size " << std::dec << dg.payload.size() << " for total size " << data.size();

			std::future<size_t> write_result = asio::async_write(*tcp_sockets_[ipAddr], asio::buffer(data), asio::use_future);
			if ( write_result.wait_for(COMMS_TIMEOUT) == std::future_status::timeout) {
				FILE_LOG(logERROR) << ipAddr << " write timed out";
				throw APS2_COMMS_ERROR;
			}
			try {
				size_t bytes_written = write_result.get();
				FILE_LOG(logDEBUG3) << "Wrote " << bytes_written << " bytes";
			} catch(std::system_error e) {
				FILE_LOG(logERROR) << ipAddr << " write errored with message: " << e.what();
				throw APS2_COMMS_ERROR;
			}
		}
		//Block until the acks come back
		for (const auto & dg : datagrams){
		  if ( dg.cmd.ack ) {
				auto timeout = COMMS_TIMEOUT;
				auto ack = read(ipAddr, timeout);
				dg.check_ack(ack, false);
		  }
		}
	} else {
		FILE_LOG(logDEBUG2) << "Sending " << datagrams.size() << " datagram" << (datagrams.size() > 1 ? "s" : "") << " over UDP";
		//Without TCP convert to APS2EthernetPacket packets and send
		for (auto dg : datagrams) {
			auto packets = APS2EthernetPacket::chunk(dg.addr, dg.payload, dg.cmd);
			//Set the ack every to a maximum of 20
			size_t ack_every;
			//For read commands don't let the ack check eat the response packet and copy in count
			if (dg.cmd.r_w) {
				ack_every = 0;
				//Should only be one packet for read requests
				packets[0].header.command.r_w = 1;
				packets[0].header.command.cnt = dg.cmd.cnt;
				send(ipAddr, packets[0], false);
				} else {
				ack_every = packets.size() >= 20 ? 20 : packets.size();
				send(ipAddr, packets, ack_every);
			}
		}
	}
}

int APS2Ethernet::send(string serial, APS2EthernetPacket msg, bool checkResponse) {
	msg.header.dest = devInfo_[serial].macAddr;
	send_chunk(serial, vector<APS2EthernetPacket>(1, msg), !checkResponse);
	return 0;
}

int APS2Ethernet::send(string serial, vector<APS2EthernetPacket> msg, unsigned ackEvery /* see header for default */) {
	FILE_LOG(logDEBUG2) << "APS2Ethernet::send";
	FILE_LOG(logDEBUG3) << "Sending " << msg.size() << " packets to " << serial;
	auto iter = msg.begin();
	bool noACK = false;
	if (ackEvery == 0) {
		noACK = true;
		ackEvery = 1;
	}
	// it's nice to have extra status on slow EPROM writes
	bool verbose = (msg[0].header.command.cmd == static_cast<uint32_t>(APS_COMMANDS::EPROMIO));

	vector<APS2EthernetPacket> buffer(ackEvery);

	while (iter != msg.end()) {

		//Copy the next chunk into a buffer
		auto endPoint = iter + ackEvery;
		if (endPoint > msg.end()) {
			endPoint = msg.end();
		}
		auto chunkSize = std::distance(iter, endPoint);
		buffer.resize(chunkSize);
		std::copy(iter, endPoint, buffer.begin());

		for (auto & packet : buffer ) {
			// insert the target MAC address - not really necessary anymore because UDP does filtering
			packet.header.dest = devInfo_[serial].macAddr;
			//NOACK sets the top bit of the command nibble of the command word
			packet.header.command.cmd |= (1 << 3);
		}

		//Apply acknowledge flag to last element of chunk
		if (!noACK) {
			buffer.back().header.command.cmd &= ~(1 << 3);
		}

		send_chunk(serial, buffer, noACK);
		std::advance(iter, chunkSize);

		if (verbose && (std::distance(msg.begin(), iter) % 1000 == 0)) {
			FILE_LOG(logDEBUG) << "Write " << 100*std::distance(msg.begin(), iter)/msg.size() << "% complete";
		}
	}
	return 0;
}

void APS2Ethernet::send_chunk(string serial, vector<APS2EthernetPacket> chunk, bool noACK) {
	FILE_LOG(logDEBUG2) << "APS2Ethernet::send_chunk";

	unsigned seqNum{0};

	seqNum = 0;
	for (auto packet : chunk) {
		packet.header.seqNum = seqNum;
		seqNum++;
		FILE_LOG(logDEBUG4) << "Packet command: " << packet.header.command.to_string();
		udp_socket_old_.send_to(asio::buffer(packet.serialize()), devInfo_[serial].endpoint);
		// sleep to make the driver compatible with newer versions of Windows
		// std::this_thread::sleep_for(std::chrono::microseconds(100));
	}

	if (noACK) return;

	//Wait for acknowledge from final packet in chunk and error check
	auto ack = read(serial, COMMS_TIMEOUT);
	APS2Datagram dg;
	dg.cmd.packed = chunk.back().header.command.packed;
	dg.addr = chunk.back().header.addr;
	dg.check_ack(ack, true);
}

APS2Datagram APS2Ethernet::read(string ipAddr, std::chrono::milliseconds timeout) {
	FILE_LOG(logDEBUG2) << "APS2Ethernet::read";
	if ( devInfo_[ipAddr].supports_tcp) {
		//Read datagram from socket
		vector<uint32_t> buf;

		auto read_with_timeout = [&]() {
		  std::future<size_t> read_result = tcp_sockets_[ipAddr]->async_receive(asio::buffer(buf), asio::use_future);
		  if (read_result.wait_for(timeout) == std::future_status::timeout) {
				FILE_LOG(logERROR) << "TCP receive timed out!";
				throw APS2_RECEIVE_TIMEOUT;
			}
		};

		  //First read header (cmd and addr)
		buf.resize(1);
		read_with_timeout();
		APS2Command cmd;
		cmd.packed = ntohl(buf[0]);
		uint32_t addr{0};
		if (!(
		  (APS_COMMANDS(cmd.cmd) == APS_COMMANDS::STATUS) ||   //Status register response has no address
		  (APS_COMMANDS(cmd.cmd) == APS_COMMANDS::FPGACONFIG_ACK) || //configuration SDRAM write/reads have no adddress
		  (APS_COMMANDS(cmd.cmd) == APS_COMMANDS::EPROMIO) || //configuration EPROM write/reads have no adddress
		  (APS_COMMANDS(cmd.cmd) == APS_COMMANDS::CHIPCONFIGIO) //chip config SPI commands have no address (built into command words)
		  )) {
		  read_with_timeout();
		  addr = ntohl(buf[0]);
		}
		//If it is a write ack then the count doesn't matter
		if ( cmd.ack && !cmd.r_w ) {
			buf.clear();
		} else {
			buf.resize(cmd.cnt);
			read_with_timeout();
			for (auto & val : buf) {
			val = ntohl(val);
		}
		}
		FILE_LOG(logDEBUG3) << "Read APS2Datagram " << hexn<8> << cmd.packed << " " << hexn<8> << addr << " and payload length " << std::dec << buf.size();
		return {cmd, addr, buf};

	} else {
	//The packets should already be in the queue
	auto pkt = receive(ipAddr, 1, timeout.count()).front();
	//strip off the ethernet header
	APS2Command cmd;
	cmd.packed = pkt.header.command.packed;
	FILE_LOG(logDEBUG3) << "Read APS2Datagram " << hexn<8> << cmd.packed << " " << hexn<8> << pkt.header.addr << " and payload length " << std::dec << pkt.payload.size();
	return {cmd, pkt.header.addr, pkt.payload};
	}
}

vector<APS2EthernetPacket> APS2Ethernet::receive(string serial, size_t numPackets, size_t timeoutMS) {
	//Read the packets coming back in up to the timeout
	//Defaults: receive(string serial, size_t numPackets = 1, size_t timeoutMS = 1000);
	FILE_LOG(logDEBUG2) << "APS2Ethernet::receive";
	std::chrono::time_point<std::chrono::steady_clock> start, end;

	start = std::chrono::steady_clock::now();
	size_t elapsedTime = 0;

	vector<APS2EthernetPacket> outVec;

	while (elapsedTime < timeoutMS) {
		if (!msgQueues_[serial].empty()) {
			msgQueue_lock_.lock();
			outVec.push_back(msgQueues_[serial].front());
			msgQueues_[serial].pop();
			msgQueue_lock_.unlock();
			FILE_LOG(logDEBUG4) << "Received packet command: " << outVec.back().header.command.to_string();
			if (outVec.size() == numPackets) {
				FILE_LOG(logDEBUG3) << "Received " << numPackets << " packets from " << serial;
				return outVec;
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		end = std::chrono::steady_clock::now();
		elapsedTime =  std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
	}

	throw APS2_RECEIVE_TIMEOUT;
}
