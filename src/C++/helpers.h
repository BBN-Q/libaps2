#include <iostream>
#include <fstream>
#include <sstream>

#include "concol.h"

using std::cout;
using std::cin;
using std::endl;
using std::string;

string get_device_id() {
  
  cout << concol::RED << "Enumerating devices" << concol::RESET << endl;

  int numDevices = get_numDevices();

  cout << concol::RED << numDevices << " APS device" << (numDevices > 1 ? "s": "")  << " found" << concol::RESET << endl;

  if (numDevices < 1)
    return 0;
  
  cout << concol::RED << "Attempting to get serials" << concol::RESET << endl;

  const char ** serialBuffer = new const char*[numDevices];
  get_deviceSerials(serialBuffer);

  for (int cnt=0; cnt < numDevices; cnt++) {
    cout << concol::RED << "Device " << cnt << " serial #: " << serialBuffer[cnt] << concol::RESET << endl;
  }

  string deviceSerial;

  if (numDevices == 1) {
    deviceSerial = string(serialBuffer[0]);
  }
  else {

    cout << "Choose device ID [0]: ";
    string input = "";
    getline(cin, input);

    int device_id = 0;
    if (input.length() != 0) {
      std::stringstream mystream(input);
      mystream >> device_id;
    }
    deviceSerial = string(serialBuffer[device_id]);
  }

  delete[] serialBuffer;
  return deviceSerial;

}