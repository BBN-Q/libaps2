#ifndef APS2_ERRNO_H_
#define APS2_ERRNO_H_

enum APS2_STATUS {
  APS2_OK,
  APS2_UNKNOWN_ERROR = -1,
  APS2_NO_DEVICE_FOUND = -2,
  APS2_UNCONNECTED = -3,
  APS2_RESET_TIMEOUT = -4,
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
  APS2_DHCP_VALIDATION_FAILURE = -15,
  APS2_RECEIVE_TIMEOUT = -16,
  APS2_SOCKET_FAILURE = -17,
  APS2_INVALID_IP_ADDR = -18,
  APS2_COMMS_ERROR = -19,
  APS2_UNALIGNED_MEMORY_ACCESS = -20,
  APS2_ERPOM_ERASE_FAILURE = -21,
  APS2_BITFILE_VALIDATION_FAILURE = -22,
  APS2_BAD_PLL_VALUE = -23,
  APS2_NO_WFS = -24,
  APS2_WAVEFORM_FREQ_OVERFLOW = -25
};

#ifdef __cplusplus

#include <map>
#include <string>

static std::map<APS2_STATUS, std::string> messages = {
    {APS2_NO_DEVICE_FOUND, "Device failed to respond at requested IP address"},
    {APS2_UNCONNECTED, "Attempt to run library function on unconnected APS2"},
    {APS2_SEQFILE_FAIL, "Failed to load HDF5 sequence file. Check it is "
                        "present and correctly formatted."},
    {APS2_FILELOG_ERROR, "Unable to open log file."},
    {APS2_PLL_LOST_LOCK,
     "The PLL chip has lost its lock.  Try power cycling the module."},
    {APS2_MMCM_LOST_LOCK,
     "The FPGA MMCM has lost its lock.  Try resetting the module."},
    {APS2_UNKNOWN_RUN_MODE, "Unknown APS2 run mode.  Available options are "
                            "RUN_SEQUENCE, TRIG_WAVEFORM, CW_WAVEFORM"},
    {APS2_FAILED_TO_CONNECT, "Unable to connect to requested APS2.  Make sure "
                             "it is connected to network and that no other "
                             "system is connected."},
    {APS2_INVALID_DAC,
     "API call requested invalide DAC channel.  Only 2 channels per module."},
    {APS2_NO_SUCH_BITFILE, "Could not find bitfile at location specified."},
    {APS2_MAC_ADDR_VALIDATION_FAILURE,
     "Failed to validate the update to the MAC address in flash memory."},
    {APS2_IP_ADDR_VALIDATION_FAILURE,
     "Failed to validate the update to the IP address in flash memory."},
    {APS2_DHCP_VALIDATION_FAILURE,
     "Failed to validate the update to the DHCP enable bit in flash memory."},
    {APS2_RECEIVE_TIMEOUT, "Timed out while waiting to receive data."},
    {APS2_SOCKET_FAILURE, "Failed to open ethernet socket. Verify that no "
                          "dangling libaps2 processes are running."},
    {APS2_INVALID_IP_ADDR, "Requested conneciton to invalid IPv4 address."},
    {APS2_COMMS_ERROR, "Ethernet communications failed."},
    {APS2_UNALIGNED_MEMORY_ACCESS,
     "SDRAM memory must be accessed at 8 (configuration SDRAM) or 16 "
     "(sequence/waveform SDRAM) or 64k (flash erase) or 256 (flash write) byte "
     "boundaries."},
    {APS2_ERPOM_ERASE_FAILURE, "ERPOM erase command failed"},
    {APS2_BITFILE_VALIDATION_FAILURE, "bitfile validation failed"},
    {APS2_BAD_PLL_VALUE, "Unexpected PLL chip value"},
    {APS2_NO_WFS, "Asked for waveform mode with no waveforms loaded"},
    {APS2_WAVEFORM_FREQ_OVERFLOW,
     "Waveform frequency must be in range [-600MHz, 600MHz)"}};

#endif

#endif
