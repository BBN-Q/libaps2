#include <iostream>
#include <iterator>
#include <algorithm>
#include <signal.h>

#include "headings.h"
#include "libaps2.h"
#include "constants.h"
#include "concol.h"
#include "helpers.h"

#include "optionparser.h"

enum  optionIndex { UNKNOWN, HELP, WFA_FILE, WFB_FILE, TRIG_MODE, TRIG_INTERVAL, LOG_LEVEL};
const option::Descriptor usage[] =
{
	{UNKNOWN, 0,"" , ""    , option::Arg::None, "USAGE: play_waveform [options]\n\n"
	                                         "Options:" },
	{HELP,    0,"" , "help", option::Arg::None, "  --help  \tPrint usage and exit." },
	{WFA_FILE, 0,"", "wfA", option::Arg::Required, "  --wfA  \tChannel A waveform file (ASCII signed 16 bit integer)" },
	{WFB_FILE, 0,"", "wfB", option::Arg::Required, "  --wfB  \tChannel B waveform file (ASCII signed 16 bit integer)" },
	{TRIG_MODE, 0,"", "trigMode", option::Arg::Required, "  --trigMode  \tTrigger mode (0: external; 1: internal; 2: software" },
	{TRIG_INTERVAL,  0,"", "trigInterval", option::Arg::Numeric, "  --trigRep  \tInternal trigger interval" },
	{LOG_LEVEL,  0,"", "logLevel", option::Arg::Numeric, "  --logLevel  \tLogging level level to print" },
	{UNKNOWN, 0,"" ,  ""   , option::Arg::None, "\nExamples:\n"
	                                         "  play_waveform --wfA=../examples/wfA.dat --wfB=../examples/wfB.dat\n"
	                                         "  play_waveform --wfA=../examples/wfB.dat --trigMode=2\n" },
	{0,0,0,0,0,0}
};


static string deviceSerial;


#ifdef _WIN32
BOOL WINAPI ConsoleHandler(DWORD dwType)
{
	switch(dwType) {
	    case CTRL_C_EVENT:
	        printf("ctrl-c\n");
			std::cout << std::endl;
			stop(deviceSerial.c_str());
			disconnect_APS(deviceSerial.c_str());
			exit(1);
	    default:
	        printf("Some other event\n");
	}
    return TRUE;
}
#else
void clean_up(int sigValue){
	std::cout << std::endl;
	stop(deviceSerial.c_str());
	disconnect_APS(deviceSerial.c_str());
	exit(1);
}
#endif


int main(int argc, char* argv[])
{
	argc-=(argc>0); argv+=(argc>0); // skip program name argv[0] if present
	option::Stats  stats(usage, argc, argv);
	option::Option options[stats.options_max], buffer[stats.buffer_max];
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
	int debugLevel = 4;
	if (options[LOG_LEVEL]) {
		debugLevel = atoi(options[LOG_LEVEL].arg);
	}

	//Trigger source -- default of internal
	int triggerSource = 1;
	if (options[TRIG_MODE]) {
		triggerSource = atoi(options[TRIG_MODE].arg);
	}

	//Trigger interval -- default of 10ms
	double trigInterval = 10e-3;
	if (options[TRIG_INTERVAL]) {
		trigInterval = atof(options[TRIG_INTERVAL].arg);
	}

	//Load the waveform files 
	vector<int16_t> wfA, wfB;
	std::ifstream ifs;
	if (options[WFA_FILE]) {
		ifs.open(std::string(options[WFA_FILE].arg));
		std::copy(std::istream_iterator<int16_t>(ifs), std::istream_iterator<int16_t>(), std::back_inserter(wfA) );
		ifs.close();
	}

	if (options[WFB_FILE]) {
		ifs.open(std::string(options[WFB_FILE].arg));
		std::copy(std::istream_iterator<int16_t>(ifs), std::istream_iterator<int16_t>(), std::back_inserter(wfB) );
	}

	//Pad the waveforms so they are the same size
	size_t longestLength = std::max(wfA.size(), wfB.size());
	wfA.resize(longestLength, 0);
	wfB.resize(longestLength, 0);

	deviceSerial = get_device_id();

	set_logging_level(debugLevel);
	set_log("stdout");

	connect_APS(deviceSerial.c_str());

	double uptime = get_uptime(deviceSerial.c_str());

	cout << concol::RED << "Uptime for device " << deviceSerial << " is " << uptime << " seconds" << concol::RESET << endl;

	// force initialize device
	initAPS(deviceSerial.c_str(), 1);

	//load the waveforms
	set_waveform_int(deviceSerial.c_str(), 0, wfA.data(), wfA.size());
	set_waveform_int(deviceSerial.c_str(), 1, wfB.data(), wfB.size());

	//Set the trigger mode
	set_trigger_source(deviceSerial.c_str(), triggerSource);

	//Trigger interval
	set_trigger_interval(deviceSerial.c_str(), trigInterval);

	//Set to triggered waveform mode
	set_run_mode(deviceSerial.c_str(), 1);

	run(deviceSerial.c_str());

	//Catch ctrl-c to clean_up the APS -- see http://zguide.zeromq.org/cpp:interrupt
	//Unfortunately we have some platform nonsense here
	#ifdef _WIN32
	if (!SetConsoleCtrlHandler((PHANDLER_ROUTINE)ConsoleHandler, TRUE)) {
        std::cerr << "Unable to install handler!" << std::endl;
        return EXIT_FAILURE;
    }
	#else
	struct sigaction action;
	action.sa_handler = clean_up;
	action.sa_flags = 0;
	sigemptyset(&action.sa_mask);
	sigaction(SIGINT, &action, NULL);
	#endif

	//For software trigger, trigger on key stroke
	if (triggerSource == 2) {
		cout << "Return to trigger or ctrl-c to exit";
		while(true) {
			cin.get();
			trigger(deviceSerial.c_str());
		}
		cout << endl;
	}
	else {
		cout << "Ctrl-c to quit";
		while(true) {
			cin.get();
		}
	}

	return 0;
 }