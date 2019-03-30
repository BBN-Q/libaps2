/*
 * libaps.cpp - C API shared library for APSv2 control.
 *
 */

#include <memory>
#include <sstream>
using std::weak_ptr;
#include <map>
using std::map;

#include "APS2.h"
#include "APS2Ethernet.h"
#include "asio.hpp"
#include "libaps2.h"
#include "version.hpp"

#define FILE_LOG 1
#define CONSOLE_LOG 2

weak_ptr<APS2Ethernet>
    ethernetRM; // resource manager for the asio ethernet interface
map<string, std::unique_ptr<APS2>> APSs; // map to hold on to the APS instances
set<string>
    deviceSerials; // set of APSs that responded to an enumerate broadcast

// stub class to open loggers
class InitAndCleanUp {
public:
  InitAndCleanUp();
};

InitAndCleanUp::InitAndCleanUp() {
  //TODO: change log file path
  if (!plog::get()) {
    static plog::RollingFileAppender<plog::TxtFormatter> fileAppender("libaps2.log", 1000000, 3);
    plog::init<FILE_LOG>(plog::info, &fileAppender);
    static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
    plog::init<CONSOLE_LOG>(plog::warning, &consoleAppender);
    plog::init(plog::verbose).addAppender(plog::get<FILE_LOG>()).addAppender(plog::get<CONSOLE_LOG>());
  }

  //make sure it was created correctly
  if (!plog::get()){
    std::cout << "Was unable to create a logger for libaps2! Exiting..." << std::endl;
    throw APS2_FILELOG_ERROR; 
  }

  LOG(plog::info) << "libaps2 driver version: " << get_driver_version();
}

static InitAndCleanUp initandcleanup_;

// Return the shared_ptr to the Ethernet interface
shared_ptr<APS2Ethernet> get_interface() {
  // See if we have to setup our own RM
  shared_ptr<APS2Ethernet> myEthernetRM = ethernetRM.lock();

  if (!myEthernetRM) {
    try {
      myEthernetRM = std::make_shared<APS2Ethernet>();
    } catch (APS2_STATUS e) {
      LOG(plog::error) << "Failed to create ethernet interface.";
      throw;
    } catch (std::system_error e) {
      LOG(plog::error) << "Unexpected error creating ethernet interface. Msg: " << e.what();
      throw APS2_UNKNOWN_ERROR;
    }
    ethernetRM = myEthernetRM;
  }

  return myEthernetRM;
}

// Define a couple of templated wrapper functions to make library calls and
// catch thrown errors
// First one for void calls
template <typename F, typename... Args>
APS2_STATUS aps2_call(const char *deviceSerial, F func, Args... args) {
  try {
    // for some reason with "(APSs.at(deviceSerial)->*func)(args...);" the
    // compiler doesn't like "->*"
    (*(APSs.at(deviceSerial)).*func)(args...);
    // Nothing thrown then assume OK
    return APS2_OK;
  } catch (std::out_of_range e) {
    if (APSs.find(deviceSerial) == APSs.end()) {
      return APS2_UNCONNECTED;
    } else {
      return APS2_UNKNOWN_ERROR;
    }
  } catch (APS2_STATUS status) {
    return status;
  } catch (...) {
    return APS2_UNKNOWN_ERROR;
  }
}

// and one for to store getter values in pointer passed to library
template <typename R, typename F, typename... Args>
APS2_STATUS aps2_getter(const char *deviceSerial, F func, R *resPtr,
                        Args... args) {
  try {
    *resPtr = (*(APSs.at(deviceSerial)).*func)(args...);
    // Nothing thrown then assume OK
    return APS2_OK;
  } catch (std::out_of_range e) {
    if (APSs.find(deviceSerial) == APSs.end()) {
      return APS2_UNCONNECTED;
    } else {
      return APS2_UNKNOWN_ERROR;
    }
  } catch (APS2_STATUS status) {
    return status;
  } catch (...) {
    return APS2_UNKNOWN_ERROR;
  }
}

#ifdef __cplusplus
extern "C" {
#endif

const char *get_error_msg(APS2_STATUS err) {
  if (messages.count(err)) {
    return messages[err].c_str();
  } else {
    return "No error message for this status number.";
  }
}

APS2_STATUS get_numDevices(unsigned int *numDevices) {
  /*
  Returns the number of APS2s that respond to a broadcast status request.
  */
  try {
    deviceSerials = get_interface()->enumerate();
    *numDevices = deviceSerials.size();
    return APS2_OK;
  } catch (APS2_STATUS status) {
    return status;
  } catch (...) {
    return APS2_UNKNOWN_ERROR;
  }
}

APS2_STATUS get_device_IPs(const char **deviceSerialsOut) {
  /*
  Fill in an array of char* with null-terminated char arrays with the
  enumerated device ip addresses.
  Assumes sufficient memory has been allocated
  */
  size_t ct = 0;
  for (auto &serial : deviceSerials) {
    deviceSerialsOut[ct] = serial.c_str();
    ct++;
  }
  return APS2_OK;
}

APS2_STATUS connect_APS(const char *deviceSerial) {
  /*
  Connect to a device specified by serial number string
  */
  string serial = string(deviceSerial);
  // create the APS2 object if it is not already in the map
  if (APSs.find(serial) == APSs.end()) {
    // C++14 use std::make_unique
    APSs.insert(
        std::make_pair(serial, std::unique_ptr<APS2>(new APS2(serial))));
  }
  // Can't seem to bind the interface lvalue to
  // ‘std::shared_ptr<APS2Ethernet>&&’
  // return aps2_call(deviceSerial, &APS2::connect, get_interface());
  try {
    APSs.at(deviceSerial)->connect(get_interface());
    return APS2_OK;
  } catch (APS2_STATUS status) {
    return status;
  } catch (...) {
    return APS2_UNKNOWN_ERROR;
  }
}

APS2_STATUS disconnect_APS(const char *deviceSerial) {
  /*
  Tear-down connection to APS specified by serial number string.
  */
  APS2_STATUS status = aps2_call(deviceSerial, &APS2::disconnect);
  APSs.erase(string(deviceSerial));
  return status;
}

APS2_STATUS reset(const char *deviceSerial, APS2_RESET_MODE mode) {
  switch (mode) {
  case RESET_TCP:
    // For TCP we may not have an APS2 device (we can't connect) so directly
    // call APS2Ethernet method
    get_interface()->reset_tcp(deviceSerial);
    return APS2_OK;
  default:
    return aps2_call(deviceSerial, &APS2::reset, mode);
  }
}

// Initialize an APS unit
APS2_STATUS init_APS(const char *deviceSerial, int forceReload) {
  return aps2_call(deviceSerial, &APS2::init, bool(forceReload), 0);
}

APS2_STATUS get_firmware_version(const char *ipAddr, uint32_t *version,
                                 uint32_t *git_sha1, uint32_t *build_timestamp,
                                 char *version_string) {
  APS2_STATUS status = APS2_OK;
  if (version != nullptr) {
    status = aps2_getter(ipAddr, &APS2::get_firmware_version, version);
    if (status != APS2_OK) {
      return status;
    }
  }
  if (git_sha1 != nullptr) {
    status = aps2_getter(ipAddr, &APS2::get_firmware_git_sha1, git_sha1);
    if (status != APS2_OK) {
      return status;
    }
  }
  if (build_timestamp != nullptr) {
    status = aps2_getter(ipAddr, &APS2::get_firmware_build_timestamp,
                         build_timestamp);
    if (status != APS2_OK) {
      return status;
    }
  }
  if (version_string != nullptr) {
    uint32_t my_version;
    uint32_t my_git_sha1;
    uint32_t my_build_timestamp;
    if (version == nullptr) {
      status = aps2_getter(ipAddr, &APS2::get_firmware_version, &my_version);
      if (status != APS2_OK) {
        return status;
      }
    } else {
      my_version = *version;
    }
    if (git_sha1 == nullptr) {
      status = aps2_getter(ipAddr, &APS2::get_firmware_git_sha1, &my_git_sha1);
      if (status != APS2_OK) {
        return status;
      }
    } else {
      my_git_sha1 = *git_sha1;
    }
    if (build_timestamp == nullptr) {
      status = aps2_getter(ipAddr, &APS2::get_firmware_build_timestamp,
                           &my_build_timestamp);
      if (status != APS2_OK) {
        return status;
      }
    } else {
      my_build_timestamp = *build_timestamp;
    }
    // put together the version string
    unsigned tag_minor = my_version & 0xff;
    unsigned tag_major = (my_version >> 8) & 0xff;
    unsigned commits_since = (my_version >> 16) & 0xfff;
    bool is_dirty = ((my_version >> 28) & 0xf) == 0xd;
    std::ostringstream version_stream;
    version_stream << "v" << tag_major << "." << tag_minor;
    if (commits_since > 0) {
      version_stream << "-" << commits_since << "-g" << std::hex << my_git_sha1
                     << std::dec;
    }
    if (is_dirty) {
      version_stream << "-dirty";
    }
    unsigned year = (my_build_timestamp >> 24) & 0xff;
    unsigned month = (my_build_timestamp >> 16) & 0xff;
    unsigned day = (my_build_timestamp >> 8) & 0xff;
    version_stream << " 20" << std::hex << year << "-" << month << "-" << day;
    const string tmp_string = version_stream.str();
    std::strcpy(version_string, tmp_string.c_str());
  }
  return status;
}

APS2_STATUS get_uptime(const char *deviceSerial, double *upTime) {
  return aps2_getter(deviceSerial, &APS2::get_uptime, upTime);
}

APS2_STATUS get_fpga_temperature(const char *deviceSerial, float *temp) {
  return aps2_getter(deviceSerial, &APS2::get_fpga_temperature, temp);
}

APS2_STATUS set_sampleRate(const char *deviceSerial, unsigned int freq) {
  return aps2_call(deviceSerial, &APS2::set_sampleRate, freq);
}

APS2_STATUS get_sampleRate(const char *deviceSerial, unsigned int *freq) {
  return aps2_getter(deviceSerial, &APS2::get_sampleRate, freq);
}

// Load the waveform library as floats
APS2_STATUS set_waveform_float(const char *deviceSerial, int channelNum,
                               float *data, int numPts) {
  // specialize the templated APS2::set_waveform here
  return aps2_call(
      deviceSerial,
      static_cast<void (APS2::*)(const int &, const vector<float> &)>(
          &APS2::set_waveform),
      channelNum, vector<float>(data, data + numPts));
}

// Load the waveform library as int16
APS2_STATUS set_waveform_int(const char *deviceSerial, int channelNum,
                             int16_t *data, int numPts) {
  // specialize the templated APS2::set_waveform here
  return aps2_call(
      deviceSerial,
      static_cast<void (APS2::*)(const int &, const vector<int16_t> &)>(
          &APS2::set_waveform),
      channelNum, vector<int16_t>(data, data + numPts));
}

APS2_STATUS set_markers(const char *deviceSerial, int channelNum, uint8_t *data,
                        int numPts) {
  return aps2_call(deviceSerial, &APS2::set_markers, channelNum,
                   vector<uint8_t>(data, data + numPts));
}

APS2_STATUS write_sequence(const char *deviceSerial, uint64_t *data,
                           uint32_t numWords) {
  return aps2_call(deviceSerial, &APS2::write_sequence,
                   vector<uint64_t>(data, data + numWords));
}

APS2_STATUS load_sequence_file(const char *deviceSerial, const char *seqFile) {
  return aps2_call(deviceSerial, &APS2::load_sequence_file, string(seqFile));
}


APS2_STATUS clear_channel_data(const char *deviceSerial) {
  return aps2_call(deviceSerial, &APS2::clear_channel_data);
}

APS2_STATUS run(const char *deviceSerial) {
  return aps2_call(deviceSerial, &APS2::run);
}

APS2_STATUS stop(const char *deviceSerial) {
  return aps2_call(deviceSerial, &APS2::stop);
}

APS2_STATUS get_runState(const char *deviceSerial, APS2_RUN_STATE *state) {
  return aps2_getter(deviceSerial, &APS2::get_runState, state);
}

// Expects a null-terminated character array
APS2_STATUS set_log(const char *fileNameArr) {
  //TODO: Fixme?
  
  LOG(plog::warning) << "The plog logger cannot change the log file name.";
  return APS2_OK;

  // // Close the current file
  // if (Output2FILE::Stream())
  //   fclose(Output2FILE::Stream());

  // string fileName(fileNameArr);
  // if (fileName.compare("stdout") == 0) {
  //   Output2FILE::Stream() = stdout;
  //   return APS2_OK;
  // } else if (fileName.compare("stderr") == 0) {
  //   Output2FILE::Stream() = stdout;
  //   return APS2_OK;
  // } else {
  //   FILE *pFile = fopen(fileName.c_str(), "a");
  //   if (!pFile) {
  //     return APS2_FILELOG_ERROR;
  //   }
  //   Output2FILE::Stream() = pFile;
  //   return APS2_OK;
  // }
}

APS2_STATUS set_file_logging_level(int severity) {
  plog::get<FILE_LOG>()->setMaxSeverity(static_cast<plog::Severity>(severity));
  return APS2_OK;
}

APS2_STATUS set_console_logging_level(int severity) {
  plog::get<CONSOLE_LOG>()->setMaxSeverity(static_cast<plog::Severity>(severity));
  return APS2_OK;
}

APS2_STATUS set_trigger_source(const char *deviceSerial,
                               APS2_TRIGGER_SOURCE src) {
  return aps2_call(deviceSerial, &APS2::set_trigger_source, src);
}

APS2_STATUS get_trigger_source(const char *deviceSerial,
                               APS2_TRIGGER_SOURCE *src) {
  return aps2_getter(deviceSerial, &APS2::get_trigger_source, src);
}

APS2_STATUS set_trigger_interval(const char *deviceSerial, double interval) {
  return aps2_call(deviceSerial, &APS2::set_trigger_interval, interval);
}

APS2_STATUS get_trigger_interval(const char *deviceSerial, double *interval) {
  return aps2_getter(deviceSerial, &APS2::get_trigger_interval, interval);
}

APS2_STATUS trigger(const char *deviceSerial) {
  return aps2_call(deviceSerial, &APS2::trigger);
}

APS2_STATUS set_channel_delay(const char *deviceSerial, int channelNum,
                               unsigned delay) {
  return aps2_call(deviceSerial, &APS2::set_channel_bitslip, channelNum, delay);
}

APS2_STATUS set_channel_offset(const char *deviceSerial, int channelNum,
                               float offset) {
  return aps2_call(deviceSerial, &APS2::set_channel_offset, channelNum, offset);
}
APS2_STATUS set_channel_scale(const char *deviceSerial, int channelNum,
                              float scale) {
  return aps2_call(deviceSerial, &APS2::set_channel_scale, channelNum, scale);
}

APS2_STATUS set_channel_enabled(const char *deviceSerial, int channelNum,
                                int enable) {
  return aps2_call(deviceSerial, &APS2::set_channel_enabled, channelNum,
                   enable);
}

APS2_STATUS get_channel_delay(const char *deviceSerial, int channelNum,
                               unsigned *delay) {
  return aps2_getter(deviceSerial, &APS2::get_channel_bitslip, delay,
                     channelNum);
}

APS2_STATUS get_channel_offset(const char *deviceSerial, int channelNum,
                               float *offset) {
  return aps2_getter(deviceSerial, &APS2::get_channel_offset, offset,
                     channelNum);
}
APS2_STATUS get_channel_scale(const char *deviceSerial, int channelNum,
                              float *scale) {
  return aps2_getter(deviceSerial, &APS2::get_channel_scale, scale, channelNum);
}
APS2_STATUS get_channel_enabled(const char *deviceSerial, int channelNum,
                                int *enabled) {
  return aps2_getter(deviceSerial, &APS2::get_channel_offset, enabled,
                     channelNum);
}

APS2_STATUS set_mixer_amplitude_imbalance(const char *deviceSerial, float amp) {
  return aps2_call(deviceSerial, &APS2::set_mixer_amplitude_imbalance, amp);
}

APS2_STATUS get_mixer_amplitude_imbalance(const char *deviceSerial,
                                          float *amp) {
  return aps2_getter(deviceSerial, &APS2::get_mixer_amplitude_imbalance, amp);
}

APS2_STATUS set_mixer_phase_skew(const char *deviceSerial, float skew) {
  return aps2_call(deviceSerial, &APS2::set_mixer_phase_skew, skew);
}

APS2_STATUS get_mixer_phase_skew(const char *deviceSerial, float *skew) {
  return aps2_getter(deviceSerial, &APS2::get_mixer_phase_skew, skew);
}

APS2_STATUS set_mixer_correction_matrix(const char *deviceSerial, float *mat) {
  return aps2_call(deviceSerial, &APS2::set_mixer_correction_matrix,
                   vector<float>(mat, mat + 4));
}

APS2_STATUS get_mixer_correction_matrix(const char *deviceSerial, float *mat) {
  try {
    auto correction_mat =
        APSs[string(deviceSerial)]->get_mixer_correction_matrix();
    std::copy(correction_mat.begin(), correction_mat.end(), mat);
  } catch (APS2_STATUS status) {
    return status;
  } catch (...) {
    return APS2_UNKNOWN_ERROR;
  }
  return APS2_OK;
}

APS2_STATUS set_run_mode(const char *deviceSerial, APS2_RUN_MODE mode) {
  return aps2_call(deviceSerial, &APS2::set_run_mode, mode);
}

APS2_STATUS set_waveform_frequency(const char *deviceSerial, float freq) {
  return aps2_call(deviceSerial, &APS2::set_waveform_frequency, freq);
}

APS2_STATUS get_waveform_frequency(const char *deviceSerial, float *freq) {
  return aps2_getter(deviceSerial, &APS2::get_waveform_frequency, freq);
}

APS2_STATUS write_memory(const char *deviceSerial, uint32_t addr,
                         uint32_t *data, uint32_t numWords) {
  return aps2_call(
      deviceSerial,
      static_cast<void (APS2::*)(const uint32_t &, const vector<uint32_t> &)>(
          &APS2::write_memory),
      addr, vector<uint32_t>(data, data + numWords));
}

APS2_STATUS read_memory(const char *deviceSerial, uint32_t addr, uint32_t *data,
                        uint32_t numWords) {
  try {
    auto readData = APSs[string(deviceSerial)]->read_memory(addr, numWords);
    std::copy(readData.begin(), readData.end(), data);
  } catch (APS2_STATUS status) {
    return status;
  } catch (...) {
    return APS2_UNKNOWN_ERROR;
  }

  return APS2_OK;
}

APS2_STATUS read_register(const char *deviceSerial, uint32_t addr,
                          uint32_t *result) {
  return read_memory(deviceSerial, addr, result, 1);
}

APS2_STATUS write_bitfile(const char *deviceSerial, const char *bitFile,
                          uint32_t addr, APS2_BITFILE_STORAGE_MEDIA media) {
  return aps2_call(deviceSerial, &APS2::write_bitfile, string(bitFile), addr,
                   media);
}

APS2_STATUS program_bitfile(const char *deviceSerial, uint32_t addr) {
  return aps2_call(deviceSerial, &APS2::program_bitfile, addr);
}

APS2_STATUS write_configuration_SDRAM(const char *ip_addr, uint32_t addr,
                                      uint32_t *data, uint32_t num_words) {
  return aps2_call(ip_addr, &APS2::write_configuration_SDRAM, addr,
                   vector<uint32_t>(data, data + num_words));
}

APS2_STATUS read_configuration_SDRAM(const char *ip_addr, uint32_t addr,
                                     uint32_t num_words, uint32_t *data) {
  try {
    auto read_data =
        APSs[string(ip_addr)]->read_configuration_SDRAM(addr, num_words);
    std::copy(read_data.begin(), read_data.end(), data);
  } catch (APS2_STATUS status) {
    return status;
  } catch (...) {
    return APS2_UNKNOWN_ERROR;
  }
  return APS2_OK;
}

APS2_STATUS write_flash(const char *deviceSerial, uint32_t addr, uint32_t *data,
                        uint32_t numWords) {
  return aps2_call(deviceSerial, &APS2::write_flash, addr,
                   vector<uint32_t>(data, data + numWords));
}

APS2_STATUS read_flash(const char *deviceSerial, uint32_t addr,
                       uint32_t numWords, uint32_t *data) {
  try {
    auto readData = APSs[string(deviceSerial)]->read_flash(addr, numWords);
    std::copy(readData.begin(), readData.end(), data);
  } catch (APS2_STATUS status) {
    return status;
  } catch (...) {
    return APS2_UNKNOWN_ERROR;
  }
  return APS2_OK;
}

APS2_BITFILE_WRITING_TASK get_bitfile_writing_task(const char *deviceSerial) {
  return APSs[string(deviceSerial)]->bitfile_writing_task;
}

void clear_bitfile_writing_progress(const char *deviceSerial) {
  APSs[string(deviceSerial)]->bitfile_writing_task = STARTING;
  APSs[string(deviceSerial)]->bitfile_writing_task_progress = 0;
}

double get_flash_progress(const char *deviceSerial) {
  return APSs[string(deviceSerial)]->bitfile_writing_task_progress;
}

uint64_t get_mac_addr(const char *deviceSerial) {
  return APSs[string(deviceSerial)]->get_mac_addr();
}

APS2_STATUS set_mac_addr(const char *deviceSerial, uint64_t mac) {
  return aps2_call(deviceSerial, &APS2::set_mac_addr, mac);
}

APS2_STATUS get_ip_addr(const char *deviceSerial, char *ipAddrPtr) {
  try {
    uint32_t ipAddr = APSs[string(deviceSerial)]->get_ip_addr();
    string ipAddrStr = asio::ip::address_v4(ipAddr).to_string();
    ipAddrStr.copy(ipAddrPtr, ipAddrStr.size(), 0);
    return APS2_OK;
  } catch (...) {
    return APS2_UNKNOWN_ERROR;
  }
}

APS2_STATUS set_ip_addr(const char *deviceSerial, const char *ipAddrStr) {
  uint32_t ipAddr = asio::ip::address_v4::from_string(ipAddrStr).to_ulong();
  return aps2_call(deviceSerial, &APS2::set_ip_addr, ipAddr);
}

APS2_STATUS write_SPI_setup(const char *deviceSerial) {
  return aps2_call(deviceSerial, &APS2::write_SPI_setup);
}

APS2_STATUS get_dhcp_enable(const char *deviceSerial, int *enabled) {
  return aps2_getter(deviceSerial, &APS2::get_dhcp_enable, enabled);
}

APS2_STATUS set_dhcp_enable(const char *deviceSerial, const int enable) {
  return aps2_call(deviceSerial, &APS2::set_dhcp_enable, enable);
}

int run_DAC_BIST(const char *deviceSerial, const int dac, int16_t *data,
                 unsigned int length, uint32_t *results) {
  vector<int16_t> testVec(data, data + length);
  vector<uint32_t> tmpResults;
  int passed =
      APSs[string(deviceSerial)]->run_DAC_BIST(dac, testVec, tmpResults);
  std::copy(tmpResults.begin(), tmpResults.end(), results);
  return passed;
}

APS2_STATUS set_DAC_SD(const char *deviceSerial, const int dac,
                       const uint8_t sd) {
  return aps2_call(deviceSerial, &APS2::set_DAC_SD, dac, sd);
}

APS2_STATUS toggle_DAC_clock(const char *deviceSerial, const int dac) {
  return aps2_call(deviceSerial, &APS2::toggle_DAC_clock, dac);
}

#ifdef __cplusplus
}
#endif
