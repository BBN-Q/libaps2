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

	cout << concol::CYAN << numDevices << " APS device" << (numDevices > 1 ? "s": "")	<< " found" << concol::RESET << endl;

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

//Copy of APS2::print_firmware_version
string print_firmware_version(uint32_t version_reg) {
	std::ostringstream ret;

	uint32_t minor{version_reg & 0xff};
	uint32_t major{(version_reg >> 8) & 0xf};
	uint32_t note_nibble{(version_reg >> 12) & 0xf};
	string note;
	switch (note_nibble) {
		case 0xa:
			note = "alpha";
			break;
		case 0xb:
			note = "beta";
			break;
	}
	ret << major << "." << minor << " " << note;
	return ret.str();
}
