#ifndef APS2_ERRNO_H_
#define APS2_ERRNO_H_

enum APS2_STATUS {
	APS2_OK,
	APS2_UNKNOWN_ERROR = -1,
	APS2_NO_DEVICE_FOUND = -2,
	APS2_UNCONNECTED = -3,
	APS2_RESET_TIMEOUT =-4,
	APS2_FILELOG_ERROR = -5,
	APS2_SEQFILE_FAIL = -6,
	APS2_PLL_LOST_LOCK = -7,
	APS2_MMCM_LOST_LOCK = -8,
	APS2_UNKNOWN_RUN_MODE = -9,
	APS2_FAILED_TO_CONNECT = -10,
	APS2_INVALID_DAC = -11,
	APS2_NO_SUCH_BITFILE = -12,
	APS2_MAC_ADDR_VALIDATION_FAILURE = -13,
	APS2_IP_ADDR_VALIDATION_FAILURE = -14,
	APS2_DHCP_VALIDATION_FAILURE = -15
};


#ifdef __cplusplus

#include <map>
#include <string>

static std::map<APS2_STATUS, std::string> messages = {
	{APS2_UNCONNECTED, "Attempt to run library function on unconnected APS2"},
	{APS2_SEQFILE_FAIL, "Failed to load HDF5 sequence file. Check it is present and correctly formatted."},
	{APS2_FILELOG_ERROR, "Unable to open log file."},
	{APS2_PLL_LOST_LOCK, "The PLL chip has lost its lock.  Try power cycling the module."},
	{APS2_MMCM_LOST_LOCK, "The FPGA MMCM has lost its lock.  Try resetting the module."},
	{APS2_UNKNOWN_RUN_MODE, "Unknown APS2 run mode.  Available options are RUN_SEQUENCE, TRIG_WAVEFORM, CW_WAVEFORM"},
	{APS2_FAILED_TO_CONNECT, "Unable to connect to requested APS2.  Make sure it is connected to network."},
	{APS2_INVALID_DAC, "API call requested invalide DAC channel.  Only 2 channels per module."},
	{APS2_NO_SUCH_BITFILE, "Could not find bitfile at location specified."},
	{APS2_MAC_ADDR_VALIDATION_FAILURE, "Failed to validate the update to the MAC address in flash memory."},
	{APS2_IP_ADDR_VALIDATION_FAILURE, "Failed to validate the update to the IP address in flash memory."},
	{APS2_DHCP_VALIDATION_FAILURE, "Failed to validate the update to the DHCP enable bit in flash memory."}
};


#endif

#endif
