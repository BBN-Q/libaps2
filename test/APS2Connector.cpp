// Helper class to handle disconnecting from APS on cleanup
//
// Original author: Colm Ryan
// Copyright 2016 Raytheon BBN Technologies
#include "APS2Connector.h"
#include "libaps2.h"

APS2Connector::APS2Connector(string ip_addr) {
  ip_addr_ = ip_addr;
  connect_APS(ip_addr_.c_str());
}

APS2Connector::~APS2Connector() { disconnect_APS(ip_addr_.c_str()); }
