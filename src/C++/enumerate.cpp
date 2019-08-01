// Lists APS2 boards found on network
//
// Original author: Colm Ryan
// Copyright 2016, Raytheon BBN Technologies

#include "concol.h"
#include "helpers.h"
#include "libaps2.h"

#include <iomanip>
#include <iostream>
#include <plog/Log.h>

#include <chrono>

int main(int argc, char *argv[]) {

  print_title("BBN APS2 Enumerate Utility");

  set_file_logging_level(plog::debug);
  set_console_logging_level(plog::info);

  // First get the number of devices we can see
  unsigned numDevices = 0;
  get_numDevices(&numDevices);
  cout << concol::CYAN << numDevices << " APS device"
       << (numDevices > 1 ? "s" : "") << " found" << concol::RESET << endl;
  cout << endl;

  // If we don't see anything bail
  if (numDevices < 1)
    return 0;

  // Get IP strings and firmware versions
  const char **serialBuffer = new const char *[numDevices];
  get_device_IPs(serialBuffer);

  for (unsigned ct = 0; ct < numDevices; ct++) {
    APS2_STATUS status;
    status = connect_APS(serialBuffer[ct]);
    if (status != APS2_OK) {
      cout << concol::RED << "Failed to connect to " << serialBuffer[ct]
           << concol::RESET << endl;
      continue;
    }

    char version_string[64];
    get_firmware_version(serialBuffer[ct], nullptr, nullptr, nullptr,
                         version_string);
    double uptime{0};
    get_uptime(serialBuffer[ct], &uptime);
    std::chrono::duration<float> uptime_seconds(uptime);
    auto uptime_days = std::chrono::duration_cast<
        std::chrono::duration<int, std::ratio<24 * 3600>>>(uptime_seconds);

    std::ostringstream uptime_pretty;
    if (uptime_days.count()) {
      uptime_pretty << uptime_days.count() << " day"
                    << (uptime_days.count() > 1 ? "s " : " ");
    }
    uptime_seconds -= uptime_days;
    auto uptime_hours =
        std::chrono::duration_cast<std::chrono::hours>(uptime_seconds);
    uptime_pretty << std::setfill('0') << std::setw(2) << uptime_hours.count()
                  << ":";
    uptime_seconds -= uptime_hours;
    auto uptime_minutes =
        std::chrono::duration_cast<std::chrono::minutes>(uptime_seconds);
    uptime_pretty << std::setfill('0') << std::setw(2) << uptime_minutes.count()
                  << ":";
    uptime_seconds -= uptime_minutes;
    uptime_pretty << std::fixed << std::setprecision(3)
                  << uptime_seconds.count();

    cout << concol::CYAN << "Device " << ct + 1 << " at IPv4 address "
         << serialBuffer[ct] << " running firmware version " << version_string
         << " has been up " << uptime_pretty.str() << concol::RESET << endl;
    disconnect_APS(serialBuffer[ct]);
  }
  cout << endl;

  return 0;
}
