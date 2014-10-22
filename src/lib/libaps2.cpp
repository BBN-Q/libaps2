/*
 * libaps.cpp - Library for APSv2 control. 
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

shared_ptr<APSEthernet> get_interface() {
	//See if we have to setup our own RM
	shared_ptr<APSEthernet> myEthernetRM = ethernetRM.lock();

	if (!myEthernetRM) {
		myEthernetRM = std::make_shared<APSEthernet>();
		ethernetRM = myEthernetRM;
	}

	return myEthernetRM;
}

//Handle exceptions thrown in library calls
APS2_STATUS make_errno(std::exception_ptr e){
	FILE_LOG(logERROR) << "Oops!";
	return APS2_UNKNOWN_ERROR;
}

//It seems Args1 and Args2 should match but I couldn't get the parameter pack template deduction to work
template<typename R, typename... Args1, typename... Args2>
APS2_STATUS aps2_call(const char * deviceSerial, std::function<R(Args1...)> func, Args2&&... args){
	try{
		func(APSs[deviceSerial], std::forward<Args2>(args)...);
		//Nothing thrown then assume OK
		return APS2_OK;
	}
	catch(...) {
		return make_errno(std::current_exception());
	}
}

template<typename R, typename... Args1, typename... Args2>
APS2_STATUS aps2_getter(const char * deviceSerial, std::function<R(Args1...)> func, R *resPtr,  Args2&&... args){
	try{
		*resPtr = func(APSs[deviceSerial], std::forward<Args2>(args)...);
		//Nothing thrown then assume OK
		return APS2_OK;
	}
	catch(...) {
		return make_errno(std::current_exception());
	}
}

#ifdef __cplusplus
extern "C" {
#endif

APS2_STATUS get_numDevices(unsigned int * numDevices) {
	/*
	Returns the number of APS2s that respond to a broadcast status request.
	*/
	deviceSerials = get_interface()->enumerate();
	*numDevices = deviceSerials.size();
	return APS2_OK;
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
	Assumes null-terminated deviceSerial
	*/
	string serial = string(deviceSerial);
	// create the APS2 object if it is not already in the map
	if (APSs.find(serial) == APSs.end()) {
		APSs[serial] = APS2(serial);
	}
	return APSs[serial].connect(get_interface());
}

//Assumes a null-terminated deviceSerial
APS2_STATUS disconnect_APS(const char * deviceSerial) {
	APS2_STATUS result = APSs[string(deviceSerial)].disconnect();
	APSs.erase(string(deviceSerial));
	return result;
}

int reset(const char * deviceSerial, int resetMode) {
	return APSs[string(deviceSerial)].reset(static_cast<APS_RESET_MODE_STAT>(resetMode));
}

//Initialize an APS unit
//Assumes null-terminated bitFile
int initAPS(const char * deviceSerial, int forceReload) {
	aps2_call(deviceSerial, std::function<APS2_STATUS(APS2&, const bool&, const int&)>(&APS2::init), bool(forceReload), 0);
	return 0;
	// return APSs[string(deviceSerial)].init(forceReload);
}

int get_firmware_version(const char * deviceSerial) {
	int silly;
	APS2_STATUS status = aps2_getter(deviceSerial, std::function<int(APS2&)>(&APS2::get_firmware_version), &silly);
	cout << silly; 
	return 0;
}

double get_uptime(const char * deviceSerial) {
	return APSs[string(deviceSerial)].get_uptime();
}

int set_sampleRate(const char * deviceSerial, int freq) {
	return APSs[string(deviceSerial)].set_sampleRate(freq);
}

int get_sampleRate(const char * deviceSerial) {
	return APSs[string(deviceSerial)].get_sampleRate();
}

//Load the waveform library as floats
int set_waveform_float(const char * deviceSerial, int channelNum, float* data, int numPts) {
	return APSs[string(deviceSerial)].set_waveform( channelNum, vector<float>(data, data+numPts));
}

//Load the waveform library as int16
int set_waveform_int(const char * deviceSerial, int channelNum, int16_t* data, int numPts) {
	return APSs[string(deviceSerial)].set_waveform(channelNum, vector<int16_t>(data, data+numPts));
}

int set_markers(const char * deviceSerial, int channelNum, uint8_t* data, int numPts) {
	return APSs[string(deviceSerial)].set_markers(channelNum, vector<uint8_t>(data, data+numPts));
}

int write_sequence(const char * deviceSerial, uint64_t* data, uint32_t numWords) {
	vector<uint64_t> dataVec(data, data+numWords);
	return APSs[string(deviceSerial)].write_sequence(dataVec);
}

int load_sequence_file(const char * deviceSerial, const char * seqFile) {
	try {
		return APSs[string(deviceSerial)].load_sequence_file(string(seqFile));
	} catch (...) {
		return APS2_UNKNOWN_ERROR;
	}
	// should not reach this point
	return APS2_UNKNOWN_ERROR;
}

int clear_channel_data(const char * deviceSerial) {
	return APSs[string(deviceSerial)].clear_channel_data();
}

int run(const char * deviceSerial) {
	return APSs[string(deviceSerial)].run();
}

int stop(const char * deviceSerial) {
	return APSs[string(deviceSerial)].stop();
}

int get_running(const char * deviceSerial) {
	return APSs[string(deviceSerial)].running;
}

//Expects a null-terminated character array
int set_log(const char * fileNameArr) {

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
			return APS2_FILE_ERROR;
		}
		Output2FILE::Stream() = pFile;
		return APS2_OK;
	}
}

int set_logging_level(int logLevel) {
	FILELog::ReportingLevel() = TLogLevel(logLevel);
	return APS2_OK;
}

int set_trigger_source(const char * deviceSerial, int triggerSource) {
	return APSs[string(deviceSerial)].set_trigger_source(TRIGGERSOURCE(triggerSource));
}

int get_trigger_source(const char * deviceSerial) {
	return int(APSs[string(deviceSerial)].get_trigger_source());
}

int set_trigger_interval(const char * deviceSerial, double interval) {
	return APSs[string(deviceSerial)].set_trigger_interval(interval);
}

double get_trigger_interval(const char * deviceSerial) {
	return APSs[string(deviceSerial)].get_trigger_interval();
}

int trigger(const char* deviceSerial) {
	return APSs[string(deviceSerial)].trigger();
}

int set_channel_offset(const char * deviceSerial, int channelNum, float offset) {
	return APSs[string(deviceSerial)].set_channel_offset(channelNum, offset);
}
int set_channel_scale(const char * deviceSerial, int channelNum, float scale) {
	return APSs[string(deviceSerial)].set_channel_scale(channelNum, scale);
}
int set_channel_enabled(const char * deviceSerial, int channelNum, int enable) {
	return APSs[string(deviceSerial)].set_channel_enabled(channelNum, enable);
}

float get_channel_offset(const char * deviceSerial, int channelNum) {
	return APSs[string(deviceSerial)].get_channel_offset(channelNum);
}
float get_channel_scale(const char * deviceSerial, int channelNum) {
	return APSs[string(deviceSerial)].get_channel_scale(channelNum);
}
int get_channel_enabled(const char * deviceSerial, int channelNum) {
	return APSs[string(deviceSerial)].get_channel_enabled(channelNum);
}

int set_run_mode(const char * deviceSerial, int mode) {
	return APSs[string(deviceSerial)].set_run_mode(RUN_MODE(mode));
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
const char * get_ip_addr(const char * deviceSerial) {
	uint32_t ip_addr = APSs[string(deviceSerial)].get_ip_addr();
	return asio::ip::address_v4(ip_addr).to_string().c_str();
}
int set_ip_addr(const char * deviceSerial, const char * ip_addr_str) {
	uint32_t ip_addr = asio::ip::address_v4::from_string(ip_addr_str).to_ulong();
	return APSs[string(deviceSerial)].set_ip_addr(ip_addr);
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

