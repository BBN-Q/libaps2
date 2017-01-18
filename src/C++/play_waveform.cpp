#include <fstream>
#include <iostream>
#include <iterator>
#include <signal.h>
#include <vector>
#include <algorithm> // std::max

#include "concol.h"
#include "helpers.h"
#include "libaps2.h"

#include "optionparser.h"

using std::vector;
using std::cout;
using std::endl;

enum optionIndex {
  UNKNOWN,
  HELP,
  WFA_FILE,
  WFB_FILE,
  TRIG_MODE,
  TRIG_INTERVAL,
  LOG_LEVEL
};
const option::Descriptor usage[] = {
    {UNKNOWN, 0, "", "", option::Arg::None, "USAGE: play_waveform [options]\n\n"
                                            "Options:"},
    {HELP, 0, "", "help", option::Arg::None,
     "	--help	\tPrint usage and exit."},
    {WFA_FILE, 0, "", "wfA", option::Arg::Required,
     "	--wfA	\tChannel A waveform file (ASCII one signed 16 bit integer per "
     "line).	Only wfA or wfB required."},
    {WFB_FILE, 0, "", "wfB", option::Arg::Required,
     "	--wfB	\tChannel B waveform file (ASCII one signed 16 bit integer per "
     "line). Only wfA or wfB required."},
    {TRIG_MODE, 0, "", "trigMode", option::Arg::Required,
     "	--trigMode	\tTrigger mode "
     "(0: external; 1: internal; 2: "
     "software - optional; default=1)."},
    {TRIG_INTERVAL, 0, "", "trigInterval", option::Arg::Numeric,
     "	--trigRep	\tInternal trigger interval (optional; default=10ms)."},
    {LOG_LEVEL, 0, "", "logLevel", option::Arg::Numeric,
     "	--logLevel	\tLogging level level to print (optional; "
     "default=2/INFO)."},
    {UNKNOWN, 0, "", "", option::Arg::None,
     "\nExamples:\n"
     "	play_waveform --wfA=../examples/wfA.dat --wfB=../examples/wfB.dat\n"
     "	play_waveform --wfA=../examples/wfB.dat --trigMode=2\n"},
    {0, 0, 0, 0, 0, 0}};

int main(int argc, char *argv[]) {

  print_title("BBN APS2 Waveform Player");

  argc -= (argc > 0);
  argv += (argc > 0); // skip program name argv[0] if present
  option::Stats stats(usage, argc, argv);
  option::Option *options = new option::Option[stats.options_max];
  option::Option *buffer = new option::Option[stats.buffer_max];
  option::Parser parse(usage, argc, argv, options, buffer);

  if (parse.error())
    return -1;

  if (options[HELP] || argc == 0) {
    option::printUsage(cout, usage);
    return 0;
  }

  for (option::Option *opt = options[UNKNOWN]; opt; opt = opt->next())
    cout << "Unknown option: " << opt->name << "\n";

  for (int i = 0; i < parse.nonOptionsCount(); ++i)
    cout << "Non-option #" << i << ": " << parse.nonOption(i) << "\n";

  // Logging level
  TLogLevel logLevel = logINFO;
  if (options[LOG_LEVEL]) {
    logLevel = TLogLevel(atoi(options[LOG_LEVEL].arg));
  }

  // Trigger source -- default of internal
  APS2_TRIGGER_SOURCE triggerSource = INTERNAL;
  if (options[TRIG_MODE]) {
    triggerSource = APS2_TRIGGER_SOURCE(atoi(options[TRIG_MODE].arg));
  }

  // Trigger interval -- default of 10ms
  double trigInterval = 10e-3;
  if (options[TRIG_INTERVAL]) {
    trigInterval = atof(options[TRIG_INTERVAL].arg);
  }
  set_logging_level(logLevel);
  set_log("stdout");

  // Load the waveform files
  vector<int16_t> wfA, wfB;
  std::ifstream ifs;
  if (options[WFA_FILE]) {
    ifs.open(std::string(options[WFA_FILE].arg));
    std::copy(std::istream_iterator<int16_t>(ifs),
              std::istream_iterator<int16_t>(), std::back_inserter(wfA));
    ifs.close();
    cout << concol::MAGENTA << "Loaded " << wfA.size()
         << " samples for waveform A." << concol::RESET << endl;
  }

  if (options[WFB_FILE]) {
    ifs.open(std::string(options[WFB_FILE].arg));
    std::copy(std::istream_iterator<int16_t>(ifs),
              std::istream_iterator<int16_t>(), std::back_inserter(wfB));
    cout << concol::MAGENTA << "Loaded " << wfB.size()
         << " samples for waveform B." << concol::RESET << endl;
  }

  // Pad the waveforms so they are the same size
  size_t longestLength = std::max(wfA.size(), wfB.size());
  wfA.resize(longestLength, 0);
  wfB.resize(longestLength, 0);

  string deviceSerial = get_device_id();
  if ( deviceSerial.empty() ) {
    cout << concol::RED << "No APS2 devices connected! Exiting..."
         << concol::RESET << endl;
    return 0;
  }

  connect_APS(deviceSerial.c_str());

  double uptime;
  get_uptime(deviceSerial.c_str(), &uptime);

  cout << concol::CYAN << "Uptime for device " << deviceSerial << " is "
       << uptime << " seconds" << concol::RESET << endl;

  // force initialize device
  init_APS(deviceSerial.c_str(), 1);

  // load the waveforms
  set_waveform_int(deviceSerial.c_str(), 0, wfA.data(), wfA.size());
  set_waveform_int(deviceSerial.c_str(), 1, wfB.data(), wfB.size());

  // Set the trigger mode
  set_trigger_source(deviceSerial.c_str(), triggerSource);

  // Trigger interval
  set_trigger_interval(deviceSerial.c_str(), trigInterval);

  // Set to triggered waveform mode
  set_run_mode(deviceSerial.c_str(), TRIG_WAVEFORM);

  run(deviceSerial.c_str());

  // For software trigger, trigger on key stroke
  if (triggerSource == 2) {
    cout << concol::YELLOW << "Press t-Return to trigger or q-Return to exit"
         << concol::RESET << endl;
    while (true) {
      char keyStroke = cin.get();
      if (keyStroke == 't') {
        trigger(deviceSerial.c_str());
      } else if (keyStroke == 'q') {
        break;
      }
    }
  } else {
    cout << concol::YELLOW << "Press any key to stop" << concol::RESET;
    cin.get();
  }

  stop(deviceSerial.c_str());
  disconnect_APS(deviceSerial.c_str());

  delete[] options;
  delete[] buffer;
  return 0;
}
