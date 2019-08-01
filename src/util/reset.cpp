#include <iostream>
#include <plog/Log.h>

#include "libaps2.h"

#include "../C++/helpers.h"
#include "../C++/optionparser.h"

#include <concol.h>

using std::cout;
using std::endl;

enum optionIndex { UNKNOWN, HELP, IP_ADDR, RESET_MODE, LOG_LEVEL };
const option::Descriptor usage[] = {
    {UNKNOWN, 0, "", "", option::Arg::None, "USAGE: reset [options]\n\n"
                                            "Options:"},
    {HELP, 0, "", "help", option::Arg::None,
     "	--help	\tPrint usage and exit."},
    {IP_ADDR, 0, "", "ipAddr", option::Arg::NonEmpty,
     "	--ipAddr	\tIP address of unit to program (optional)."},
    {RESET_MODE, 0, "", "resetMode", option::Arg::NonEmpty,
     "	--resetMode	\tWhat type of reset to do (USER_IMAGE,BASE_IMAGE,TCP) "
     "(optional)."},
    {LOG_LEVEL, 0, "", "logLevel", option::Arg::Numeric,
     "	--logLevel	\t(optional) Logging level level to print (optional; "
     "default=3/DEBUG)."},

    {UNKNOWN, 0, "", "", option::Arg::None, "\nExamples:\n"
                                            "	flash --IP\n"
                                            "	flash --SPI\n"},
    {0, 0, 0, 0, 0, 0}};

APS2_RESET_MODE get_reset_mode() {
  cout << concol::RED << "Reset options:" << concol::RESET << endl;
  cout << "1) Reset to user image" << endl;
  cout << "2) Reset to backup image" << endl;
  cout << "3) Reset tcp connection" << endl << endl;
  cout << "Choose option [1]: ";

  char input;
  cin.get(input);
  switch (input) {
  case '1':
  default:
    return RECONFIG_EPROM_USER;
    break;
  case '2':
    return RECONFIG_EPROM_BASE;
    break;
  case '3':
    return RESET_TCP;
    break;
  }
}

int main(int argc, char *argv[]) {

  print_title("BBN APS2 Reset Utility");

  argc -= (argc > 0);
  argv += (argc > 0); // skip program name argv[0] if present
  option::Stats stats(usage, argc, argv);
  option::Option *options = new option::Option[stats.options_max];
  option::Option *buffer = new option::Option[stats.buffer_max];
  option::Parser parse(usage, argc, argv, options, buffer);

  if (parse.error())
    return -1;

  if (options[HELP]) {
    option::printUsage(std::cout, usage);
    return 0;
  }

  // Logging level
  plog::Severity logLevel = plog::debug;
  if (options[LOG_LEVEL]) {
    //TODO: Input validation??
    logLevel =  static_cast<plog::Severity>(atoi(options[LOG_LEVEL].arg));
  }
  set_file_logging_level(logLevel);
  set_console_logging_level(plog::info);

  string deviceSerial;
  if (options[IP_ADDR]) {
    deviceSerial = string(options[IP_ADDR].arg);
    cout << "Programming device " << deviceSerial << endl;
  } else {
    deviceSerial = get_device_id();
  }
  if ( deviceSerial.empty() ) {
    cout << concol::RED << "No APS2 devices connected! Exiting..."
         << concol::RESET << endl;
    return 0;
  }

  APS2_RESET_MODE mode;
  if (options[RESET_MODE]) {
    std::map<string, APS2_RESET_MODE> mode_map{
        {"USER_IMAGE", RECONFIG_EPROM_USER},
        {"BASE_IMAGE", RECONFIG_EPROM_BASE},
        {"TCP", RESET_TCP}};
    string mode_str(options[RESET_MODE].arg);
    if (mode_map.count(mode_str)) {
      mode = mode_map[mode_str];
    } else {
      std::cerr << concol::RED << "Unexpected reset mode" << concol::RESET
                << endl;
      return -1;
    }
  } else {
    mode = get_reset_mode();
  }

  if (mode != RESET_TCP) {
    connect_APS(deviceSerial.c_str());
  }

  reset(deviceSerial.c_str(), mode);

  if (mode != RESET_TCP) {
    disconnect_APS(deviceSerial.c_str());
  }

  cout << concol::RED << "Finished!" << concol::RESET << endl;

  return 0;
}
