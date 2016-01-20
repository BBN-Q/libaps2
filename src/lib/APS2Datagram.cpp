// Datagrams that are sent/received from the APS2
// Consists of a vector of 32 bit words
// 1. command word - see APSCommand_t
// 2. address
// 3. payload
//
// Original author: Colm Ryan
// Copyright 2016 Raytheon BBN Technologi

#include "APS2Datagram.h"

vector<APS2Datagram> APS2Datagram::chunk(unsigned chunk_size) {
  //Break a datagram up into muliple datagrams of a maximum payload size
  vector<APS2Datagram> chunks;
  auto start_it = payload.begin();
  while (start_it != payload.end()) {
    auto end_it = (std::distance(start_it, payload.end()) > chunk_size) ? start_it + chunk_size : payload.end();
    chunks.push_back(APS2Datagram{cmd, addr, vector<uint32_t>(start_it, end_it)});
    std::advance(start_it, std::distance(start_it, end_it));
  }
  return chunks;
}
