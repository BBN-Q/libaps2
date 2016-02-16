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

	uint32_t minor = version_reg & 0xff;
	uint32_t major = (version_reg >> 8) & 0xf;
	uint32_t sha1_nibble = (version_reg >> 12) & 0xfffff;
	string note;
	switch (sha1_nibble) {
		case 0x00000:
			note = "";
			break;
		case 0x0000a:
			note = "-dev";
			break;
		default:
			std::ostringstream sha1;
			sha1 << "-dev-" << std::hex << sha1_nibble;
			note = sha1.str();
			break;
	}
	ret << major << "." << minor << note;
	return ret.str();
}
