//Lists APS2 boards found on network
//
// Original author: Colm Ryan
// Copyright 2016, Raytheon BBN Technologies

#include "libaps2.h"
#include "concol.h"
#include "helpers.h"

int main(int argc, char* argv[])
{

	print_title("BBN APS2 Enumerate Utility");

	set_logging_level(logDEBUG1);

	//First get the number of devices we can see
  unsigned numDevices = 0;
  get_numDevices(&numDevices);
  cout << concol::CYAN << numDevices << " APS device" << (numDevices > 1 ? "s": "")  << " found" << concol::RESET << endl;
	cout << endl;

	//If we don't see anything bail
	if (numDevices < 1) return 0;

	//Get IP strings and firmware versions
  const char ** serialBuffer = new const char*[numDevices];
  get_deviceSerials(serialBuffer);

  for (unsigned cnt=0; cnt < numDevices; cnt++) {
    cout << concol::CYAN << "Device " << cnt << " IPv4 address: " << serialBuffer[cnt] << concol::RESET << endl;
  }
	cout << endl;

  return 0;
}
