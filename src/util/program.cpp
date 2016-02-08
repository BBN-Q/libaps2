#include <iostream>
#include <thread>
#include <future>

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

enum	optionIndex { UNKNOWN, HELP, BIT_FILE, IP_ADDR, PROG_MODE, LOG_LEVEL};
const option::Descriptor usage[] =
{
	{UNKNOWN, 0,"" , ""		, option::Arg::None, "USAGE: program [options]\n\n"
																					 "Options:" },
	{HELP,		0,"" , "help", option::Arg::None, "	--help	\tPrint usage and exit." },
	{BIT_FILE, 0,"", "bitFile", option::Arg::Required, "	--bitFile	\tPath to firmware bitfile (required)." },
	{IP_ADDR,	0,"", "ipAddr", option::Arg::NonEmpty, "	--ipAddr	\tIP address of unit to program (optional)." },
	{PROG_MODE, 0,"", "progMode", option::Arg::NonEmpty, "	--progMode	\t(optional) Where to program firmware DRAM/EPROM/BACKUP (optional)." },
	{LOG_LEVEL,	0,"", "logLevel", option::Arg::Numeric, "	--logLevel	\t(optional) Logging level level to print (optional; default=3/DEBUG)." },
	{UNKNOWN, 0,"" ,	""	 , option::Arg::None, "\nExamples:\n"
																					 "	program --bitFile=/path/to/bitfile (all other options will be prompted for)\n"
																					 "	program --bitFile=/path/to/bitfile --ipAddr=192.168.2.2 --progMode=DRAM " },
	{0,0,0,0,0,0}
};


enum PROGRAM_TARGET {TARGET_DRAM, TARGET_EPROM, TARGET_EPROM_BACKUP};

PROGRAM_TARGET get_target() {
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

void progress_bar(string label, double percent ){
	std::string bar(50, ' ');

	size_t str_pos = static_cast<size_t>(percent*50);
	bar.replace(0, str_pos, str_pos, '=');
	if (str_pos < bar.size()) {
		bar.replace(str_pos, 1, 1, '>');
	}

	cout << label;
	cout << std::setw(3) << static_cast<int>(100*percent) << "% [" << bar << "] ";
	cout << "\r" << std::flush;
}

int main (int argc, char* argv[])
{

	concol::concolinit();
	cout << concol::RED << "BBN AP2 Firmware Programming Executable" << concol::RESET << endl;

	argc-=(argc>0); argv+=(argc>0); // skip program name argv[0] if present
	option::Stats	stats(usage, argc, argv);
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

	string bitfile;
	if (options[BIT_FILE]) {
		bitfile = string(options[BIT_FILE].arg);
	}
	else {
		std::cerr << "Bitfile option is required." << endl;
		return -1;
	}

	//Logging level
	TLogLevel logLevel = logDEBUG1;
	if (options[LOG_LEVEL]) {
		logLevel = TLogLevel(atoi(options[LOG_LEVEL].arg));
	}
	set_logging_level(logLevel);

	string deviceSerial;
	if (options[IP_ADDR]) {
		deviceSerial = string(options[IP_ADDR].arg);
		cout << "Programming device " << deviceSerial << endl;
	} else {
		deviceSerial = get_device_id();
	}

	PROGRAM_TARGET target;
	if (options[PROG_MODE]) {
		string target_in(options[PROG_MODE].arg);
		if (!target_in.compare("DRAM")) {
			target = TARGET_DRAM;
		}
		else if (!target_in.compare("EPROM")){
			target = TARGET_EPROM;
		}
		else if (!target_in.compare("BACKUP")){
			target = TARGET_EPROM_BACKUP;
		}
		else{
			std::cerr << "Unrecognized programming mode " << target_in;
			return -1;
		}
	} else {
		target = get_target();
	}

	connect_APS(deviceSerial.c_str());

	uint32_t target_addr;
	switch (target) {
		case TARGET_EPROM:
			cout << concol::RED << "Reprogramming user EPROM image" << concol::RESET << endl;
			target_addr = EPROM_USER_IMAGE_ADDR;
			break;
		case TARGET_EPROM_BACKUP:
			cout << concol::RED << "Reprogramming backup EPROM image" << concol::RESET << endl;
			target_addr = EPROM_BASE_IMAGE_ADDR;
			break;
		case TARGET_DRAM:
			cout << concol::RED << "Reprogramming DRAM image" << concol::RESET << endl;
			//default to address 0 for now
			target_addr = 0;
			break;
	}

	switch (target) {
		case TARGET_EPROM:
		case TARGET_EPROM_BACKUP:
		{
			auto thread_future = std::async(std::launch::async, [deviceSerial, bitfile, target_addr]() {
				return write_bitfile(deviceSerial.c_str(), bitfile.c_str(), target_addr, BITFILE_MEDIA_EPROM);
			});

			//First report erase percentage
			double percentage;
			do {
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
				//Make sure we haven't return prematurely
				auto thread_status = thread_future.wait_for(std::chrono::milliseconds(0));
				if ( thread_status == std::future_status::ready ) {
					auto status = thread_future.get();
					if ( status == APS2_OK ) {
						break;
					} else {
						std::cerr << "Writing bit file failed with error message: " << get_error_msg(status) << endl;
						disconnect_APS(deviceSerial.c_str());
						return -1;
					}
				}
				percentage = get_flash_erase_done(deviceSerial.c_str());
				progress_bar("Erasing: ", percentage);
			} while(percentage < (1-1e-6));
			cout << endl;

			//Now write percentage
			do {
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
				//Make sure we haven't return prematurely
				auto thread_status = thread_future.wait_for(std::chrono::milliseconds(0));
				if ( thread_status == std::future_status::ready ) {
					auto status = thread_future.get();
					if ( status == APS2_OK ) {
						break;
					} else {
						std::cerr << "Writing bit file failed with error message: " << get_error_msg(status) << endl;
						disconnect_APS(deviceSerial.c_str());
						return -1;
					}
				}
				percentage = get_flash_write_done(deviceSerial.c_str());
				progress_bar("Writing: ", percentage);
			} while(percentage < (1-1e-6));
			cout << endl;

			//Now validation percentage
			do {
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
				//Make sure we haven't return prematurely
				auto thread_status = thread_future.wait_for(std::chrono::milliseconds(0));
				if ( thread_status == std::future_status::ready ) {
					auto status = thread_future.get();
					if ( status == APS2_OK ) {
						break;
					} else {
						std::cerr << "Writing bit file failed with error message: " << get_error_msg(status) << endl;
						disconnect_APS(deviceSerial.c_str());
						return -1;
					}
				}
				percentage = get_flash_validate_done(deviceSerial.c_str());
				progress_bar("Validating: ", percentage);
			} while(percentage < (1-1e-6));
			cout << endl;
			break;
		}
		case TARGET_DRAM:
			write_bitfile(deviceSerial.c_str(), bitfile.c_str(), target_addr, BITFILE_MEDIA_DRAM);
			cout << concol::RED << "Booting from DRAM image... ";
			program_bitfile(deviceSerial.c_str(), target_addr);
			//APS will drop connection so disconnect wait and reconnect
			disconnect_APS(deviceSerial.c_str());
			for (size_t ct = 0; ct < 5; ct++) {
				cout << 5-ct << " " << std::flush;
				std::this_thread::sleep_for(std::chrono::seconds(1));
			}
			cout << endl;
			APS2_STATUS status;
			status = connect_APS(deviceSerial.c_str());

			if ( status != APS2_OK ) {
				std::cerr << "APS2 failed to come back up after programming!" << endl;
				return -1;
			}

			uint32_t newVersion{0};
			get_firmware_version(deviceSerial.c_str(), &newVersion);
			cout << concol::RED << "Device came up with firmware version: " << print_firmware_version(newVersion) << endl;
			break;
		}


	disconnect_APS(deviceSerial.c_str());
	cout << concol::RED << "Finished!" << concol::RESET << endl;

	return 0;
}
