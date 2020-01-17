#include <future>
#include <iomanip>
#include <iostream>
#include <thread>

#include "constants.h"
#include "libaps2.h"

#include <concol.h>

#include "../C++/helpers.h"
#include "../C++/optionparser.h"

using std::vector;
using std::cout;
using std::endl;
using std::flush;

enum optionIndex { UNKNOWN, HELP, BIT_FILE, IP_ADDR, PROG_MODE, LOG_LEVEL };
const option::Descriptor usage[] = {
    {UNKNOWN, 0, "", "", option::Arg::None, "USAGE: program [options]\n\n"
                                            "Options:"},
    {HELP, 0, "", "help", option::Arg::None,
     "	--help	\tPrint usage and exit."},
    {BIT_FILE, 0, "", "bitFile", option::Arg::Required,
     "	--bitFile	\tPath to firmware bitfile (required)."},
    {IP_ADDR, 0, "", "ipAddr", option::Arg::NonEmpty,
     "	--ipAddr	\tIP address of unit to program (optional)."},
    {PROG_MODE, 0, "", "progMode", option::Arg::NonEmpty,
     "	--progMode	\t(optional) Where to program firmware "
     "DRAM/EPROM/BACKUP (optional)."},
    {LOG_LEVEL, 0, "", "logLevel", option::Arg::Numeric,
     "	--logLevel	\t(optional) Logging level level to print (optional; "
     "default=3/DEBUG)."},
    {UNKNOWN, 0, "", "", option::Arg::None,
     "\nExamples:\n"
     "	program --bitFile=/path/to/bitfile (all other options will be prompted "
     "for)\n"
     "	program --bitFile=/path/to/bitfile --ipAddr=192.168.2.2 "
     "--progMode=DRAM "},
    {0, 0, 0, 0, 0, 0}};

enum PROGRAM_TARGET { TARGET_DRAM, TARGET_EPROM, TARGET_EPROM_BACKUP };

PROGRAM_TARGET get_target() {
  cout << endl
       << concol::RED << "Programming options:" << concol::RESET << endl;
  cout << "1) Upload DRAM image" << endl;
  cout << "2) Update EPROM image" << endl;
  cout << "3) Update backup EPROM image" << endl << endl;
  cout << "Choose option [1]: ";

  char input;
  cin.get(input);
  switch (input) {
  case '1':
  default:
    return TARGET_DRAM;
    break;
  case '2':
    return TARGET_EPROM;
    break;
  case '3':
    return TARGET_EPROM_BACKUP;
    break;
  }
}

void progress_bar(string label, double percent) {
  std::string bar(50, ' ');

  size_t str_pos = static_cast<size_t>(percent * 50);
  bar.replace(0, str_pos, str_pos, '=');
  if (str_pos < bar.size()) {
    bar.replace(str_pos, 1, 1, '>');
  }

  cout << label;
  cout << std::setw(3) << static_cast<int>(100 * percent) << "% [" << bar
       << "] ";
  cout << "\r" << std::flush;
}

int main(int argc, char *argv[]) {

  print_title("BBN APS2 Firmware Programming Utility");

  argc -= (argc > 0);
  argv += (argc > 0); // skip program name argv[0] if present
  option::Stats stats(usage, argc, argv);
  option::Option *options = new option::Option[stats.options_max];
  option::Option *buffer = new option::Option[stats.buffer_max];
  option::Parser parse(usage, argc, argv, options, buffer);

  if (parse.error())
    return -1;

  if (options[HELP] || argc == 0) {
    option::printUsage(std::cout, usage);
    return 0;
  }

  for (option::Option *opt = options[UNKNOWN]; opt; opt = opt->next())
    std::cout << "Unknown option: " << opt->name << "\n";

  for (int i = 0; i < parse.nonOptionsCount(); ++i)
    std::cout << "Non-option #" << i << ": " << parse.nonOption(i) << "\n";

  string bitfile;
  if (options[BIT_FILE]) {
    bitfile = string(options[BIT_FILE].arg);
  } else {
    std::cerr << "Bitfile option is required." << endl;
    return -1;
  }

  // Logging level
  TLogLevel logLevel = logDEBUG1;
  if (options[LOG_LEVEL]) {
    logLevel = TLogLevel(atoi(options[LOG_LEVEL].arg));
  }
  set_logging_level(logLevel);

  vector<string> ip_addrs;
  if (options[IP_ADDR]) {
    ip_addrs = {string(options[IP_ADDR].arg)};
  } else {
    ip_addrs = get_device_ids();
  }
  if ( ip_addrs.empty() ) {
    cout << concol::RED << "No APS2 devices connected! Exiting..."
         << concol::RESET << endl;
    return 0;
  }

  PROGRAM_TARGET target;
  if (options[PROG_MODE]) {
    string target_str(options[PROG_MODE].arg);
    std::map<string, PROGRAM_TARGET> target_map{
        {"DRAM", TARGET_DRAM},
        {"EPROM", TARGET_EPROM},
        {"BACKUP", TARGET_EPROM_BACKUP}};
    if (target_map.count(target_str)) {
      target = target_map[target_str];
    } else {
      std::cerr << "Unrecognized programming mode " << target_str;
      return -1;
    }
  } else {
    target = get_target();
  }

  for (auto ip_addr : ip_addrs) {
    cout << endl << "Programming " << ip_addr << endl;

    connect_APS(ip_addr.c_str());

    uint32_t target_addr;
    APS2_BITFILE_STORAGE_MEDIA target_media;
    switch (target) {
    case TARGET_EPROM:
      cout << "Reprogramming user EPROM image" << endl;
      target_addr = EPROM_USER_IMAGE_ADDR;
      target_media = BITFILE_MEDIA_EPROM;
      break;
    case TARGET_EPROM_BACKUP:
      cout << "Reprogramming backup EPROM image" << endl;
      target_addr = EPROM_BASE_IMAGE_ADDR;
      target_media = BITFILE_MEDIA_EPROM;
      break;
    case TARGET_DRAM:
      cout << "Reprogramming DRAM image" << endl;
      // default to address 0 for now
      target_addr = 0;
      target_media = BITFILE_MEDIA_DRAM;
      break;
    }

    clear_bitfile_writing_progress(ip_addr.c_str());
    // Launch the write on another thread so we can poll for progress
    auto thread_future = std::async(
        std::launch::async, [ip_addr, bitfile, target_addr, target_media]() {
          return write_bitfile(ip_addr.c_str(), bitfile.c_str(), target_addr,
                               target_media);
        });

    auto prev_bitfile_writing_task = get_bitfile_writing_task(ip_addr.c_str());

    while (get_bitfile_writing_task(ip_addr.c_str()) != DONE) {
      // Make sure we haven't return prematurely
      auto thread_status = thread_future.wait_for(std::chrono::milliseconds(0));
      if (thread_status == std::future_status::ready) {
        auto status = thread_future.get();
        if (status == APS2_OK) {
          break;
        } else {
          std::cerr << "Writing bit file failed with error message: "
                    << get_error_msg(status) << endl;
          disconnect_APS(ip_addr.c_str());
          return -1;
        }
      }

      // Otherwise update progress bars
      // First check for change in task to move onto next progress bar
      auto cur_bitfile_writing_task = get_bitfile_writing_task(ip_addr.c_str());
      if (cur_bitfile_writing_task != prev_bitfile_writing_task) {
        prev_bitfile_writing_task = cur_bitfile_writing_task;
        cout << endl;
      }

      // Update the progress bar
      std::map<APS2_BITFILE_WRITING_TASK, string> task_string_map{
          {ERASING, "Erasing: "},
          {WRITING, "Writing: "},
          {VALIDATING, "Validating: "}};
      if ((cur_bitfile_writing_task == ERASING) ||
          (cur_bitfile_writing_task == WRITING) ||
          (cur_bitfile_writing_task == VALIDATING)) {
        progress_bar(task_string_map[cur_bitfile_writing_task],
                     get_flash_progress(ip_addr.c_str()));
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    cout << endl;

    if (target == TARGET_DRAM) {
      // boot to DARM
      cout << concol::RED << "Booting from DRAM image... ";
      program_bitfile(ip_addr.c_str(), target_addr);
      // APS will drop connection so disconnect wait and reconnect
      disconnect_APS(ip_addr.c_str());
      for (size_t ct = 5; ct > 0; ct--) {
        cout << ct << " " << std::flush;
        std::this_thread::sleep_for(std::chrono::seconds(1));
      }
      cout << endl;
      APS2_STATUS status;
      status = connect_APS(ip_addr.c_str());

      if (status != APS2_OK) {
        std::cerr << "APS2 failed to come back up after programming!" << endl;
        cout << concol::RESET << endl;
        disconnect_APS(ip_addr.c_str());
        return -1;
      }

      char new_firmware_version[64];
      get_firmware_version(ip_addr.c_str(), nullptr, nullptr, nullptr,
                           new_firmware_version);
      cout << concol::RED
           << "Device came up with firmware version: " << new_firmware_version
           << endl;
    }

    disconnect_APS(ip_addr.c_str());
    cout << concol::RED << "Finished " << ip_addr << concol::RESET << endl;
  }
  return 0;
}
