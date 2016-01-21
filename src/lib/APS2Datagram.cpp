// Datagrams that are sent/received from the APS2
// Consists of a vector of 32 bit words
// 1. command word - see APSCommand_t
// 2. address
// 3. payload
//
// Original author: Colm Ryan
// Copyright 2016 Raytheon BBN Technologi

#include "APS2Datagram.h"

vector<APS2Datagram> APS2Datagram::chunk(APS2Command cmd, uint32_t addr, const vector<uint32_t>& data, uint16_t chunk_size) {
  //Create multiple datagrams of a maximum payload size
  vector<APS2Datagram> chunks;
  auto start_it = data.begin();
  while (start_it != data.end()) {
    auto end_it = (std::distance(start_it, data.end()) > chunk_size) ? start_it + chunk_size : data.end();
    cmd.cnt = chunk_size;
    chunks.push_back(APS2Datagram{cmd, addr, vector<uint32_t>(start_it, end_it)});
    std::advance(start_it, std::distance(start_it, end_it));
  }
  return chunks;
}

vector<uint32_t> APS2Datagram::data() {
  //Construct the datagram
  vector<uint32_t> data;
  data.reserve(2+payload.size());
  cmd.cnt = payload.size() & 0xffff; //TODO: what if payload is too large?
  data.push_back(cmd.packed);
  data.push_back(addr);
  std::copy(payload.begin(), payload.end(), std::back_inserter(data));
  return data;
}
