#include <iostream>
#include <sstream>
#include <iomanip>

#include "concol.h"
#include "libaps2.h"
#include "../C++/helpers.h"
#include "../C++/optionparser.h"

using std::cout;
using std::endl;

enum  optionIndex { UNKNOWN, HELP, IP_ADDR, MAC_ADDR, DHCP_ENABLE, SPI, LOG_LEVEL, RESET};
const option::Descriptor usage[] =
{
  {UNKNOWN, 0,"" , ""    , option::Arg::None, "USAGE: flash [options]\n\n"
                                           "Options:" },
  {HELP,    0,"" , "help", option::Arg::None, "  --help  \tPrint usage and exit." },
  {IP_ADDR, 0,"", "IP", option::Arg::None, "  --IP  \tProgram a new IP address." },
  {MAC_ADDR, 0,"", "MAC", option::Arg::None, "  --MAC  \tProgram a new MAC address." },
  {DHCP_ENABLE, 0,"", "DHCP", option::Arg::None, "  --DHCP  \tProgram DHCP Enable" },
  {SPI,  0,"", "SPI", option::Arg::None, "  --SPI  \tWrite the SPI startup sequence." },
  {LOG_LEVEL,  0,"", "logLevel", option::Arg::Numeric, "  --logLevel  \t(optional) Logging level level to print to console (optional; default=2/INFO)." },
  {RESET,  0,"", "reset", option::Arg::None, "  --reset  \t(optional) Issue soft reset to FPGA (optional; default=N)." },
    
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
  cout << concol::YELLOW << "New MAC address [ENTER to skip]: " << concol::RESET;
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
  cout << concol::YELLOW << "New IP address [ENTER to skip]: " << concol::RESET;
  string input = "";
  getline(cin, input);
  return input;
}

bool get_yes_no_input() {
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

bool get_dhcp_input() {
  cout << concol::YELLOW << "Do you want to enable DHCP [y/N]: " << concol::RESET;
  return get_yes_no_input();
}


bool spi_prompt() {
  cout << concol::YELLOW << "Do you want to program the SPI startup sequence? [y/N]: " << concol::RESET;
  return get_yes_no_input();
}

bool get_soft_reset_input() {
  cout << concol::YELLOW << "Do you want to soft reset FPGA [y/N]: " << concol::RESET;
  return get_yes_no_input();
}


int main(int argc, char* argv[])
{

  print_title("BBN APS2 Flash Writing Utility");

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
    cout << concol::CYAN << "Programmed MAC and IP address at 0x00FF0000 are " << concol::RESET << endl;
    cout << concol::CYAN << "MAC addr: " << hexn<12> << get_mac_addr(deviceSerial.c_str()) << concol::RESET << endl;
    char curIP[16];
    get_ip_addr(deviceSerial.c_str(), curIP);
    cout << concol::CYAN << "IP addr: " << curIP << concol::RESET << endl;
    int dhcp_enable;
    get_dhcp_enable(deviceSerial.c_str(), &dhcp_enable);
    cout << concol::CYAN << "DHCP enable: " <<  dhcp_enable << endl;

  }

  // write a new MAC address
  if (options[MAC_ADDR] || interactiveMode) {
    uint64_t mac_addr = get_mac_input();
    if (mac_addr != 0) {
      cout << concol::CYAN << "Writing new MAC address" << concol::RESET << endl;
      set_mac_addr(deviceSerial.c_str(), mac_addr);
    }
  }

  // write a new IP address
  if (options[IP_ADDR] || interactiveMode) {
    string ip_addr = get_ip_input();
    if (ip_addr != "") {
      cout << concol::CYAN << "Writing new IP address" << concol::RESET << endl;
      set_ip_addr(deviceSerial.c_str(), ip_addr.c_str());
    }
  }

  // write dhcp_ebable
  if (options[DHCP_ENABLE] || interactiveMode) {
    bool dhcp_enable = get_dhcp_input();
    int dhcp_int = (dhcp_enable) ? 1 : 0;
    cout << concol::CYAN << "Writing DHCP enable" << concol::RESET << endl;
    set_dhcp_enable(deviceSerial.c_str(), dhcp_int);
  }

  // read SPI setup sequence
  if (options[SPI] || interactiveMode) {
    uint32_t setup[36];
    read_flash(deviceSerial.c_str(), 0x0, 32, setup);
    cout << concol::CYAN << "Programmed setup SPI sequence:" << concol::RESET << endl;
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
      cout << concol::CYAN << "Writing SPI startup sequence" << concol::RESET << endl;
      write_SPI_setup(deviceSerial.c_str());
    }
  }

  if (options[RESET] || interactiveMode) {
    bool softReset = get_soft_reset_input();
    #define SOFT_RESET 1
    #define HARD_RESET 0
    if (softReset) reset(deviceSerial.c_str(), SOFT_RESET);
  }

  disconnect_APS(deviceSerial.c_str());

  cout << concol::CYAN << "Finished!" << concol::RESET << endl;

  return 0;
}
