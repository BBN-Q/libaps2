// Ethernet communications with APS2 boards
//
// Original authors: Colm Ryan, Blake Johnson, Brian Donovan
// Copyright 2016, Raytheon BBN Technologies

#ifndef APS2ETHERNET_H
#define APS2ETHERNET_H

#include <chrono>
#include <future>
#include <memory>
#include <unordered_map>
using std::unordered_map;
#include <queue>
using std::queue;

#include "asio.hpp"
#include <asio/use_future.hpp>
using asio::ip::udp;
using asio::ip::tcp;

#include "APS2Datagram.h"
#include "APS2EthernetPacket.h"
#include "APS2_errno.h"
#include "MACAddr.h"

struct EthernetDevInfo {
  MACAddr macAddr;
  udp::endpoint endpoint;
  uint16_t seqNum = 0;
  bool supports_tcp = false;
};

class APS2Ethernet {
public:
  APS2Ethernet &operator=(APS2Ethernet &rhs) { return rhs; };

  APS2Ethernet();
  ~APS2Ethernet();
  void init();
  set<string> enumerate();
  void connect(string serial);
  void disconnect(string serial);
  void reset_tcp(const string &);
  void send(string, const vector<APS2Datagram> &);
  int send(string serial, APS2EthernetPacket msg, bool checkResponse = true);
  int send(string serial, vector<APS2EthernetPacket> msg,
           unsigned ackEvery = 1);

  APS2Datagram read(string, std::chrono::milliseconds);
  vector<APS2EthernetPacket> receive(string serial, size_t numPackets = 1,
                                     size_t timeoutMS = 2000);

private:
  APS2Ethernet(APS2Ethernet const &) = delete;

  MACAddr srcMAC_;

  // Keep track of all the device info with a map from I.P. addresses to devInfo
  // structs
  unordered_map<string, EthernetDevInfo> devInfo_;

  unordered_map<string, queue<APS2EthernetPacket>> msgQueues_;

  vector<std::pair<string, string>> get_local_IPs();

  void reset_maps();

  // ASIO service and sockets
  asio::io_service ios_;
  udp::socket udp_socket_old_;
  udp::socket udp_socket_;
  unordered_map<string, std::unique_ptr<tcp::socket>> tcp_sockets_;

  // storage for received UDP packets and remote endpoints
  uint8_t received_udp_data_old_[2048];
  uint8_t received_udp_data_[2048];
  udp::endpoint remote_udp_endpoint_old_;
  udp::endpoint remote_udp_endpoint_;

  void setup_udp_receive(udp::socket &, uint8_t *, udp::endpoint &);

  void sort_packet(const vector<uint8_t> &, const udp::endpoint &);

  void send_chunk(string, vector<APS2EthernetPacket>, bool);

  std::thread receiveThread_;
  std::mutex msgQueue_lock_;
  std::mutex sorter_lock_;
};

#endif
