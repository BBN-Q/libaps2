#include <iostream>
#include <fstream>
#include <sstream>


#include "concol.h"

using std::cout;
using std::cin;
using std::endl;
using std::string;

string get_device_id() {
  /*
  Helper function than enumerates and asks for which APS to talk to.
  */
  cout << concol::CYAN << "Enumerating devices" << concol::RESET << endl;

  unsigned numDevices = 0;
  get_numDevices(&numDevices);

  cout << concol::CYAN << numDevices << " APS device" << (numDevices > 1 ? "s": "")  << " found" << concol::RESET << endl;

  if (numDevices < 1)
    return "";

  const char ** serialBuffer = new const char*[numDevices];
  get_deviceSerials(serialBuffer);

  for (unsigned cnt=0; cnt < numDevices; cnt++) {
    cout << concol::CYAN << "Device " << cnt << " serial #: " << serialBuffer[cnt] << concol::RESET << endl;
  }

  string deviceSerial;

  if (numDevices == 1) {
    deviceSerial = string(serialBuffer[0]);
  }
  else {

    cout << concol::YELLOW << "Choose device ID [0]: " << concol::RESET;
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


void print_title(const string & title){
  concol::concolinit();
  cout << endl;
  cout << concol::BOLDMAGENTA << title << concol::RESET << endl;
  cout << endl;
}
