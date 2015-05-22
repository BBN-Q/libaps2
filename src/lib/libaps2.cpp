/*
 * libaps.cpp - C API shared library for APSv2 control.
 *
 */

#include <sstream>

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

//Return the shared_ptr to the Ethernet interface
shared_ptr<APSEthernet> get_interface() {
	//See if we have to setup our own RM
	shared_ptr<APSEthernet> myEthernetRM = ethernetRM.lock();

	if (!myEthernetRM) {
		myEthernetRM = std::make_shared<APSEthernet>();
		ethernetRM = myEthernetRM;
	}

	return myEthernetRM;
}

//Define a couple of templated wrapper functions to make library calls and catch thrown errors
//First one for void calls
template<typename F, typename... Args>
APS2_STATUS aps2_call(const char* deviceSerial, F func, Args... args){
	try {
		(APSs.at(deviceSerial).*func)(args...);
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

//and one for to store getter values in pointer passed to library
template<typename R, typename F, typename... Args>
APS2_STATUS aps2_getter(const char* deviceSerial, F func, R* resPtr, Args... args){
	try {
		*resPtr = (APSs.at(deviceSerial).*func)(args...);
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

APS2_STATUS get_numDevices(unsigned int* numDevices) {
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

APS2_STATUS get_deviceSerials(const char** deviceSerialsOut) {
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

APS2_STATUS connect_APS(const char* deviceSerial) {
	/*
	Connect to a device specified by serial number string
	*/
	string serial = string(deviceSerial);
	// create the APS2 object if it is not already in the map
	if (APSs.find(serial) == APSs.end()) {
		APSs[serial] = APS2(serial);
	}
	//Can't seem to bind the interface lvalue to ‘std::shared_ptr<APSEthernet>&&’
	// return aps2_call(deviceSerial, &APS2::connect, get_interface());
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

APS2_STATUS disconnect_APS(const char* deviceSerial) {
	/*
	Tear-down connection to APS specified by serial number string.
	*/
	APS2_STATUS status = aps2_call(deviceSerial, &APS2::disconnect);
	APSs.erase(string(deviceSerial));
	return status;
}

APS2_STATUS reset(const char* deviceSerial, int resetMode) {
	return aps2_call(deviceSerial, &APS2::reset, static_cast<APS_RESET_MODE_STAT>(resetMode));
}

//Initialize an APS unit
APS2_STATUS init_APS(const char* deviceSerial, int forceReload) {
	return aps2_call(deviceSerial, &APS2::init, bool(forceReload), 0);
}

APS2_STATUS get_firmware_version(const char* deviceSerial, uint32_t* version) {
	return aps2_getter(deviceSerial, &APS2::get_firmware_version, version);
}

APS2_STATUS get_uptime(const char* deviceSerial, double* upTime) {
	return aps2_getter(deviceSerial, &APS2::get_uptime, upTime);
}

APS2_STATUS get_fpga_temperature(const char* deviceSerial, double* temp) {
	return aps2_getter(deviceSerial, &APS2::get_fpga_temperature, temp);
}

APS2_STATUS set_sampleRate(const char* deviceSerial, unsigned int freq) {
	return aps2_call(deviceSerial, &APS2::set_sampleRate, freq);
}

APS2_STATUS get_sampleRate(const char* deviceSerial, unsigned int* freq) {
	return aps2_getter(deviceSerial, &APS2::get_sampleRate, freq);
}

//Load the waveform library as floats
APS2_STATUS set_waveform_float(const char* deviceSerial, int channelNum, float* data, int numPts) {
	//specialize the templated APS2::set_waveform here
	return aps2_call(deviceSerial,
						static_cast<void(APS2::*)(const int&, const vector<float>&)>(&APS2::set_waveform),
						channelNum, vector<float>(data, data+numPts));
}

//Load the waveform library as int16
APS2_STATUS set_waveform_int(const char* deviceSerial, int channelNum, int16_t* data, int numPts) {
	//specialize the templated APS2::set_waveform here
	return aps2_call(deviceSerial,
						static_cast<void(APS2::*)(const int&, const vector<int16_t>&)>(&APS2::set_waveform),
						channelNum, vector<int16_t>(data, data+numPts));
}

APS2_STATUS set_markers(const char* deviceSerial, int channelNum, uint8_t* data, int numPts) {
	return aps2_call(deviceSerial, &APS2::set_markers, channelNum, vector<uint8_t>(data, data+numPts));
}

APS2_STATUS write_sequence(const char* deviceSerial, uint64_t* data, uint32_t numWords) {
	return aps2_call(deviceSerial, &APS2::write_sequence, vector<uint64_t>(data, data+numWords));
}

APS2_STATUS load_sequence_file(const char* deviceSerial, const char* seqFile) {
	return aps2_call(deviceSerial, &APS2::load_sequence_file, string(seqFile));
}

APS2_STATUS clear_channel_data(const char* deviceSerial) {
	return aps2_call(deviceSerial, &APS2::clear_channel_data);
}

APS2_STATUS run(const char* deviceSerial) {
	return aps2_call(deviceSerial, &APS2::run);
}

APS2_STATUS stop(const char* deviceSerial) {
	return aps2_call(deviceSerial, &APS2::stop);
}

APS2_STATUS get_runState(const char* deviceSerial, RUN_STATE* state) {
	return aps2_getter(deviceSerial, &APS2::get_runState, state);
}

//Expects a null-terminated character array
APS2_STATUS set_log(const char* fileNameArr) {

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

APS2_STATUS set_trigger_source(const char* deviceSerial, TRIGGER_SOURCE src) {
	return aps2_call(deviceSerial, &APS2::set_trigger_source, src);
}

APS2_STATUS get_trigger_source(const char* deviceSerial, TRIGGER_SOURCE* src) {
	return aps2_getter(deviceSerial, &APS2::get_trigger_source, src);
}

APS2_STATUS set_trigger_interval(const char* deviceSerial, double interval) {
	return aps2_call(deviceSerial, &APS2::set_trigger_interval, interval);
}

APS2_STATUS get_trigger_interval(const char* deviceSerial, double* interval) {
	return aps2_getter(deviceSerial, &APS2::get_trigger_interval, interval);
}

APS2_STATUS trigger(const char* deviceSerial) {
	return aps2_call(deviceSerial, &APS2::trigger);
}

APS2_STATUS set_channel_offset(const char* deviceSerial, int channelNum, float offset) {
	return aps2_call(deviceSerial, &APS2::set_channel_offset, channelNum, offset);
}
APS2_STATUS set_channel_scale(const char* deviceSerial, int channelNum, float scale) {
	return aps2_call(deviceSerial, &APS2::set_channel_scale, channelNum, scale);
}
APS2_STATUS set_channel_enabled(const char* deviceSerial, int channelNum, int enable) {
	return aps2_call(deviceSerial, &APS2::set_channel_enabled, channelNum, enable);
}

APS2_STATUS get_channel_offset(const char* deviceSerial, int channelNum, float* offset) {
	return aps2_getter(deviceSerial, &APS2::get_channel_offset, offset, channelNum);
}
APS2_STATUS get_channel_scale(const char* deviceSerial, int channelNum, float* scale) {
	return aps2_getter(deviceSerial, &APS2::get_channel_scale, scale, channelNum);
}
APS2_STATUS get_channel_enabled(const char* deviceSerial, int channelNum, int* enabled) {
	return aps2_getter(deviceSerial, &APS2::get_channel_offset, enabled, channelNum);
}

APS2_STATUS set_run_mode(const char* deviceSerial, RUN_MODE mode) {
	return aps2_call(deviceSerial, &APS2::set_run_mode, mode);
}

int write_memory(const char* deviceSerial, uint32_t addr, uint32_t* data, uint32_t numWords) {
	return aps2_call(deviceSerial,
						static_cast<void(APS2::*)(const uint32_t&, const vector<uint32_t>&)>(&APS2::write_memory),
						addr, vector<uint32_t>(data, data + numWords));
}

int read_memory(const char* deviceSerial, uint32_t addr, uint32_t* data, uint32_t numWords) {
	auto readData = APSs[string(deviceSerial)].read_memory(addr, numWords);
	std::copy(readData.begin(), readData.end(), data);
	return 0;
}

int read_register(const char* deviceSerial, uint32_t addr) {
	uint32_t buffer[1];
	read_memory(deviceSerial, addr, buffer, 1);
	return buffer[0];
}

int program_FPGA(const char* deviceSerial, const char* bitFile) {
	return aps2_call(deviceSerial, &APS2::program_FPGA, string(bitFile));
}

int write_flash(const char* deviceSerial, uint32_t addr, uint32_t* data, uint32_t numWords) {
	return aps2_call(deviceSerial, &APS2::write_flash, addr, vector<uint32_t>(data, data + numWords));
}

int read_flash(const char* deviceSerial, uint32_t addr, uint32_t numWords, uint32_t* data) {
	auto readData = APSs[string(deviceSerial)].read_flash(addr, numWords);
	std::copy(readData.begin(), readData.end(), data);
	return 0;
}

uint64_t get_mac_addr(const char* deviceSerial) {
	return APSs[string(deviceSerial)].get_mac_addr();
}

APS2_STATUS set_mac_addr(const char* deviceSerial, uint64_t mac) {
	return aps2_call(deviceSerial, &APS2::set_mac_addr, mac);
}

APS2_STATUS get_ip_addr(const char* deviceSerial, char* ipAddrPtr) {
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

APS2_STATUS set_ip_addr(const char* deviceSerial, const char* ipAddrStr) {
	uint32_t ipAddr = asio::ip::address_v4::from_string(ipAddrStr).to_ulong();
	return aps2_call(deviceSerial, &APS2::set_ip_addr, ipAddr);
}

APS2_STATUS write_SPI_setup(const char* deviceSerial) {
	return aps2_call(deviceSerial, &APS2::write_SPI_setup);
}

APS2_STATUS get_dhcp_enable(const char* deviceSerial, int * enabled) {
	return aps2_getter(deviceSerial, &APS2::get_dhcp_enable, enabled);
}

APS2_STATUS set_dhcp_enable(const char* deviceSerial, const int enable) {
	return aps2_call(deviceSerial, &APS2::set_dhcp_enable, enable);
}

int run_DAC_BIST(const char* deviceSerial, const int dac, int16_t* data, unsigned int length, uint32_t* results){
	vector<int16_t> testVec(data, data+length);
	vector<uint32_t> tmpResults;
	int passed = APSs[string(deviceSerial)].run_DAC_BIST(dac, testVec, tmpResults);
	std::copy(tmpResults.begin(), tmpResults.end(), results);
	return passed;
}

APS2_STATUS set_DAC_SD(const char* deviceSerial, const int dac, const uint8_t sd){
	return aps2_call(deviceSerial, &APS2::set_DAC_SD, dac, sd);
}

#ifdef __cplusplus
}
#endif
