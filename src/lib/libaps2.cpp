/*
 * libaps.cpp - Library for APSv2 control. 
 *
 */

#include <sstream>
#include <functional>

using namespace std::placeholders;
using std::function;
using std::bind;


#include "headings.h"
#include "libaps2.h"
#include "APS2.h"
#include "APSEthernet.h"
#include "asio.hpp"

weak_ptr<APSEthernet> ethernetRM; //resource manager for the asio ethernet interface
map<string, APS2> APSs; //map to hold on to the APS instances
set<string> deviceSerials; // set of APSs that responded to an enumerate broadcast

// stub class to close the logger file handle when the driver goes out of scope
class InitAndCleanUp {
public:
	InitAndCleanUp();
	~InitAndCleanUp();
};

InitAndCleanUp::InitAndCleanUp() {
	//Open the default log file
	FILE* pFile = fopen("libaps2.log", "a");
	Output2FILE::Stream() = pFile;
}

InitAndCleanUp::~InitAndCleanUp() {
	if (Output2FILE::Stream()) {
		fclose(Output2FILE::Stream());
	}
}
InitAndCleanUp initandcleanup_;

shared_ptr<APSEthernet> get_interface() {
	//See if we have to setup our own RM
	shared_ptr<APSEthernet> myEthernetRM = ethernetRM.lock();

	if (!myEthernetRM) {
		myEthernetRM = std::make_shared<APSEthernet>();
		ethernetRM = myEthernetRM;
	}

	return myEthernetRM;
}

template<typename F>
APS2_STATUS aps2_call(const char * deviceSerial, F func){
	try {
		func(APSs.at(deviceSerial));
		//Nothing thrown then assume OK
		return APS2_OK;
	}
	catch (std::out_of_range e) {
		if (APSs.find(deviceSerial) == APSs.end()) {
			return APS2_UNCONNECTED;
		} else {
			return APS2_UNKNOWN_ERROR;
		}
	}
	catch (APS2_STATUS status) {
		return status;
	}
	catch (...) {
		return APS2_UNKNOWN_ERROR;
	}
}

template<typename R, typename F>
APS2_STATUS aps2_getter(const char * deviceSerial, F func, R *resPtr){
	try {
		*resPtr = func(APSs.at(deviceSerial));
		//Nothing thrown then assume OK
		return APS2_OK;
	}
	catch (std::out_of_range e) {
		if (APSs.find(deviceSerial) == APSs.end()) {
			return APS2_UNCONNECTED;
		} else {
			return APS2_UNKNOWN_ERROR;
		}
	}
	catch (APS2_STATUS status) {
		return status;
	}
	catch (...) {
		return APS2_UNKNOWN_ERROR;
	}
}

#ifdef __cplusplus
extern "C" {
#endif

const char* get_error_msg(APS2_STATUS err) {
	if (messages.count(err)) {
		return messages[err].c_str();
	}
	else {
		return "No error message for this status number.";
	}
}

APS2_STATUS get_numDevices(unsigned int * numDevices) {
	/*
	Returns the number of APS2s that respond to a broadcast status request.
	*/
	try {
		deviceSerials = get_interface()->enumerate();
		*numDevices = deviceSerials.size();
		return APS2_OK;
	}
	catch (APS2_STATUS status) {
		return status;
	}
	catch (...) {
		return APS2_UNKNOWN_ERROR;
	}
}

APS2_STATUS get_deviceSerials(const char ** deviceSerialsOut) {
	/*
	Fill in an array of char* with null-terminated char arrays with the
	enumerated device ip addresses.
	Assumes sufficient memory has been allocated
	*/
	size_t ct = 0;
	for (auto & serial : deviceSerials) {
		deviceSerialsOut[ct] = serial.c_str();
		ct++;
	}
	return APS2_OK;
}

APS2_STATUS connect_APS(const char * deviceSerial) {
	/*
	Connect to a device specified by serial number string
	*/
	string serial = string(deviceSerial);
	// create the APS2 object if it is not already in the map
	if (APSs.find(serial) == APSs.end()) {
		APSs[serial] = APS2(serial);
	}
	//Can't seem to bind the interface
	// function<void(APS2&)> func = bind(&APS2::connect, _1, get_interface());
	// return aps2_call(deviceSerial, func);
	try {
		APSs.at(deviceSerial).connect(get_interface());
		return APS2_OK;
	}
	catch (APS2_STATUS status) {
		return status;
	}
	catch (...) {
		return APS2_UNKNOWN_ERROR;
	}
}

APS2_STATUS disconnect_APS(const char * deviceSerial) {
	/*
	Tear-down connection to APS specified by serial number string.
	*/
	auto func = bind(&APS2::disconnect, _1);
	APS2_STATUS status = aps2_call(deviceSerial, func);
	APSs.erase(string(deviceSerial));
	return status;
}

APS2_STATUS reset(const char * deviceSerial, int resetMode) {
	auto func = bind(&APS2::reset, _1, static_cast<APS_RESET_MODE_STAT>(resetMode));
	return aps2_call(deviceSerial, func);
}

//Initialize an APS unit
//Assumes null-terminated bitFile
APS2_STATUS initAPS(const char * deviceSerial, int forceReload) {
	auto func = bind(&APS2::init, _1, bool(forceReload), 0);
	return aps2_call(deviceSerial, func);
}

APS2_STATUS get_firmware_version(const char * deviceSerial, uint32_t * version) {
	auto func = bind(&APS2::get_firmware_version, _1);
	return aps2_getter(deviceSerial, func, version);
}

APS2_STATUS get_uptime(const char * deviceSerial, double * upTime) {
	auto func = bind(&APS2::get_uptime, _1);
	return aps2_getter(deviceSerial, func, upTime);
}

APS2_STATUS get_fpga_temperature(const char * deviceSerial, double * temp) {
	auto func = bind(&APS2::get_fpga_temperature, _1);
	return aps2_getter(deviceSerial, func, temp);
}

APS2_STATUS set_sampleRate(const char * deviceSerial, unsigned int freq) {
	auto func = bind(&APS2::set_sampleRate, _1, freq);
	return aps2_call(deviceSerial, func);
}

APS2_STATUS get_sampleRate(const char * deviceSerial, unsigned int * freq) {
	auto func = bind(&APS2::get_sampleRate, _1);
	return aps2_getter(deviceSerial, func, freq);
}

//Load the waveform library as floats
APS2_STATUS set_waveform_float(const char * deviceSerial, int channelNum, float* data, int numPts) {
	auto func = bind(static_cast<void(APS2::*)(const int&, const vector<float>&)>(&APS2::set_waveform), _1, channelNum, vector<float>(data, data+numPts));
	return aps2_call(deviceSerial, func);
}

//Load the waveform library as int16
APS2_STATUS set_waveform_int(const char * deviceSerial, int channelNum, int16_t* data, int numPts) {
	auto func = bind(static_cast<void(APS2::*)(const int&, const vector<int16_t>&)>(&APS2::set_waveform), _1, channelNum, vector<int16_t>(data, data+numPts));
	return aps2_call(deviceSerial, func);
}

APS2_STATUS set_markers(const char * deviceSerial, int channelNum, uint8_t* data, int numPts) {
	auto func = bind(&APS2::set_markers, _1, channelNum, vector<uint8_t>(data, data+numPts));
	return aps2_call(deviceSerial, func);
}

APS2_STATUS write_sequence(const char * deviceSerial, uint64_t* data, uint32_t numWords) {
	auto func = bind(&APS2::write_sequence, _1, vector<uint64_t>(data, data+numWords));
	return aps2_call(deviceSerial, func);
}

APS2_STATUS load_sequence_file(const char * deviceSerial, const char * seqFile) {
	auto func = bind(&APS2::load_sequence_file, _1, string(seqFile));
	return aps2_call(deviceSerial, func);
}

APS2_STATUS clear_channel_data(const char * deviceSerial) {
	auto func = bind(&APS2::clear_channel_data, _1);
	return aps2_call(deviceSerial, func);
}

APS2_STATUS run(const char * deviceSerial) {
	auto func = bind(&APS2::run, _1);
	return aps2_call(deviceSerial, func);
}

APS2_STATUS stop(const char * deviceSerial) {
	auto func = bind(&APS2::stop, _1);
	return aps2_call(deviceSerial, func);
}

APS2_STATUS get_runState(const char * deviceSerial, RUN_STATE* state) {
	auto func = bind(&APS2::get_runState, _1);
	return aps2_getter(deviceSerial, func, state);
}

//Expects a null-terminated character array
APS2_STATUS set_log(const char * fileNameArr) {

	//Close the current file
	if (Output2FILE::Stream()) fclose(Output2FILE::Stream());
	
	string fileName(fileNameArr);
	if (fileName.compare("stdout") == 0) {
		Output2FILE::Stream() = stdout;
		return APS2_OK;
	}
	else if (fileName.compare("stderr") == 0) {
		Output2FILE::Stream() = stdout;
		return APS2_OK;
	}
	else {
		FILE* pFile = fopen(fileName.c_str(), "a");
		if (!pFile) {
			return APS2_FILELOG_ERROR;
		}
		Output2FILE::Stream() = pFile;
		return APS2_OK;
	}
}

APS2_STATUS set_logging_level(TLogLevel logLevel) {
	FILELog::ReportingLevel() = TLogLevel(logLevel);
	return APS2_OK;
}

APS2_STATUS set_trigger_source(const char * deviceSerial, TRIGGER_SOURCE triggerSource) {
	auto func = bind(&APS2::set_trigger_source, _1, triggerSource);
	return aps2_call(deviceSerial, func);	
}

APS2_STATUS get_trigger_source(const char * deviceSerial, TRIGGER_SOURCE * source) {
	function<TRIGGER_SOURCE(APS2&)> func = bind(&APS2::get_trigger_source, _1);
	return aps2_getter(deviceSerial, func, source);
}

APS2_STATUS set_trigger_interval(const char * deviceSerial, double interval) {
	auto func = bind(&APS2::set_trigger_interval, _1, interval);
	return aps2_call(deviceSerial, func);	
}

APS2_STATUS get_trigger_interval(const char * deviceSerial, double * interval) {
	function<double(APS2&)> func = bind(&APS2::get_trigger_interval, _1);
	return aps2_getter(deviceSerial, func, interval);
}

APS2_STATUS trigger(const char* deviceSerial) {
	auto func = bind(&APS2::trigger, _1);
	return aps2_call(deviceSerial, func);	
}

APS2_STATUS set_channel_offset(const char * deviceSerial, int channelNum, float offset) {
	auto func = bind(&APS2::set_channel_offset, _1, channelNum, offset);
	return aps2_call(deviceSerial, func);
}
APS2_STATUS set_channel_scale(const char * deviceSerial, int channelNum, float scale) {
	auto func = bind(&APS2::set_channel_scale, _1, channelNum, scale);
	return aps2_call(deviceSerial, func);
}
APS2_STATUS set_channel_enabled(const char * deviceSerial, int channelNum, int enable) {
	auto func = bind(&APS2::set_channel_enabled, _1, channelNum, enable);
	return aps2_call(deviceSerial, func);
}

APS2_STATUS get_channel_offset(const char * deviceSerial, int channelNum, float * offset) {
	auto func = bind(&APS2::get_channel_offset, _1, channelNum);
	return aps2_getter(deviceSerial, func, offset);
}
APS2_STATUS get_channel_scale(const char * deviceSerial, int channelNum, float * scale) {
	auto func = bind(&APS2::get_channel_offset, _1, channelNum);
	return aps2_getter(deviceSerial, func, scale);
}
APS2_STATUS get_channel_enabled(const char * deviceSerial, int channelNum, int * enabled) {
	auto func = bind(&APS2::get_channel_offset, _1, channelNum);
	return aps2_getter(deviceSerial, func, enabled);
}

APS2_STATUS set_run_mode(const char * deviceSerial, RUN_MODE mode) {
	auto func = bind(&APS2::set_run_mode, _1, mode);
	return aps2_call(deviceSerial, func);
}

int write_memory(const char * deviceSerial, uint32_t addr, uint32_t* data, uint32_t numWords) {
	vector<uint32_t> dataVec(data, data+numWords);
	return APSs[string(deviceSerial)].write_memory(addr, dataVec);
}

int read_memory(const char * deviceSerial, uint32_t addr, uint32_t* data, uint32_t numWords) {
	auto readData = APSs[string(deviceSerial)].read_memory(addr, numWords);
	std::copy(readData.begin(), readData.end(), data);
	return 0;
}

int read_register(const char * deviceSerial, uint32_t addr) {
	uint32_t buffer[1];
	read_memory(deviceSerial, addr, buffer, 1);
	return buffer[0];
}

int program_FPGA(const char * deviceSerial, const char * bitFile) {
	return APSs[string(deviceSerial)].program_FPGA(string(bitFile));
}

int write_flash(const char * deviceSerial, uint32_t addr, uint32_t* data, uint32_t numWords) {
	vector<uint32_t> writeData(data, data+numWords);
	return APSs[string(deviceSerial)].write_flash(addr, writeData);
}

int read_flash(const char * deviceSerial, uint32_t addr, uint32_t numWords, uint32_t* data) {
	auto readData = APSs[string(deviceSerial)].read_flash(addr, numWords);
	std::copy(readData.begin(), readData.end(), data);
	return 0;
}

uint64_t get_mac_addr(const char * deviceSerial) {
	return APSs[string(deviceSerial)].get_mac_addr();
}
int set_mac_addr(const char * deviceSerial, uint64_t mac) {
	return APSs[string(deviceSerial)].set_mac_addr(mac);
}

APS2_STATUS get_ip_addr(const char * deviceSerial, char * ipAddrPtr) {
	try {
		uint32_t ipAddr = APSs[string(deviceSerial)].get_ip_addr();
		string ipAddrStr = asio::ip::address_v4(ipAddr).to_string();
		ipAddrStr.copy(ipAddrPtr, ipAddrStr.size(), 0);
		return APS2_OK;
	}
	catch (...) {
		return APS2_UNKNOWN_ERROR;
	}
}

APS2_STATUS set_ip_addr(const char * deviceSerial, const char * ipAddrStr) {
	uint32_t ipAddr = asio::ip::address_v4::from_string(ipAddrStr).to_ulong();
	auto func = bind(&APS2::set_ip_addr, _1, ipAddr);
	return aps2_call(deviceSerial, func);
}

int write_SPI_setup(const char * deviceSerial) {
	return APSs[string(deviceSerial)].write_SPI_setup();
}

int run_DAC_BIST(const char * deviceSerial, const int dac, int16_t* data, unsigned int length, uint32_t* results){
	vector<int16_t> testVec(data, data+length);
	vector<uint32_t> tmpResults;
	int passed = APSs[string(deviceSerial)].run_DAC_BIST(dac, testVec, tmpResults);
	std::copy(tmpResults.begin(), tmpResults.end(), results);
	return passed;
}

int set_DAC_SD(const char * deviceSerial, const int dac, const uint8_t sd){
	return APSs[string(deviceSerial)].set_DAC_SD(dac, sd);
}

#ifdef __cplusplus
}
#endif

