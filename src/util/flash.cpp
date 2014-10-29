#include <iostream>
#include <sstream>
#include <iomanip>

#include "concol.h"
#include "libaps2.h"
#include "../C++/helpers.h"
#include "../C++/optionparser.h"

using std::cout;
using std::endl;

enum  optionIndex { UNKNOWN, HELP, IP_ADDR, MAC_ADDR, SPI, LOG_LEVEL};
const option::Descriptor usage[] =
{
  {UNKNOWN, 0,"" , ""    , option::Arg::None, "USAGE: flash [options]\n\n"
                                           "Options:" },
  {HELP,    0,"" , "help", option::Arg::None, "  --help  \tPrint usage and exit." },
  {IP_ADDR, 0,"", "IP", option::Arg::None, "  --IP  \tProgram a new IP address." },
  {MAC_ADDR, 0,"", "MAC", option::Arg::Required, "  --MAC  \tProgram a new MAC address." },
  {SPI,  0,"", "SPI", option::Arg::Numeric, "  --SPI  \tWrite the SPI startup sequence." },
  {LOG_LEVEL,  0,"", "logLevel", option::Arg::Numeric, "  --logLevel  \t(optional) Logging level level to print to console (optional; default=2/INFO)." },
  {UNKNOWN, 0,"" ,  ""   , option::Arg::None, "\nExamples:\n"
                                           "  flash --IP\n"
                                           "  flash --SPI\n" },
  {0,0,0,0,0,0}
};

// N-wide hex output with 0x
template <unsigned int N>
std::ostream& hexn(std::ostream& out)
{
    return out << "0x" << std::hex << std::setw(N) << std::setfill('0');
}

uint64_t get_mac_input() {
  cout << "New MAC address [ENTER to skip]: ";
  string input = "";
  getline(cin, input);

  if (input.length() == 0) {
    return 0;
  }
  std::stringstream mystream(input);
  uint64_t mac_addr;
  mystream >> std::hex >> mac_addr;

  cout << "Received " << hexn<12> << mac_addr << endl;
  return mac_addr;
}

string get_ip_input() {
  cout << "New IP address [ENTER to skip]: ";
  string input = "";
  getline(cin, input);
  return input;
}

bool spi_prompt() {
  cout << "Do you want to program the SPI startup sequence? [y/N]: ";
  string input = "";
  getline(cin, input);
  if (input.length() == 0) {
    return false;
  }
  std::stringstream mystream(input);
  char response;
  mystream >> response;
  switch (response) {
    case 'y':
    case 'Y':
      return true;
      break;
    case 'n':
    case 'N':
    default:
      return false;
  }
}


int main(int argc, char* argv[])
{
  argc-=(argc>0); argv+=(argc>0); // skip program name argv[0] if present
  option::Stats  stats(usage, argc, argv);
  option::Option *options = new option::Option[stats.options_max];
  option::Option *buffer = new option::Option[stats.buffer_max];
  option::Parser parse(usage, argc, argv, options, buffer);

  if (parse.error())
   return -1;

  if (options[HELP]) {
    option::printUsage(std::cout, usage);
    return 0;
  }

  bool interactiveMode = (argc == 0) ? true : false;

  for (option::Option* opt = options[UNKNOWN]; opt; opt = opt->next())
   std::cout << "Unknown option: " << opt->name << "\n";

  for (int i = 0; i < parse.nonOptionsCount(); ++i)
   std::cout << "Non-option #" << i << ": " << parse.nonOption(i) << "\n";

  concol::concolinit();
  cout << concol::RED << "BBN AP2 Flash Test Executable" << concol::RESET << endl;

  //Logging level
  TLogLevel logLevel = logINFO;
  if (options[LOG_LEVEL]) {
    logLevel = TLogLevel(atoi(options[LOG_LEVEL].arg));
  }
  set_log("stdout");
  set_logging_level(logLevel);

  string deviceSerial = get_device_id();
  if (deviceSerial.empty()){
    cout << concol::RED << "No APS2 devices connected! Exiting..." << concol::RESET << endl;
    return 0;
  }
  
  connect_APS(deviceSerial.c_str());

  if (options[MAC_ADDR] || options[IP_ADDR] || interactiveMode) {
    cout << "Programmed MAC and IP address at 0x00FF0000 are " << endl;
    cout << "MAC addr: " << hexn<12> << get_mac_addr(deviceSerial.c_str()) << endl;
    char curIP[16];
    get_ip_addr(deviceSerial.c_str(), curIP);
    cout << "IP addr: " << curIP << endl;
  }

  // write a new MAC address
  if (options[MAC_ADDR] || interactiveMode) {
    uint64_t mac_addr = get_mac_input();
    if (mac_addr != 0) {
      cout << concol::RED << "Writing new MAC address" << concol::RESET << endl;
      set_mac_addr(deviceSerial.c_str(), mac_addr);
    }
  }

  // write a new IP address
  if (options[IP_ADDR] || interactiveMode) {
    string ip_addr = get_ip_input();
    if (ip_addr != "") {
      cout << concol::RED << "Writing new IP address" << concol::RESET << endl;
      set_ip_addr(deviceSerial.c_str(), ip_addr.c_str());
    }
  }

  // read SPI setup sequence
  if (options[SPI] || interactiveMode) {
    uint32_t setup[36];
    read_flash(deviceSerial.c_str(), 0x0, 32, setup);
    cout << "Programmed setup SPI sequence:" << endl;
    for (size_t ct=0; ct < 32; ct++) {
      cout << hexn<8> << setup[ct] << " ";
      if (ct % 4 == 3) cout << endl;
    }

    bool runSPI = true;
    if (interactiveMode){
      runSPI = spi_prompt();
    }
    if (runSPI) {
      // write new SPI setup sequence
      cout << concol::RED << "Writing SPI startup sequence" << concol::RESET << endl;
      write_SPI_setup(deviceSerial.c_str());
    }
  }

  disconnect_APS(deviceSerial.c_str());

  cout << concol::RED << "Finished!" << concol::RESET << endl;

  return 0;
}
