#include <iostream>
#include <stdexcept>

#include "headings.h"
#include "libaps2.h"
#include "constants.h"

#include <concol.h>

#include "../C++/helpers.h"
#include "../C++/optionparser.h"

using std::vector;
using std::cout;
using std::endl;
using std::flush;

enum  optionIndex { UNKNOWN, HELP, BIT_FILE, IP_ADDR, PROG_MODE, LOG_LEVEL};
const option::Descriptor usage[] =
{
  {UNKNOWN, 0,"" , ""    , option::Arg::None, "USAGE: play_waveform [options]\n\n"
                                           "Options:" },
  {HELP,    0,"" , "help", option::Arg::None, "  --help  \tPrint usage and exit." },
  {BIT_FILE, 0,"", "bitFile", option::Arg::Required, "  --bitFile  \tPath to firmware bitfile." },
  {IP_ADDR,  0,"", "ipAddr", option::Arg::NonEmpty, "  --ipAddr  \tIP address of unit to program (optional)" },
  {PROG_MODE, 0,"", "progMode", option::Arg::NonEmpty, "  --progMode  \t(optional) Where to program firmware DRAM/EPROM/BACKUP (optional)" },
  {LOG_LEVEL,  0,"", "logLevel", option::Arg::Numeric, "  --logLevel  \t(optional) Logging level level to print (optional; default=2/INFO)" },
  {UNKNOWN, 0,"" ,  ""   , option::Arg::None, "\nExamples:\n"
                                           "  program --bitFile=/path/to/bitfile (all other options will be prompted for)\n"
                                           "  program --bitFile=/path/to/bitfile --ipAddr=192.168.2.2 --progMode=DRAM " },
  {0,0,0,0,0,0}
};


enum MODE {DRAM, EPROM, EPROM_BACKUP};

MODE get_mode() {
  cout << concol::RED << "Programming options:" << concol::RESET << endl;
  cout << "1) Upload DRAM image" << endl;
  cout << "2) Update EPROM image" << endl;
  cout << "3) Update backup EPROM image" << endl << endl;
  cout << "Choose option [1]: ";

  char input;
  cin.get(input);
  switch (input) {
    case '1':
    default:
      return DRAM;
      break;
    case '2':
      return EPROM;
      break;
    case '3':
      return EPROM_BACKUP;
      break;
  }
}

vector<uint32_t> read_bit_file(string fileName) {
  std::ifstream FID (fileName, std::ios::in|std::ios::binary);
  if (!FID.is_open()){
    throw std::runtime_error("Unable to open file.");
  }

  //Get the file size in bytes
  FID.seekg(0, std::ios::end);
  size_t fileSize = FID.tellg();
  cout << "Bitfile is " << fileSize << " bytes" << endl;
  FID.seekg(0, std::ios::beg);

  //Copy over the file data to the data vector
  vector<uint32_t> packedData;
  packedData.resize(fileSize/4);
  FID.read(reinterpret_cast<char *>(packedData.data()), fileSize);

  //Convert to big endian byte order - basically because it will be byte-swapped again when the packet is serialized
  for (auto & packet : packedData) {
    packet = htonl(packet);
  }

  cout << "Bit file is " << packedData.size() << " 32-bit words long" << endl;
  return packedData;
}

int write_image(string deviceSerial, string fileName, MODE mode) {
  vector<uint32_t> data;
  try {
    data = read_bit_file(fileName);
  } catch (std::exception &e) {
    cout << concol::RED << "Unable to open file." << concol::RESET << endl;
    return -1;
  }
  uint32_t addr;
  switch (mode) {
    case EPROM:
      addr = EPROM_USER_IMAGE_ADDR;
      break;
    case EPROM_BACKUP:
      addr = EPROM_BASE_IMAGE_ADDR;
      break;
    default:
      cout << concol::RED << "Unrecognized mode: " << mode << concol::RESET << endl;
      return -2;
  }
  write_flash(deviceSerial.c_str(), addr, data.data(), data.size());
  //verify the write
  vector<uint32_t> buffer(256);
  uint32_t numWords = 256;
  cout << "Verifying:" << endl;
  for (size_t ct=0; ct < data.size(); ct+=256) {
    if (ct % 1000 == 0) {
      cout << "\r" << 100*ct/data.size() << "%" << flush;
    }
    if (std::distance(data.begin() + ct, data.end()) < 256) {
      numWords = std::distance(data.begin() + ct, data.end());
    }
    read_flash(deviceSerial.c_str(), addr + 4*ct, numWords, buffer.data());
    if (!std::equal(buffer.begin(), buffer.begin()+numWords, data.begin()+ct)) {
      cout << endl << "Mismatched data at offset " << hexn<6> << ct << endl;
      return -2;
    }
  }
  cout << "\r100%" << endl;
  return reset(deviceSerial.c_str(), static_cast<int>(APS_RESET_MODE_STAT::RECONFIG_EPROM));
}

int main (int argc, char* argv[])
{

  concol::concolinit();
  cout << concol::RED << "BBN AP2 Firmware Programming Executable" << concol::RESET << endl;

  argc-=(argc>0); argv+=(argc>0); // skip program name argv[0] if present
  option::Stats  stats(usage, argc, argv);
  option::Option *options = new option::Option[stats.options_max];
  option::Option *buffer = new option::Option[stats.buffer_max];
  option::Parser parse(usage, argc, argv, options, buffer);

  if (parse.error())
   return -1;

  if (options[HELP] || argc == 0) {
    option::printUsage(std::cout, usage);
    return 0;
  }

  for (option::Option* opt = options[UNKNOWN]; opt; opt = opt->next())
   std::cout << "Unknown option: " << opt->name << "\n";

  for (int i = 0; i < parse.nonOptionsCount(); ++i)
   std::cout << "Non-option #" << i << ": " << parse.nonOption(i) << "\n";

  //Debug level
  int logLevel = 2;
  if (options[LOG_LEVEL]) {
    logLevel = atoi(options[LOG_LEVEL].arg);
  }
  set_logging_level(logLevel);

  string deviceSerial;
  if (options[IP_ADDR]) {
    deviceSerial = string(options[IP_ADDR].arg);
    cout << "Programming device " << deviceSerial << endl;
  } else {
    deviceSerial = get_device_id();
  }

  MODE mode;
  if (options[PROG_MODE]) {
    string modeIn(options[PROG_MODE].arg);
    if (modeIn.compare("DRAM")) {
      mode = DRAM;
    }
    else if (modeIn.compare("EPROM")){
      mode = EPROM;
    }
    else if (modeIn.compare("BACKUP")){
      mode = EPROM_BACKUP;
    }
    else{
      std::cerr << "Unrecognized programming mode " << modeIn;
      return -1;
    }
  } else {
    mode = get_mode();
  }

  string bitFile(options[BIT_FILE].arg);

  connect_APS(deviceSerial.c_str());

  switch (mode) {
    case EPROM:
    case EPROM_BACKUP:
      cout << concol::RED << "Reprogramming EPROM image" << concol::RESET << endl;
      write_image(deviceSerial, bitFile, mode);
      break;

    case DRAM:
      cout << concol::RED << "Reprogramming DRAM image" << concol::RESET << endl;
      program_FPGA(deviceSerial.c_str(), bitFile.c_str());
      break;
  }

  cout << concol::RED << "Device came up with firmware version: " << hexn<4> << get_firmware_version(deviceSerial.c_str()) << endl;

  disconnect_APS(deviceSerial.c_str());

  cout << concol::RED << "Finished!" << concol::RESET << endl;

  return 0;
}
