// Helper class to handle disconnecting from APS on cleanup
//
// Original author: Colm Ryan
// Copyright 2016 Raytheon BBN Technologies

#ifndef APS2Connector_H
#define APS2Connector_H

#include <string>
using std::string;

class APS2Connector {
public:
	
	APS2Connector(string);
	~APS2Connector();

private:
	string ip_addr_;

};

#endif /* end of include guard: APS2Connector_H */
