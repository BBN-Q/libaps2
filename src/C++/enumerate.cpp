//Lists APS2 boards found on network
//
// Original author: Colm Ryan
// Copyright 2016, Raytheon BBN Technologies

#include "libaps2.h"
#include "concol.h"
#include "helpers.h"

#include <chrono>

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

  for (unsigned ct=0; ct < numDevices; ct++) {
		connect_APS(serialBuffer[ct]);
		uint32_t firmware_version{0};
		get_firmware_version(serialBuffer[ct], &firmware_version);
		double uptime{0};
		get_uptime(serialBuffer[ct], &uptime);
		std::chrono::duration<float> uptime_seconds(uptime);
		auto uptime_days = std::chrono::duration_cast<std::chrono::duration<int, std::ratio<24*3600>>>(uptime_seconds);

		std::ostringstream uptime_pretty;
		if (uptime_days.count()) {
			uptime_pretty << uptime_days.count() << " day" << (uptime_days.count() > 1 ? "s " : " ");
		}
		uptime_seconds -= uptime_days;
		auto uptime_hours = std::chrono::duration_cast<std::chrono::hours>(uptime_seconds);
		if (uptime_hours.count()) {
			uptime_pretty << uptime_hours.count() << " hour" << (uptime_hours.count() > 1 ? "s " : " ");
		}
		uptime_seconds -= uptime_hours;
		auto uptime_minutes = std::chrono::duration_cast<std::chrono::minutes>(uptime_seconds);
		if (uptime_minutes.count()) {
			uptime_pretty << uptime_minutes.count() << " minute" << (uptime_minutes.count() > 1 ? "s " : " ");
		}
		uptime_seconds -= uptime_minutes;
		uptime_pretty << uptime_seconds.count() << " seconds";

    cout << concol::CYAN << "Device " << ct << " at IPv4 address " << serialBuffer[ct] <<
		" running firmware version " << print_firmware_version(firmware_version) <<
		" has been up " << uptime_pretty.str() << concol::RESET << endl;
		disconnect_APS(serialBuffer[ct]);
  }
	cout << endl;

  return 0;
}
