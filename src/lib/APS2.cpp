#include <bitset>
#include <fstream>
#include <stdexcept> //std::runtime_error
#include <utility>   //std::swap
using std::endl;

#include "APS2.h"
#include "APS2Datagram.h"

APS2::APS2()
    : legacy_firmware{false}, ipAddr_{""}, connected_{false}, channels_(2),
      samplingRate_{0} {};

APS2::APS2(string deviceSerial)
    : legacy_firmware{false}, ipAddr_{deviceSerial}, connected_{false},
      samplingRate_{0} {
  channels_.reserve(2);
  for (size_t ct = 0; ct < 2; ct++)
    channels_.push_back(Channel(ct));
};

APS2::~APS2() = default;

void APS2::connect(shared_ptr<APS2Ethernet> &&ethernetRM) {
  FILE_LOG(logDEBUG) << ipAddr_ << " APS2::connect";
  // Hold on to APS2Ethernet class to keep socket alive
  ethernetRM_ = ethernetRM;
  if (!connected_) {
    try {
      ethernetRM_->connect(ipAddr_);
      connected_ = true;
    } catch (...) {
      // release reference to APS2Ethernet
      ethernetRM_.reset();
      throw APS2_FAILED_TO_CONNECT;
    }

    // Figure out whether this is an APS2 or TDM and what register map to use
    APSStatusBank_t statusRegs = read_status_registers();
    if (statusRegs.userFirmwareVersion != 0xbadda555) {
      legacy_firmware = true;
    }
    if ((statusRegs.hostFirmwareVersion & (1 << APS2_HOST_TYPE_BIT)) ==
        (1 << APS2_HOST_TYPE_BIT)) {
      host_type = TDM;
    } else {
      host_type = APS;
    }

    FILE_LOG(logINFO) << ipAddr_ << " opened connection to device";

    // TODO: restore state information from file
  }
}

void APS2::disconnect() {
  FILE_LOG(logDEBUG) << ipAddr_ << " APS2::disconnect";
  if (connected_) {
    ethernetRM_->disconnect(ipAddr_);
    connected_ = false;
    FILE_LOG(logINFO) << ipAddr_ << " closed connection to device";

    // Release reference to ethernet RM
    ethernetRM_.reset();
    // TODO: save state information to file
  }
}

void APS2::reset(APS2_RESET_MODE mode) {
  FILE_LOG(logDEBUG1) << ipAddr_ << " APS2::reset";

  APS2Command cmd;
  cmd.sel = 1;
  cmd.cmd = static_cast<uint32_t>(APS_COMMANDS::RESET);

  // For EPROM reset we can specify the image to load
  // Use user image for now
  uint32_t addr = 0;
  switch (mode) {
  case RECONFIG_EPROM_USER:
    addr = EPROM_USER_IMAGE_ADDR;
    break;
  case RECONFIG_EPROM_BASE:
    addr = EPROM_BASE_IMAGE_ADDR;
    break;
  default:
    throw APS2_UNKNOWN_ERROR;
  }

  // Send the command
  ethernetRM_->send(ipAddr_, {{cmd, addr, {}}});

  // we expect to loose the connection at this point...
}

APS2_STATUS APS2::init(const bool &forceReload, const int &bitFileNum) {
  if (host_type == TDM) {
    return APS2_OK;
  }

  get_sampleRate(); // to update state variable

  check_clocks_status();

  // check if previously initialized
  uint32_t initReg = read_memory(INIT_STATUS_ADDR, 1)[0];
  bool initialized = (initReg & 0x1) == 0x1;

  if (!initialized || forceReload) {
    FILE_LOG(logINFO) << ipAddr_ << " initializing APS2";

    // toggle the OSERDES reset for firmware < v4.3
    set_register_bit(RESETS_ADDR, {IO_CHA_RST_BIT, IO_CHB_RST_BIT});
    clear_register_bit(RESETS_ADDR, {IO_CHA_RST_BIT, IO_CHB_RST_BIT});

    // align DAC data clock boundaries
    setup_DACs();

    clear_channel_data();

    write_memory_map();

    // write to INIT_STATUS_ADDR to record that init() was run
    initReg |= 0x1;
    write_memory(INIT_STATUS_ADDR, initReg);
  }

  return APS2_OK;
}

void APS2::setup_DACs() {
  align_DAC_clock(0);
  align_DAC_clock(1);

  align_DAC_LVDS_capture(0);
  align_DAC_LVDS_capture(1);
}

APSStatusBank_t APS2::read_status_registers() {
  FILE_LOG(logDEBUG1) << ipAddr_ << " APS2::read_status_registers";
  // Query with the status request command
  APS2Command cmd;
  cmd.cmd = static_cast<uint32_t>(APS_COMMANDS::STATUS);
  cmd.r_w = 1;
  cmd.sel = 1; // necessary for newer firmware to demux to ApsMsgProc
  cmd.mode_stat = APS_STATUS_HOST;

  // Send the request
  ethernetRM_->send(ipAddr_, {{cmd, 0, {}}});

  // Read the response
  auto resp_dg = ethernetRM_->read(ipAddr_, COMMS_TIMEOUT);

  // Copy the data back into the status type
  APSStatusBank_t statusRegs;
  std::copy(resp_dg.payload.begin(), resp_dg.payload.end(), statusRegs.array);
  FILE_LOG(logDEBUG1) << ipAddr_ << print_status_bank(statusRegs);
  return statusRegs;
}

uint32_t APS2::get_firmware_version() {
  // Return the firmware version register value
  uint32_t version;
  if (legacy_firmware) {
    // Reads version information from status registers
    APSStatusBank_t statusRegs = read_status_registers();
    version = statusRegs.userFirmwareVersion;
  } else {
    // Reads version information from CSR register
    version = read_memory(FIRMWARE_VERSION_ADDR, 1)[0];
  }

  FILE_LOG(logDEBUG) << ipAddr_ << " FPGA firmware version "
                     << print_firmware_version(version);

  return version;
}

uint32_t APS2::get_firmware_git_sha1() {
  // Return the firmware version register value
  return read_memory(FIRMWARE_GIT_SHA1_ADDR, 1)[0];
}

uint32_t APS2::get_firmware_build_timestamp() {
  // Return the firmware version register value
  return read_memory(FIRMWARE_BUILD_TIMESTAMP_ADDR, 1)[0];
}

double APS2::get_uptime() {
  /*
  * Return the board uptime in seconds.
  */
  FILE_LOG(logDEBUG1) << ipAddr_ << " APS2::get_uptime";
  uint32_t uptime_seconds, uptime_nanoseconds;
  if (legacy_firmware) {
    // Read the status registers
    APSStatusBank_t statusRegs = read_status_registers();
    // Put together the seconds and nanoseconds parts
    // In the APS2MsgProc the nanoseconds doesn't reset at 1s so take fractional
    // part
    uptime_seconds = statusRegs.uptimeSeconds;
    uptime_nanoseconds = statusRegs.uptimeNanoSeconds;
  } else {
    // Reads uptime from adjacent CSR registers
    auto uptime_vec = read_memory(UPTIME_SECONDS_ADDR, 2);
    uptime_seconds = uptime_vec[0];
    uptime_nanoseconds = uptime_vec[1];
  }
  auto uptime = std::chrono::duration_cast<std::chrono::duration<double>>(
      std::chrono::seconds(uptime_seconds) +
      std::chrono::nanoseconds(uptime_nanoseconds));
  return uptime.count();
}

float APS2::get_fpga_temperature() {
  /*
  * Return the FGPA die temperature in C.
  */
  uint32_t temperature_reg;
  if (legacy_firmware) {
    // Read the status registers
    APSStatusBank_t statusRegs = read_status_registers();
    temperature_reg = statusRegs.userStatus;
  } else {
    // Read CSR register
    temperature_reg = read_memory(TEMPERATURE_ADDR, 1).front();
  }

  // Temperature is return in bottom 12bits of user status and needs to be
  // converted from the 12bit ADC value
  float temp =
      static_cast<float>((temperature_reg & 0xfff)) * 503.975 / 4096 - 273.15;

  // Don't return a stupid number of digits
  // It seems the scale goes from 0-504K with 12bits = 0.12 degrees precision at
  // best
  temp = round(10 * temp) / 10;

  return temp;
}

void APS2::write_bitfile(const string &bitFile, uint32_t start_addr,
                         APS2_BITFILE_STORAGE_MEDIA media) {
  // Write a bitfile to either configuration DRAM or ERPOM starting at specified
  // address
  FILE_LOG(logDEBUG1) << ipAddr_ << " APS2::write_bitfile";

  // byte alignment of storage media
  uint32_t alignment;
  switch (media) {
  case BITFILE_MEDIA_DRAM:
    alignment = 8;
    break;
  case BITFILE_MEDIA_EPROM:
    alignment = 256;
    break;
  }

  // Get the file size in bytes
  std::ifstream FID(bitFile, std::ios::in | std::ios::binary);
  if (!FID.is_open()) {
    FILE_LOG(logERROR) << ipAddr_ << " unable to open bitfile: " << bitFile;
    throw APS2_NO_SUCH_BITFILE;
  }

  FID.seekg(0, std::ios::end);
  size_t file_size = FID.tellg();
  FILE_LOG(logDEBUG) << ipAddr_ << " opened bitfile: " << bitFile << " with "
                     << file_size << " bytes";
  FID.seekg(0, std::ios::beg);

  // Figure out padding size for alignment
  size_t padding_bytes = (alignment - (file_size % alignment)) % alignment;
  FILE_LOG(logDEBUG1) << ipAddr_ << " padding bitfile byte vector with "
                      << padding_bytes << " bytes.";
  // Copy the file data to a 32bit word vector
  vector<uint32_t> bitfile_words((file_size + padding_bytes) / 4, 0xffffffff);
  FID.read(reinterpret_cast<char *>(bitfile_words.data()), file_size);

  // Swap bytes because we want to preserve bytes order in file it will be
  // byte-swapped again when the packet is serialized
  for (auto &val : bitfile_words) {
    val = htonl(val);
  }

  switch (media) {
  case BITFILE_MEDIA_DRAM:
    write_configuration_SDRAM(start_addr, bitfile_words);
    break;
  case BITFILE_MEDIA_EPROM:
    write_flash(start_addr, bitfile_words);
    break;
  }

  // Now validate bitfile data
  bitfile_writing_task = VALIDATING;
  bitfile_writing_task_progress = 0;
  uint32_t end_addr = start_addr + 4 * bitfile_words.size();
  uint32_t addr = start_addr;
  do {
    uint16_t read_length;
    // Read up to 1kB at a time because of ApsMsgProc limitations
    if ((end_addr - addr) >= (1 << 10)) {
      read_length = 256;
    } else {
      read_length = (end_addr - addr) / 4;
    }

    vector<uint32_t> check_vec;
    switch (media) {
    case BITFILE_MEDIA_DRAM:
      check_vec = read_configuration_SDRAM(addr, read_length);
      break;
    case BITFILE_MEDIA_EPROM:
      check_vec = read_flash(addr, read_length);
      break;
    }

    for (size_t ct = 0; ct < read_length; ct++) {
      if (check_vec[ct] != bitfile_words[(addr - start_addr) / 4]) {
        throw APS2_BITFILE_VALIDATION_FAILURE;
      }
      addr += 4;
    }
    bitfile_writing_task_progress =
        static_cast<double>(addr - start_addr) / (end_addr - start_addr);
  } while (addr < end_addr);
}

void APS2::program_bitfile(uint32_t addr) {
  // Program the bitfile from configuration SDRAM at the specified address
  // FPGA will reset so connection will be dropped
  FILE_LOG(logDEBUG1) << ipAddr_ << " APS2::program_bitfile";
  FILE_LOG(logINFO) << ipAddr_ << " programming bitfile starting at address "
                    << hexn<8> << addr;

  APS2Command cmd;
  cmd.sel = 1; // necessary for newer firmware to demux to ApsMsgProc
  cmd.cmd = static_cast<uint32_t>(APS_COMMANDS::FPGACONFIG_CTRL);
  ethernetRM_->send(ipAddr_, {{cmd, addr, {}}});
}

void APS2::set_sampleRate(const unsigned int &freq) {
  if (samplingRate_ != freq) {
    // Set PLL frequency
    APS2::set_PLL_freq(freq);

    samplingRate_ = freq;

    // Test the sync
    // APS2::test_PLL_sync();
  }
}

unsigned int APS2::get_sampleRate() {
  FILE_LOG(logDEBUG1) << ipAddr_ << " APS2::get_sampleRate";
  samplingRate_ = get_PLL_freq();
  return samplingRate_;
}

void APS2::clear_channel_data() {
  FILE_LOG(logINFO) << ipAddr_ << " clearing all channel data for APS2 "
                    << ipAddr_;
  for (auto &ch : channels_) {
    ch.clear_data();
  }
}

void APS2::load_sequence_file(const string &seqFile) {
  /*
   * Load a sequence file from a binary file
   */
  try {
    FILE_LOG(logINFO) << ipAddr_ << " opening sequence file: " << seqFile;

    // Read the header information
    char junk[100];
    uint64_t buff_length;
    uint16_t num_chans;

    std::fstream file(seqFile, std::ios::binary | std::ios::in);
    if( !file ) throw APS2_SEQFILE_FAIL; 

    file.read(junk, 12); // Don't need this info
    file.read(reinterpret_cast<char *> (&num_chans), sizeof(uint16_t));

    // Start anew
    clear_channel_data();

    // Read the instructions
    file.read(reinterpret_cast<char *> (&buff_length), sizeof(uint64_t));
    vector<uint64_t> instructions;
    instructions.resize(buff_length);
    file.read(reinterpret_cast<char *> (instructions.data()), buff_length*sizeof(uint64_t));
    write_sequence(instructions);

    // Read the waveforms
    vector<int16_t> waveform;
    for (int chanct = 0; chanct < num_chans; chanct++) {
      file.read(reinterpret_cast<char *> (&buff_length), sizeof(uint64_t));
      waveform.resize(buff_length);
      file.read(reinterpret_cast<char *> (waveform.data()), buff_length*sizeof(int16_t));
      set_waveform(chanct, waveform);
    }
  } catch (...) {
    throw APS2_SEQFILE_FAIL;
  }
}

void APS2::check_channel_num(int chan) const {
  if (chan < 0 || chan >= NUM_ANALOG_CHANNELS) {
    FILE_LOG(logERROR) << ipAddr_ << " invalid DAC " << chan;
    throw APS2_INVALID_DAC;
  }
}

void APS2::set_channel_enabled(int dac, bool enable) {
  check_channel_num(dac);
  channels_[dac].set_enabled(enable);
}

bool APS2::get_channel_enabled(int dac) const {
  check_channel_num(dac);
  return channels_[dac].get_enabled();
}

void APS2::set_channel_bitslip(int dac, unsigned slip) {
  check_channel_num(dac);
  write_memory(dac == 0 ? BITSLIP_A_ADDR : BITSLIP_B_ADDR, slip & 0x3);
}

unsigned APS2::get_channel_bitslip(int dac) {
  check_channel_num(dac);
  return read_memory(dac == 0 ? BITSLIP_A_ADDR : BITSLIP_B_ADDR, 1).front() & 0x3;
}

void APS2::set_channel_offset(int dac, float offset) {
  check_channel_num(dac);
  // Scale offset to Q0.13 fixed point
  int16_t offset_fixed = offset * MAX_WF_AMP;

  // read the current value
  uint32_t val = read_memory(CHANNEL_OFFSET_ADDR, 1)[0];

  // Overwrite the upper/lower word
  if (dac == 0) {
    // Top half
    val = (static_cast<uint32_t>(offset_fixed) << 16) | (val & 0xffff);
  } else {
    // Bottom half
    val = (val & 0xffff0000) | (offset_fixed & 0xffff);
  }
  write_memory(CHANNEL_OFFSET_ADDR, val);
}

float APS2::get_channel_offset(int dac) const {
  check_channel_num(dac);
  // get register val
  uint32_t val = read_memory(CHANNEL_OFFSET_ADDR, 1)[0];

  // extract upper/lower half
  int16_t offset_fixed =
      static_cast<int16_t>((dac == 0 ? val >> 16 : val) & 0xffff);

  // covert back to float
  return static_cast<float>(offset_fixed) / MAX_WF_AMP;
}

void APS2::set_channel_scale(int dac, float scale) {
  check_channel_num(dac);
  // write register for future getting
  uint32_t reg_addr = dac == 0 ? CH_A_SCALE_ADDR : CH_B_SCALE_ADDR;
  write_memory(reg_addr, reinterpret_cast<uint32_t &>(scale));
  // update correction matrix
  update_correction_matrix();
}

float APS2::get_channel_scale(int dac) const {
  check_channel_num(dac);
  // get register value and convert back to float
  uint32_t reg_addr = dac == 0 ? CH_A_SCALE_ADDR : CH_B_SCALE_ADDR;
  return reinterpret_cast<float &>(read_memory(reg_addr, 1)[0]);
}

void APS2::set_mixer_amplitude_imbalance(float amp) {
  // write register for future getting
  write_memory(MIXER_AMP_IMBALANCE_ADDR, reinterpret_cast<uint32_t &>(amp));
  // update correction matrix
  update_correction_matrix();
}

float APS2::get_mixer_amplitude_imbalance() {
  // get register value and convert back to float
  return reinterpret_cast<float &>(read_memory(MIXER_AMP_IMBALANCE_ADDR, 1)[0]);
}

void APS2::set_mixer_phase_skew(float skew) {
  // write register for future getting
  write_memory(MIXER_PHASE_SKEW_ADDR, reinterpret_cast<uint32_t &>(skew));
  // update correction matrix
  update_correction_matrix();
}

float APS2::get_mixer_phase_skew() {
  // get register value and convert back to float
  return reinterpret_cast<float &>(read_memory(MIXER_PHASE_SKEW_ADDR, 1)[0]);
}

void APS2::set_mixer_correction_matrix(const vector<float> &mat) {
  FILE_LOG(logDEBUG1) << ipAddr_ << " APS2::set_mixer_correction_matrix";
  // convert to Q2.13 fixed point
  vector<int32_t> correction_matrix;
  for (auto &val : mat) {
    correction_matrix.push_back(
        static_cast<int32_t>(CORRECTION_MATRIX_SCALING * val));
  }
  // Slot into registers and write to memory
  uint32_t row0, row1;
  row0 = (correction_matrix[0] << 16) | (correction_matrix[1] & 0xffff);
  write_memory(CORRECTION_MATRIX_ROW0_ADDR, row0);
  row1 = (correction_matrix[2] << 16) | (correction_matrix[3] & 0xffff);
  write_memory(CORRECTION_MATRIX_ROW1_ADDR, row1);
}

vector<float> APS2::get_mixer_correction_matrix() {
  FILE_LOG(logDEBUG1) << ipAddr_ << " APS2::get_mixer_correction_matrix";
  // read packed registers
  uint32_t row0, row1;
  row0 = read_memory(CORRECTION_MATRIX_ROW0_ADDR, 1)[0];
  row1 = read_memory(CORRECTION_MATRIX_ROW1_ADDR, 1)[0];
  // convert back to float
  vector<float> correction_matrix;
  correction_matrix.push_back(
      static_cast<float>(static_cast<int16_t>(row0 >> 16)) /
      CORRECTION_MATRIX_SCALING);
  correction_matrix.push_back(
      static_cast<float>(static_cast<int16_t>(row0 & 0xffff)) /
      CORRECTION_MATRIX_SCALING);
  correction_matrix.push_back(
      static_cast<float>(static_cast<int16_t>(row1 >> 16)) /
      CORRECTION_MATRIX_SCALING);
  correction_matrix.push_back(
      static_cast<float>(static_cast<int16_t>(row1 & 0xffff)) /
      CORRECTION_MATRIX_SCALING);
  return correction_matrix;
}

void APS2::update_correction_matrix() {
  // Update the 2x2 correction matrix assuming an mixer amplitude imbalance,
  // phase skew and independent channel scales
  // [i_scale, 0;0, q_scale] * [amp, amp*tan(phi); 0, 1/cos(phi)] =
  // [i_scale*amp, i_scale*amp*tan(phi); 0, q_scale*1/cos(phi)]}
  float amp_imbalance =
      reinterpret_cast<float &>(read_memory(MIXER_AMP_IMBALANCE_ADDR, 1)[0]);
  float phase_skew =
      reinterpret_cast<float &>(read_memory(MIXER_PHASE_SKEW_ADDR, 1)[0]);
  float i_scale = reinterpret_cast<float &>(read_memory(CH_A_SCALE_ADDR, 1)[0]);
  float q_scale = reinterpret_cast<float &>(read_memory(CH_B_SCALE_ADDR, 1)[0]);

  // calculate matrix terms and convert to Q2.13 fixed point
  int32_t correction_matrix_00 =
      static_cast<int32_t>(i_scale * amp_imbalance * CORRECTION_MATRIX_SCALING);
  int32_t correction_matrix_01 = static_cast<int32_t>(
      i_scale * amp_imbalance * tan(phase_skew) * CORRECTION_MATRIX_SCALING);
  int32_t correction_matrix_10 = 0;
  int32_t correction_matrix_11 = static_cast<int32_t>(
      q_scale / cos(phase_skew) * CORRECTION_MATRIX_SCALING);

  // Slot into registers and write to memory
  uint32_t row0, row1;
  row0 = (correction_matrix_00 << 16) | (correction_matrix_01 & 0xffff);
  write_memory(CORRECTION_MATRIX_ROW0_ADDR, row0);
  row1 = (correction_matrix_10 << 16) | (correction_matrix_11 & 0xffff);
  write_memory(CORRECTION_MATRIX_ROW1_ADDR, row1);
}

void APS2::set_markers(const int &dac, const vector<uint8_t> &data) {
  channels_[dac].set_markers(data);
  // write the waveform data again to add packed marker data
  write_waveform(dac, channels_[dac].prep_waveform());
}

void APS2::set_trigger_source(const APS2_TRIGGER_SOURCE &triggerSource) {
  FILE_LOG(logDEBUG) << ipAddr_ << " setting trigger source to "
                     << triggerSource;

  uint32_t regVal = read_memory(CONTROL_REG_ADDR, 1)[0];

  // Set the trigger source bits
  regVal = (regVal & ~(3 << TRIGSRC_BIT)) |
           (static_cast<uint32_t>(triggerSource) << TRIGSRC_BIT);
  write_memory(CONTROL_REG_ADDR, regVal);
}

APS2_TRIGGER_SOURCE APS2::get_trigger_source() {
  uint32_t regVal = read_memory(CONTROL_REG_ADDR, 1)[0];
  return APS2_TRIGGER_SOURCE((regVal & (3 << TRIGSRC_BIT)) >> TRIGSRC_BIT);
}

void APS2::set_trigger_interval(const double &interval) {
  FILE_LOG(logDEBUG) << ipAddr_ << " APS2::set_trigger_interval";
  uint32_t clocks;
  switch (host_type) {
  case APS:
    // SM clock is 1/4 of samplingRate so the trigger interval in SM clock
    // periods is
    clocks = round(interval * 0.25 * get_sampleRate() * 1e6);
    break;
  case TDM:
    // TDM operates on a fixed 100 MHz clock
    clocks = round(interval * 100e6);
    break;
  }
  FILE_LOG(logDEBUG1) << ipAddr_ << " setting trigger interval to "
                      << interval << "s (" << clocks << " cycles)";

  write_memory(TRIGGER_INTERVAL_ADDR, clocks);

}

double APS2::get_trigger_interval() {
  FILE_LOG(logDEBUG) << ipAddr_ << " APS2::get_trigger_interval";
  uint32_t clocks;
  clocks = read_memory(TRIGGER_INTERVAL_ADDR, 1)[0];
  FILE_LOG(logDEBUG1) << ipAddr_ << " trigger intervals clocks = " << clocks;
  // convert from clock cycles to time
  switch (host_type) {
  case APS:
    // SM clock is 1/4 of samplingRate so the trigger interval in SM clock
    // periods is
    return static_cast<double>(clocks) / (0.25 * get_sampleRate() * 1e6);
  case TDM:
    // TDM operates on a fixed 100 MHz clock
    return static_cast<double>(clocks) / (100e6);
  }
  // Shoud never get here;
  throw APS2_UNKNOWN_ERROR;
}

void APS2::trigger() {
  // Apply a software trigger by toggling the trigger line
  FILE_LOG(logDEBUG) << ipAddr_ << " APS2::trigger";
  uint32_t regVal = read_memory(CONTROL_REG_ADDR, 1)[0];
  FILE_LOG(logDEBUG3) << ipAddr_ << " SEQ_CONTROL register was "
                      << hexn<8> << regVal;
  regVal ^= (1 << SOFT_TRIG_BIT);
  FILE_LOG(logDEBUG3) << ipAddr_ << " setting SEQ_CONTROL register to "
                      << hexn<8> << regVal;
  write_memory(CONTROL_REG_ADDR, regVal);
}

void APS2::run() {
  FILE_LOG(logDEBUG1) << ipAddr_ << " APS2::run";
  if (host_type == APS) {
    FILE_LOG(logDEBUG1) << ipAddr_ << " releasing the cache controller...";
    set_register_bit(CACHE_CONTROL_ADDR, {CACHE_ENABLE_BIT});
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  FILE_LOG(logDEBUG1) << ipAddr_
                      << " releasing pulse sequencer state machine...";
  set_register_bit(CONTROL_REG_ADDR, {SM_ENABLE_BIT});
  std::this_thread::sleep_for(std::chrono::milliseconds(1));

  FILE_LOG(logDEBUG1) << ipAddr_ << " enabling trigger...";
  set_register_bit(CONTROL_REG_ADDR, {TRIGGER_ENABLE_BIT});
}

void APS2::stop() {
  FILE_LOG(logDEBUG1) << ipAddr_ << " APS2::stop";
  switch (host_type) {
  case APS:
    // hold the cache in reset
    clear_register_bit(CACHE_CONTROL_ADDR, {CACHE_ENABLE_BIT});
    break;
  case TDM:
    break;
  }
  // Put the sequencer and trigger back in reset
  clear_register_bit(CONTROL_REG_ADDR, {SM_ENABLE_BIT, TRIGGER_ENABLE_BIT});
}

APS2_RUN_STATE APS2::get_runState() { return runState; }

void APS2::set_run_mode(const APS2_RUN_MODE &mode) {
  FILE_LOG(logDEBUG) << ipAddr_ << " setting run mode to " << mode;

  // If necessary pull the correct instruction sequence scaffold
  vector<uint64_t> instructions;
  switch (mode) {
  case RUN_SEQUENCE:
    // don't need to do anything... already there
    return;
  case TRIG_WAVEFORM:
    instructions = WF_SEQ_TRIG;
    break;
  case CW_WAVEFORM:
    instructions = WF_SEQ_CW;
    break;
  default:
    // unknown mode
    throw APS2_UNKNOWN_RUN_MODE;
  }
  // set SSB frequency
  instructions[0] |= read_memory(WF_SSB_FREQ_ADDR, 1)[0];

  // inject waveform lengths into the instructions
  size_t wf_length_a, wf_length_b;
  wf_length_a = read_memory(CH_A_WF_LENGTH_ADDR, 1)[0];
  wf_length_b = read_memory(CH_B_WF_LENGTH_ADDR, 1)[0];
  if ((wf_length_a == 0) && (wf_length_b == 0)) {
    throw APS2_NO_WFS;
  }
  // get insertion point before end
  auto goto_entry = std::prev(instructions.end());
  // insert marker instructions on MK 1/MK 2 for TRIG_WAVEFORM
  if (mode == TRIG_WAVEFORM) {
    if (wf_length_a > 0) {
      instructions.insert(goto_entry,
                          0x1000000100000000 | (wf_length_a / 4 - 1));
      goto_entry = std::prev(instructions.end());
    }
    if (wf_length_b > 0) {
      instructions.insert(goto_entry,
                          0x1400000100000000 | (wf_length_b / 4 - 1));
      goto_entry = std::prev(instructions.end());
    }
  }
  if (wf_length_a == wf_length_b) {
    // single broadcast wf instruction with write flag high
    instructions.insert(goto_entry,
                        0x0d00000000000000L | ((wf_length_a / 4 - 1) << 24));
  } else if (wf_length_b == 0) {
    // single wf instruction to a with write flag high
    instructions.insert(goto_entry,
                        0x0500000000000000L | ((wf_length_a / 4 - 1) << 24));
  } else if (wf_length_a == 0) {
    // single wf instruction to b with write flag high
    instructions.insert(goto_entry,
                        0x0900000000000000L | ((wf_length_b / 4 - 1) << 24));
  } else {
    // wf instruction to with write flag low; wf insruction to b with write flag
    // high
    instructions.insert(goto_entry,
                        0x0400000000000000L | ((wf_length_a / 4 - 1) << 24));
    goto_entry = std::prev(instructions.end());
    instructions.insert(goto_entry,
                        0x0900000000000000L | ((wf_length_b / 4 - 1) << 24));
  }

  FILE_LOG(logDEBUG2) << ipAddr_ << " writing waveform mode sequence:";
  for (auto instr : instructions) {
    FILE_LOG(logDEBUG2) << "\t" << hexn<16> << instr;
  }
  write_sequence(instructions);
}

void APS2::set_waveform_frequency(float freq) {
  // frequency gets converted to portion of circle per 300MHz clock cycle in
  // range [-2, 2)
  if ((freq >= 600e6) || (freq < -600e6)) {
    throw APS2_WAVEFORM_FREQ_OVERFLOW;
  }
  uint32_t freq_increment =
      freq > 0 ? (freq / 300e6) * (1 << 28) : (freq / 300e6 + 4) * (1 << 28);
  FILE_LOG(logDEBUG2) << ipAddr_ << " writing waveform frequency increment: "
                      << freq_increment;
  write_memory(WF_SSB_FREQ_ADDR, freq_increment);
}

float APS2::get_waveform_frequency() {
  // frequency gets converted to portion of circle per 300MHz clock cycle in
  // range [-2, 2)
  uint32_t freq_increment = read_memory(WF_SSB_FREQ_ADDR, 1)[0];
  float freq = static_cast<float>(freq_increment) / (1 << 28) * 300e6;
  if (freq > 600e6) {
    freq += -1200e6;
  }
  return freq;
}

// FPGA memory read/write
void APS2::write_memory(const uint32_t &addr, const uint32_t &data) {
  // Create the vector and pass through
  write_memory(addr, vector<uint32_t>({data}));
}

void APS2::write_memory(const uint32_t &addr, const vector<uint32_t> &data) {
  /* APS2::write_memory
  * addr = start byte of address space
  * data = vector<uint32_t> data
  */

  // Memory writes to SDRAM need to be 16 byte aligned and padded to a multiple
  // of 16 bytes (4 words)
  if (addr < MEMORY_ADDR + 0x40000000) {
    if ((addr & 0xf) != 0) {
      FILE_LOG(logERROR) << ipAddr_ << " attempted to write "
                                       "waveform/instruction SDRAM at an "
                                       "address not aligned to 16 bytes";
      throw APS2_UNALIGNED_MEMORY_ACCESS;
    }
    if ((data.size() % 4) != 0) {
      FILE_LOG(logERROR) << ipAddr_ << " attempted to write "
                                       "waveform/instruction SDRAM with data "
                                       "not a multiple of 16 bytes";
      throw APS2_UNALIGNED_MEMORY_ACCESS;
    }
  }

  // Convert to datagrams and send
  APS2Command cmd;
  cmd.ack = 1;
  cmd.cmd = static_cast<uint32_t>(
      APS_COMMANDS::USERIO_ACK); // TODO: take out when all TCP comms
  auto dgs = APS2Datagram::chunk(
      cmd, addr, data,
      0xfffc); // max chunk_size is limited by 128bit data alignment in SDRAM
  ethernetRM_->send(ipAddr_, dgs);
}

vector<uint32_t> APS2::read_memory(uint32_t addr, uint32_t numWords) const {
  // TODO: handle numWords that require mulitple requests

  // Send the read request
  APS2Command cmd;
  cmd.r_w = 1;
  cmd.cmd = static_cast<uint32_t>(
      APS_COMMANDS::USERIO_ACK); // TODO: take out when all TCP comms
  cmd.cnt = numWords;
  ethernetRM_->send(ipAddr_, {{cmd, addr, {}}});

  // Read the response
  // We expect a single datagram back
  auto result = ethernetRM_->read(ipAddr_, COMMS_TIMEOUT);
  // TODO: error checking
  return result.payload;
}

void APS2::write_configuration_SDRAM(uint32_t addr,
                                     const vector<uint32_t> &data) {
  FILE_LOG(logDEBUG2) << ipAddr_ << " APS2::write_configuration_SDRAM";
  // Write data to configuratoin SDRAM

  // SDRAM writes must be 8 byte aligned
  if ((addr & 0x7) != 0) {
    FILE_LOG(logERROR) << ipAddr_ << " attempted to write configuration SDRAM "
                                     "at an address not aligned to 8 bytes";
    throw APS2_UNALIGNED_MEMORY_ACCESS;
  }
  if ((data.size() % 2) != 0) {
    FILE_LOG(logERROR) << ipAddr_ << " attempted to write configuration SDRAM "
                                     "with data not a multiple of 8 bytes";
    throw APS2_UNALIGNED_MEMORY_ACCESS;
  }

  // Convert to datagrams.	For the now the ApsMsgProc needs maximum of 1kB
  // chunks
  APS2Command cmd;
  cmd.ack = 1;
  cmd.sel = 1; // necessary for newer firmware to demux to ApsMsgProc
  cmd.cmd = static_cast<uint32_t>(APS_COMMANDS::FPGACONFIG_ACK);
  auto dgs = APS2Datagram::chunk(
      cmd, addr, data,
      0x100); // 1kB chunks to fit in fake ethernet packet to ApsMsgProc
  bitfile_writing_task = WRITING;
  bitfile_writing_task_progress = 0;
  // Write 1 at a time so we can update progress
  for (size_t ct = 0; ct < dgs.size(); ct++) {
    ethernetRM_->send(ipAddr_, {dgs[ct]});
    bitfile_writing_task_progress =
        static_cast<double>(ct + 1) / static_cast<double>(dgs.size());
  }
}

vector<uint32_t> APS2::read_configuration_SDRAM(uint32_t addr,
                                                uint32_t num_words) {
  FILE_LOG(logDEBUG2) << ipAddr_ << " APS2::read_configuration_SDRAM";
  // Send the read request
  APS2Command cmd;
  cmd.r_w = 1;
  cmd.sel = 1;
  cmd.cmd = static_cast<uint32_t>(APS_COMMANDS::FPGACONFIG_ACK);
  cmd.cnt = num_words;
  ethernetRM_->send(ipAddr_, {{cmd, addr, {}}});

  // Read the response
  // We expect a single datagram back
  auto result = ethernetRM_->read(ipAddr_, COMMS_TIMEOUT);
  // TODO: error checking
  return result.payload;
}

// SPI read/write
void APS2::write_SPI(vector<uint32_t> &msg) {
  FILE_LOG(logDEBUG2) << ipAddr_ << " APS2::write_SPI";

  // push on "end of message"
  APSChipConfigCommand_t cmd;
  cmd.target = CHIPCONFIG_IO_TARGET_EOL;
  msg.push_back(cmd.packed);

  // build datagram
  APS2Command cmd_bis;
  cmd_bis.ack = 1;
  cmd_bis.sel = 1; // necessary for newer firmware to demux to ApsMsgProc
  cmd_bis.cmd = static_cast<uint32_t>(APS_COMMANDS::CHIPCONFIGIO);
  auto dgs = APS2Datagram::chunk(
      cmd_bis, 0, msg,
      0x100); // 1kB chunks to fit in fake ethernet packet to ApsMsgProc
  ethernetRM_->send(ipAddr_, dgs);
}

uint32_t APS2::read_SPI(const CHIPCONFIG_IO_TARGET &target,
                        const uint16_t &addr) {
  // reads a single byte from the target SPI device
  FILE_LOG(logDEBUG2) << ipAddr_ << " APS2::read_SPI";

  // build message
  APSChipConfigCommand_t cmd;
  DACCommand_t dacinstr;
  PLLCommand_t pllinstr;

  // config target and instruction
  switch (target) {
  case CHIPCONFIG_TARGET_DAC_0:
    cmd.target = CHIPCONFIG_IO_TARGET_DAC_0;
    dacinstr.addr = addr;
    dacinstr.N = 0;   // single-byte read
    dacinstr.r_w = 1; // read
    cmd.instr = dacinstr.packed;
    break;
  case CHIPCONFIG_TARGET_DAC_1:
    cmd.target = CHIPCONFIG_IO_TARGET_DAC_1;
    dacinstr.addr = addr;
    dacinstr.N = 0;   // single-byte read
    dacinstr.r_w = 1; // read
    cmd.instr = dacinstr.packed;
    break;
  case CHIPCONFIG_TARGET_PLL:
    cmd.target = CHIPCONFIG_IO_TARGET_PLL;
    pllinstr.addr = addr;
    pllinstr.W = 0;   // single-byte read
    pllinstr.r_w = 1; // read
    cmd.instr = pllinstr.packed;
    break;
  default:
    FILE_LOG(logERROR) << ipAddr_ << " invalid read_SPI target "
                       << hexn<1> << target;
    return 0;
  }
  cmd.spicnt_data = 1; // request 1 byte

  // interface logic requires at least 3 bytes of data to return anything, so
  // push on the same instruction three more times more
  vector<uint32_t> msg = {cmd.packed, cmd.packed, cmd.packed, cmd.packed};

  // write the SPI read instruction
  write_SPI(msg);

  // build read datagram
  APS2Command cmd_bis;
  cmd_bis.r_w = 1;
  cmd_bis.sel = 1; // necessary for newer firmware to demux to ApsMsgProc
  cmd_bis.cmd = static_cast<uint32_t>(APS_COMMANDS::CHIPCONFIGIO);
  cmd_bis.cnt = 1;
  ethernetRM_->send(ipAddr_, {{cmd_bis, 0, {}}});

  // Read the response
  // We expect a single datagram back with a single word
  auto result = ethernetRM_->read(ipAddr_, COMMS_TIMEOUT);
  if (result.cmd.mode_stat != 0x00) {
    FILE_LOG(logERROR) << ipAddr_
                       << " read_SPI response datagram reported error: "
                       << result.cmd.mode_stat;
    throw APS2_COMMS_ERROR;
  }
  if (result.payload.size() != 1) {
    FILE_LOG(logERROR)
        << ipAddr_
        << " read_SPI response datagram unexpected size: expected 1, got "
        << result.payload.size();
  }
  return result.payload[0] >> 24; // first response is in MSB of 32-bit word
}

// Flash read/write
void APS2::write_flash(uint32_t addr, vector<uint32_t> &data) {
  FILE_LOG(logDEBUG1) << ipAddr_ << " APS2::write_flash";

  // Flash writes must be 256 byte aligned and written in 256 byte chunks
  if ((addr & 0xff) != 0) {
    FILE_LOG(logERROR) << ipAddr_ << " attempted to write configuration ERPOM "
                                     "at an address not aligned to 256 bytes";
    throw APS2_UNALIGNED_MEMORY_ACCESS;
  }
  int pad_words = (64 - (data.size() % 64)) % 64;
  FILE_LOG(logDEBUG1) << ipAddr_ << " flash write: padding payload with "
                      << pad_words << " words";
  data.resize(data.size() + pad_words, 0xffffffff);

  // erase before write
  erase_flash(addr, sizeof(uint32_t) * data.size());

  APS2Command cmd;
  cmd.ack = 1;
  cmd.sel = 1;
  cmd.cmd = static_cast<uint32_t>(APS_COMMANDS::EPROMIO);
  cmd.mode_stat = EPROM_RW;
  auto dgs = APS2Datagram::chunk(
      cmd, addr, data,
      0x0100); // max chunk_size is limited wrapping in Ethernet frames
  FILE_LOG(logDEBUG1) << ipAddr_ << " flash write chunked into " << dgs.size()
                      << " datagrams.";
  bitfile_writing_task = WRITING;
  bitfile_writing_task_progress = 0;
  // Write 1 at a time so we can update progress
  for (size_t ct = 0; ct < dgs.size(); ct++) {
    ethernetRM_->send(ipAddr_, {dgs[ct]});
    bitfile_writing_task_progress =
        static_cast<double>(ct + 1) / static_cast<double>(dgs.size());
  }
}

void APS2::erase_flash(uint32_t start_addr, uint32_t num_bytes) {
  FILE_LOG(logDEBUG1) << ipAddr_ << " APS2::erase_flash";
  bitfile_writing_task = ERASING;
  bitfile_writing_task_progress = 0;

  // each erase command erases 64 KB of data starting at addr
  if ((start_addr % (1 << 16)) != 0) {
    FILE_LOG(logERROR)
        << ipAddr_ << " EPROM memory erase command addr was not 64KB aligned!";
    throw APS2_UNALIGNED_MEMORY_ACCESS;
  }

  FILE_LOG(logDEBUG) << ipAddr_ << " erasing " << num_bytes / (1 < 10)
                     << " kbytes starting at EPROM address "
                     << hexn<8> << start_addr;

  APS2Command cmd;
  cmd.ack = 1;
  cmd.sel = 1;
  cmd.cmd = static_cast<uint32_t>(APS_COMMANDS::EPROMIO);
  cmd.mode_stat = EPROM_ERASE;

  uint32_t addr{start_addr};
  uint32_t end_addr{start_addr + num_bytes};
  do {
    FILE_LOG(logDEBUG2) << ipAddr_ << " erasing 64kB page at EPROM addr "
                        << hexn<8> << addr;
    // Have noticed we don't always get a response so try a few times to make
    // sure
    unsigned tryct = 0;
    while (tryct < 5) {
      try {
        ethernetRM_->send(ipAddr_, {{cmd, addr, {}}});
      } catch (APS2_STATUS status) {
        if (status == APS2_RECEIVE_TIMEOUT) {
          FILE_LOG(logERROR) << ipAddr_
                             << " flash erase timed out. Retrying...";
          tryct++;
        } else {
          throw status;
        }
      } catch (...) {
        throw APS2_UNKNOWN_ERROR;
      }
      break;
    }
    if (tryct == 5) {
      FILE_LOG(logERROR) << ipAddr_ << " flash erase failed with timeout.";
      throw APS2_RECEIVE_TIMEOUT;
    }

    addr += (1 << 16);
    bitfile_writing_task_progress =
        static_cast<double>(addr - start_addr) / num_bytes;
  } while (addr < end_addr);
}

vector<uint32_t> APS2::read_flash(uint32_t addr, uint32_t num_words) {
  // TODO: handle reads that require multiple packets

  // Send the read request
  APS2Command cmd;
  cmd.r_w = 1;
  cmd.sel = 1;
  cmd.cmd = static_cast<uint32_t>(APS_COMMANDS::EPROMIO);
  cmd.cnt = num_words;
  ethernetRM_->send(ipAddr_, {{cmd, addr, {}}});

  // Read the response
  // We expect a single datagram back
  auto result = ethernetRM_->read(ipAddr_, COMMS_TIMEOUT);
  // TODO: error checking
  return result.payload;
}

void APS2::write_macip_flash(const uint64_t &mac, const uint32_t &ip_addr,
                             const bool &dhcp_enable) {
  uint32_t dhcp_int;

  dhcp_int = (dhcp_enable) ? 1 : 0;
  vector<uint32_t> data = {static_cast<uint32_t>(mac >> 16),
                           static_cast<uint32_t>((mac & 0xffff) << 16), ip_addr,
                           dhcp_int};
  write_flash(EPROM_MACIP_ADDR, data);
  // verify
  if (get_mac_addr() != mac) {
    throw APS2_MAC_ADDR_VALIDATION_FAILURE;
  }
  if (get_ip_addr() != ip_addr) {
    throw APS2_IP_ADDR_VALIDATION_FAILURE;
  }
  if (get_dhcp_enable() != dhcp_enable) {
    throw APS2_DHCP_VALIDATION_FAILURE;
  }
}

uint64_t APS2::get_mac_addr() {
  auto data = read_flash(EPROM_MACIP_ADDR, 2);
  return (static_cast<uint64_t>(data[0]) << 16) | (data[1] >> 16);
}

void APS2::set_mac_addr(const uint64_t &mac) {
  uint32_t ip_addr = get_ip_addr();
  bool dhcp_enable = get_dhcp_enable();
  write_macip_flash(mac, ip_addr, dhcp_enable);
}

uint32_t APS2::get_ip_addr() {
  return read_flash(EPROM_MACIP_ADDR + EPROM_IP_OFFSET, 1)[0];
}

void APS2::set_ip_addr(const uint32_t &ip_addr) {
  uint64_t mac = get_mac_addr();
  bool dhcp_enable = get_dhcp_enable();
  write_macip_flash(mac, ip_addr, dhcp_enable);
}

bool APS2::get_dhcp_enable() {
  uint32_t dhcp_enable = read_flash(EPROM_MACIP_ADDR + EPROM_DHCP_OFFSET, 1)[0];
  return ((dhcp_enable & 0x1) == 0x1);
}

void APS2::set_dhcp_enable(const bool &dhcp_enable) {
  uint64_t mac = get_mac_addr();
  uint32_t ip_addr = get_ip_addr();
  write_macip_flash(mac, ip_addr, dhcp_enable);
}

// Create/restore setup SPI sequence
void APS2::write_SPI_setup() {
  FILE_LOG(logINFO) << ipAddr_ << " writing SPI startup sequence";
  vector<uint32_t> msg = build_VCXO_SPI_msg(VCXO_INIT);
  vector<uint32_t> pll_msg = build_PLL_SPI_msg(PLL_INIT);
  msg.insert(msg.end(), pll_msg.begin(), pll_msg.end());
  // push on "sleep" for 8*256*100ns = 0.205ms
  msg.push_back(0x00000800);
  // push on "end of message"
  APSChipConfigCommand_t cmd;
  cmd.target = CHIPCONFIG_IO_TARGET_EOL;
  msg.push_back(cmd.packed);
  write_flash(0x0, msg);
}

/*
 *
 * Private Functions
 */

vector<uint32_t>
APS2::build_DAC_SPI_msg(const CHIPCONFIG_IO_TARGET &target,
                        const vector<SPI_AddrData_t> &addrData) {
  vector<uint32_t> msg;
  APSChipConfigCommand_t cmd;
  // force SINGLE writes for now
  switch (target) {
  case CHIPCONFIG_TARGET_DAC_0:
    cmd.target = CHIPCONFIG_IO_TARGET_DAC_0_SINGLE;
    break;
  case CHIPCONFIG_TARGET_DAC_1:
    cmd.target = CHIPCONFIG_IO_TARGET_DAC_1_SINGLE;
    break;
  default:
    FILE_LOG(logERROR) << ipAddr_ << " unexpected CHIPCONFIG_IO_TARGET";
    throw std::runtime_error("Unexpected CHIPCONFIG_IO_TARGET");
  }
  for (auto ad : addrData) {
    cmd.instr = ad.first;
    cmd.spicnt_data = ad.second;
    msg.push_back(cmd.packed);
  }
  return msg;
}

vector<uint32_t>
APS2::build_PLL_SPI_msg(const vector<SPI_AddrData_t> &addrData) {
  vector<uint32_t> msg;
  APSChipConfigCommand_t cmd;
  // force SINGLE writes for now
  cmd.target = CHIPCONFIG_IO_TARGET_PLL_SINGLE;
  for (auto ad : addrData) {
    cmd.instr = ad.first;
    cmd.spicnt_data = ad.second;
    msg.push_back(cmd.packed);
  }
  return msg;
}
vector<uint32_t> APS2::build_VCXO_SPI_msg(const vector<uint8_t> &data) {
  vector<uint32_t> msg;
  APSChipConfigCommand_t cmd;
  cmd.target = CHIPCONFIG_IO_TARGET_VCXO;
  cmd.instr = 0;
  cmd.spicnt_data = 0;

  if (data.size() % 4 != 0) {
    FILE_LOG(logERROR) << ipAddr_ << " VCXO messages must be 4-byte aligned";
    throw std::runtime_error("VCXO messages must be 4-byte aligned");
  }

  // pack 4 bytes into 1 32-bit word
  for (size_t ct = 0; ct < data.size(); ct += 4) {
    // alternate commands with data
    msg.push_back(cmd.packed);
    msg.push_back((data[ct] << 24) | (data[ct + 1] << 16) |
                  (data[ct + 2] << 8) | data[ct + 3]);
  }
  return msg;
}

int APS2::setup_PLL() {
  // set the on-board PLL to its default state (two 1.2 GHz outputs to DAC's,
  // 300 MHz sys_clk to FPGA, and 400 MHz mem_clk to FPGA)
  FILE_LOG(logINFO) << ipAddr_ << " running base-line setup of PLL";

  vector<uint32_t> msg = build_PLL_SPI_msg(PLL_INIT);
  write_SPI(msg);

  // Record that sampling rate has been set to 1200
  samplingRate_ = 1200;

  return 0;
}

int APS2::set_PLL_freq(const int &freq) {
  /* APS2::set_PLL_freq
   * fpga = FPGA1, FPGA2, or ALL_FPGAS
   * freq = frequency to set in MHz, allowed values are (1200, 600, 300, 200,
   * 100, 50, and 40)
   */

  uint32_t pllCyclesAddr, pllBypassAddr;
  uint8_t pllCyclesVal, pllBypassVal;

  FILE_LOG(logDEBUG) << ipAddr_ << " setting PLL FPGA: Freq.: " << freq;

  pllCyclesAddr = 0x190;
  pllBypassAddr = 0x191;

  switch (freq) {
  //		case 40: pllCyclesVal = 0xEE; break; // 15 high / 15 low (divide
  // by
  // 30)
  //		case 50: pllCyclesVal = 0xBB; break;// 12 high / 12 low (divide
  // by
  // 24)
  //		case 100: pllCyclesVal = 0x55; break; // 6 high / 6 low (divide
  // by
  // 12)
  case 200:
    pllCyclesVal = 0x22;
    break; // 3 high / 3 low (divide by 6)
  case 300:
    pllCyclesVal = 0x11;
    break; // 2 high /2 low (divide by 4)
  case 600:
    pllCyclesVal = 0x00;
    break; // 1 high / 1 low (divide by 2)
  case 1200:
    pllCyclesVal = 0x00;
    break; // value ignored, set bypass below
  default:
    return -2;
  }

  // bypass divider if freq == 1200
  pllBypassVal = (freq == 1200) ? 0x80 : 0x00;
  FILE_LOG(logDEBUG2) << ipAddr_ << " setting PLL cycles addr: "
                      << hexn<2> << pllCyclesAddr
                      << " val: " << hexn<2> << int(pllCyclesVal);
  FILE_LOG(logDEBUG2) << ipAddr_ << " setting PLL bypass addr: "
                      << hexn<2> << pllBypassAddr
                      << " val: " << hexn<2> << int(pllBypassVal);

  // Disable DDRs
  // int ddr_mask = CSRMSK_CHA_DDR | CSRMSK_CHB_DDR;
  // TODO: fix!
  //	FPGA::clear_register_bit(socket_, FPGA_ADDR_CSR, ddr_mask);

  // Disable oscillator by clearing APS2_STATUS_CTRL register
  // TODO: fix!
  //	if (APS2::clear_status_ctrl() != 1) return -4;

  // Setup of a vector of address-data pairs for all the writes we need for the
  // PLL routine
  const vector<SPI_AddrData_t> PLL_Routine = {
      {pllCyclesAddr, pllCyclesVal},
      {pllBypassAddr, pllBypassVal},
      // {0x230, 0x01}, // Initiate channel sync
      // {0x232, 0x1}, // Update registers
      // {0x230, 0x00}, // Clear SYNC register
      {0x232, 0x1} // Update registers
  };

  vector<uint32_t> msg = build_PLL_SPI_msg(PLL_Routine);
  write_SPI(msg);

  // Enable Oscillator
  // TODO: fix!
  //	if (APS2::reset_status_ctrl() != 1) return -4;

  // Enable DDRs
  // TODO: fix!
  //	FPGA::set_register_bit(socket_, FPGA_ADDR_CSR, ddr_mask);

  return 0;
}

void APS2::check_clocks_status() {
  /*
   * Reads the locked status of the clock distribution PLL and the FPGA MMCM's
   * (SYS_CLK, CFG_CLK and MEM_CLK)
   */
  FILE_LOG(logDEBUG1) << ipAddr_ << " APS2::check_clocks_status";

  // PLL chips are reported through status registers
  APSStatusBank_t statusRegs = read_status_registers();

  // First check the clock distribution PLL (the bottom three bits should be
  // high)
  FILE_LOG(logDEBUG1) << ipAddr_
                      << " PLL status: " << hexn<8> << statusRegs.pllStatus;
  if ((statusRegs.pllStatus & 0x3) != 0x3) {
    throw APS2_PLL_LOST_LOCK;
  }

  // On board MMCM PLL's are reported either through user status or the
  // PLL_STATUS_ADDR CSR register
  uint32_t pll_status;
  if (legacy_firmware) {
    pll_status = statusRegs.userStatus;
  } else {
    pll_status = read_memory(PLL_STATUS_ADDR, 1).front();
  }
  bool clocksGood = true;
  for (auto bit : {MMCM_SYS_LOCK_BIT, MMCM_CFG_LOCK_BIT, MIG_C0_LOCK_BIT,
                   MIG_C0_CAL_BIT, MIG_C1_LOCK_BIT, MIG_C1_CAL_BIT}) {
    clocksGood &= (pll_status >> bit) & 0x1;
  }
  if (!clocksGood) {
    throw APS2_MMCM_LOST_LOCK;
  }
}

int APS2::get_PLL_freq() {
  // Poll APS2 PLL chip to determine current frequency in MHz
  FILE_LOG(logDEBUG1) << ipAddr_ << " APS2::get_PLL_freq";

  int freq = 0;
  uint16_t pll_cycles_addr = 0x190;
  uint16_t pll_bypass_addr = 0x191;

  uint32_t pll_cycles_val = read_SPI(CHIPCONFIG_TARGET_PLL, pll_cycles_addr);
  uint32_t pll_bypass_val = read_SPI(CHIPCONFIG_TARGET_PLL, pll_bypass_addr);

  FILE_LOG(logDEBUG3) << ipAddr_
                      << " pll_cycles_val = " << hexn<2> << pll_cycles_val;
  FILE_LOG(logDEBUG3) << ipAddr_
                      << " pll_bypass_val = " << hexn<2> << pll_bypass_val;

  // select frequency based on pll cycles setting
  // the values here should match the reverse lookup in APS2::set_PLL_freq

  if ((pll_bypass_val & 0x80) == 0x80 && pll_cycles_val == 0x00)
    freq = 1200;
  else {
    switch (pll_cycles_val) {
    case 0xEE:
      freq = 40;
      break;
    case 0xBB:
      freq = 50;
      break;
    case 0x55:
      freq = 100;
      break;
    case 0x22:
      freq = 200;
      break;
    case 0x11:
      freq = 300;
      break;
    case 0x00:
      freq = 600;
      break;
    default:
        FILE_LOG(logERROR) << "Unexpected PLL cycles value";
        FILE_LOG(logERROR) << ipAddr_
                            << " pll_cycles_val = " << hexn<2> << pll_cycles_val;
        FILE_LOG(logERROR) << ipAddr_
                            << " pll_bypass_val = " << hexn<2> << pll_bypass_val;
      throw APS2_BAD_PLL_VALUE;
    }
  }

  FILE_LOG(logDEBUG1) << ipAddr_ << " PLL frequency " << freq;

  return freq;
}

void APS2::enable_DAC_clock(const int dac) {
  check_channel_num(dac);
  // enables the PLL output to a DAC (0 or 1)
  const vector<uint16_t> DAC_PLL_ADDR = {0xF0, 0xF1};

  vector<SPI_AddrData_t> enable_msg = {{DAC_PLL_ADDR[dac], 0x00}, {0x232, 0x1}};
  auto msg = build_PLL_SPI_msg(enable_msg);
  write_SPI(msg);
}

void APS2::disable_DAC_clock(const int dac) {
  check_channel_num(dac);
  const vector<uint16_t> DAC_PLL_ADDR = {0xF0, 0xF1};

  vector<SPI_AddrData_t> disable_msg = {{DAC_PLL_ADDR[dac], 0x02},
                                        {0x232, 0x1}};
  auto msg = build_PLL_SPI_msg(disable_msg);
  write_SPI(msg);
}

void APS2::toggle_DAC_clock(const int dac) {
  disable_DAC_clock(dac);
  enable_DAC_clock(dac);
}

void APS2::setup_VCXO() {
  // Write the standard VCXO setup

  FILE_LOG(logINFO) << ipAddr_ << " setting up VCX0";

  // ensure the oscillator is disabled before programming
  // TODO: fix!
  //	if (APS2::clear_status_ctrl() != 1)
  //		return -1;

  vector<uint32_t> msg = build_VCXO_SPI_msg(VCXO_INIT);
  write_SPI(msg);
}

void APS2::align_DAC_clock(int dac) {
  check_channel_num(dac);
  // toggle DAC clocks until DATACLK_OUT comes up "aligned" to system 600
  const vector<uint32_t> PHASE_COUNT_ADDR = {PHASE_COUNT_A_ADDR,
                                             PHASE_COUNT_B_ADDR};
  // Loop over number of tries
  for (int ct = 0; ct < MAX_DAC_CLOCK_PHASE_TEST_TRIES; ct++) {
    // register returns a value in [0, 0xffff]. Re-interpret as portion of
    // circle
    auto dac_clk_phase =
        static_cast<double>(read_memory(PHASE_COUNT_ADDR[dac], 1)[0] & 0xffff) /
        (0xffff);

    FILE_LOG(logDEBUG1) << ipAddr_ << " measured DAC "
                        << ((dac == 0) ? "A" : "B") << " clock phase of "
                        << dac_clk_phase;

    if (dac_clk_phase < 0.5) {
      // done with this channel
      return;
    } else {
      // toggle DAC clock to try and get different phase
      toggle_DAC_clock(dac);
      // wait for phase count to run
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }
  // if we got here then we never aligned
  FILE_LOG(logERROR) << ipAddr_ << " failed to align DAC " << dac;
}

void APS2::align_DAC_LVDS_capture(int dac)
/*
 * Description: Aligns the data valid window of the DAC with the output of the
 * FPGA.
 * inputs: dac = 0 or 1
 */
{

  uint8_t data;
  vector<uint32_t> msg;
  uint8_t SD, MSD, MHD;
  uint8_t edgeMSD, edgeMHD;

  check_channel_num(dac);
  FILE_LOG(logINFO) << ipAddr_ << " setting up DAC " << dac;

  const vector<CHIPCONFIG_IO_TARGET> targets = {CHIPCONFIG_TARGET_DAC_0,
                                                CHIPCONFIG_TARGET_DAC_1};

  // Step 0: check control clock divider
  data = read_SPI(targets[dac], DAC_CONTROLLERCLOCK_ADDR);
  FILE_LOG(logDEBUG1) << ipAddr_ << " DAC controller clock divider register = "
                      << (data & 0xf);
  // Max freq is 1.2GS/s so dividing by 128 gets us below 10MHz for sure
  msg = build_DAC_SPI_msg(targets[dac], {{DAC_CONTROLLERCLOCK_ADDR, 5}});
  write_SPI(msg);

  // disable SYNC FIFO
  disable_DAC_FIFO(dac);

  // Step 1: calibrate and set the LVDS controller.
  // get initial states of registers

  // TODO: remove int(... & 0x1F)
  data = read_SPI(targets[dac], DAC_INTERRUPT_ADDR);
  FILE_LOG(logDEBUG2) << ipAddr_
                      << " reg: " << hexn<2> << int(DAC_INTERRUPT_ADDR & 0x1F)
                      << " Val: " << int(data & 0xFF);
  data = read_SPI(targets[dac], DAC_MSDMHD_ADDR);
  FILE_LOG(logDEBUG2) << ipAddr_
                      << " reg: " << hexn<2> << int(DAC_MSDMHD_ADDR & 0x1F)
                      << " Val: " << int(data & 0xFF);
  data = read_SPI(targets[dac], DAC_SD_ADDR);
  FILE_LOG(logDEBUG2) << ipAddr_
                      << " reg: " << hexn<2> << int(DAC_SD_ADDR & 0x1F)
                      << " Val: " << int(data & 0xFF);

  // Ensure that surveilance and auto modes are off
  data = read_SPI(targets[dac], DAC_CONTROLLER_ADDR);
  FILE_LOG(logDEBUG2) << ipAddr_
                      << " reg: " << hexn<2> << int(DAC_CONTROLLER_ADDR & 0x1F)
                      << " Val: " << int(data & 0xFF);
  data = 0;
  msg = build_DAC_SPI_msg(targets[dac], {{DAC_CONTROLLER_ADDR, data}});
  write_SPI(msg);

  // Slide the data valid window left (with MSD) and check for the interrupt
  SD = 0;  //(sample delay nibble, stored in Reg. 5, bits 7:4)
  MSD = 0; //(setup delay nibble, stored in Reg. 4, bits 7:4)
  MHD = 0; //(hold delay nibble,	stored in Reg. 4, bits 3:0)

  set_DAC_SD(dac, SD);

  for (MSD = 0; MSD < 16; MSD++) {
    FILE_LOG(logDEBUG2) << ipAddr_ << " setting MSD: " << int(MSD);

    data = (MSD << 4) | MHD;
    msg = build_DAC_SPI_msg(targets[dac], {{DAC_MSDMHD_ADDR, data}});
    write_SPI(msg);
    FILE_LOG(logDEBUG2) << ipAddr_ << " write Reg: "
                        << hexn<2> << int(DAC_MSDMHD_ADDR & 0x1F)
                        << " Val: " << int(data & 0xFF);

    data = read_SPI(targets[dac], DAC_SD_ADDR);
    FILE_LOG(logDEBUG2) << ipAddr_
                        << " read Reg: " << hexn<2> << int(DAC_SD_ADDR & 0x1F)
                        << " Val: " << int(data & 0xFF);

    bool check = data & 1;
    FILE_LOG(logDEBUG2) << ipAddr_ << " check: " << check;
    if (!check)
      break;
  }
  edgeMSD = MSD;
  FILE_LOG(logDEBUG) << ipAddr_ << " found MSD: " << int(edgeMSD);

  // Clear the MSD, then slide right (with MHD)
  MSD = 0;
  for (MHD = 0; MHD < 16; MHD++) {
    FILE_LOG(logDEBUG2) << ipAddr_ << " setting MHD: " << int(MHD);

    data = (MSD << 4) | MHD;
    msg = build_DAC_SPI_msg(targets[dac], {{DAC_MSDMHD_ADDR, data}});
    write_SPI(msg);

    data = read_SPI(targets[dac], DAC_SD_ADDR);
    FILE_LOG(logDEBUG2) << ipAddr_ << " read: " << hexn<2> << int(data & 0xFF);
    bool check = data & 1;
    FILE_LOG(logDEBUG2) << ipAddr_ << " check: " << check;
    if (!check)
      break;
  }
  edgeMHD = MHD;
  FILE_LOG(logDEBUG) << ipAddr_ << " found MHD = " << int(edgeMHD);
  SD = (edgeMHD - edgeMSD) / 2;

  // Clear MSD and MHD
  MHD = 0;
  data = (MSD << 4) | MHD;
  msg = build_DAC_SPI_msg(targets[dac], {{DAC_MSDMHD_ADDR, data}});
  write_SPI(msg);

  // Set the optimal sample delay (SD)
  set_DAC_SD(dac, SD);

  // AD9376 data sheet advises us to enable surveilance and auto modes, but this
  // has introduced output glitches in limited testing
  // set the filter length, threshold, and enable surveilance mode and auto mode
  // int filter_length = 12;
  // int threshold = 1;
  // data = (1 << 7) | (1 << 6) | (filter_length << 2) | (threshold & 0x3);
  // msg = build_DAC_SPI_msg(targets[dac], {{DAC_CONTROLLER_ADDR, data}});
  // write_SPI(msg);

  // turn on SYNC FIFO
  // enable_DAC_FIFO(dac);
}

void APS2::set_DAC_SD(const int &dac, const uint8_t &sd) {
  // Sets the sample delay
  FILE_LOG(logDEBUG) << ipAddr_ << " setting SD = " << int(sd);
  const vector<CHIPCONFIG_IO_TARGET> targets = {CHIPCONFIG_TARGET_DAC_0,
                                                CHIPCONFIG_TARGET_DAC_1};
  auto msg =
      build_DAC_SPI_msg(targets[dac], {{DAC_SD_ADDR, ((sd & 0xf) << 4)}});
  write_SPI(msg);
}

void APS2::run_chip_config(uint32_t addr /* default = 0 */) {
  FILE_LOG(logINFO) << ipAddr_ << " running chip config from address "
                    << hexn<8> << addr;
  // construct the chip config command
  APS2Command cmd;
  cmd.ack = 1;
  cmd.cmd = static_cast<uint32_t>(APS_COMMANDS::RUNCHIPCONFIG);
  ethernetRM_->send(ipAddr_, {{cmd, addr, {}}});
}

int APS2::get_DAC_FIFO_phase(const int dac) {
  check_channel_num(dac);
  const vector<CHIPCONFIG_IO_TARGET> targets = {CHIPCONFIG_TARGET_DAC_0,
                                                CHIPCONFIG_TARGET_DAC_1};

  auto data = read_SPI(targets[dac], DAC_FIFOSTAT_ADDR);
  FILE_LOG(logDEBUG2) << ipAddr_ << " read: " << hexn<2> << int(data & 0xFF);

  uint8_t phase = (data & 0x70) >> 4;  // phase (FIFOPHASE) is in bits <6:4>
  uint8_t valid = (data >> 3) & 0x1;   // VALID is bit 3
  uint8_t status = (data  >> 7) & 0x1; // error STATUS is bit 7
  FILE_LOG(logDEBUG) << ipAddr_ << " DAC " << dac
                     << " FIFO phase = " << int(phase)
                     << " (valid=" << int(valid)
                     << ", status=" << int(status) << ")";
  return phase;
}

void APS2::enable_DAC_FIFO(const int &dac) {

  if (dac < 0 || dac >= NUM_ANALOG_CHANNELS) {
    FILE_LOG(logERROR) << ipAddr_ << " APS2::enable_DAC_FIFO unknown DAC, "
                       << dac;
    throw APS2_INVALID_DAC;
  }

  uint8_t data = 0;
  FILE_LOG(logDEBUG) << ipAddr_ << " enabling DAC " << dac << " FIFO";
  const vector<CHIPCONFIG_IO_TARGET> targets = {CHIPCONFIG_TARGET_DAC_0,
                                                CHIPCONFIG_TARGET_DAC_1};

  // set sync bit (Reg 0, bit 2)
  data = read_SPI(targets[dac], DAC_SYNC_ADDR);
  data = data | (1 << 2);
  vector<uint32_t> msg =
      build_DAC_SPI_msg(targets[dac], {{DAC_SYNC_ADDR, data}});
  write_SPI(msg);

  // clear OFFSET bits
  msg = build_DAC_SPI_msg(targets[dac], {{DAC_FIFOSTAT_ADDR, 0}});
  write_SPI(msg);

  // read back FIFO phase to ensure we are in a safe zone
  uint8_t phase = get_DAC_FIFO_phase(dac);

  // fix the FIFO phase if too close together
  // want to get to a phase of 3 or 4, but can only change it by decreasing in
  // steps of 2 (mod 8),
  // i.e. setting OFFSET = 1, decreases the phase by 2
  // reduce the problem to making 0, 1, 2, 3 move towards 2
  int8_t offset = ((phase + 1) / 2) % 4;
  offset = mymod(offset - 2, 4);
  FILE_LOG(logDEBUG) << ipAddr_ << " setting FIFO offset = " << int(offset);

  msg = build_DAC_SPI_msg(targets[dac], {{DAC_FIFOSTAT_ADDR, offset}});
  write_SPI(msg);

  // verify by measuring FIFO offset again
  get_DAC_FIFO_phase(dac);
}

void APS2::disable_DAC_FIFO(const int &dac) {
  uint8_t data, mask;
  const vector<CHIPCONFIG_IO_TARGET> targets = {CHIPCONFIG_TARGET_DAC_0,
                                                CHIPCONFIG_TARGET_DAC_1};

  FILE_LOG(logDEBUG1) << ipAddr_ << " disable DAC " << dac << " FIFO";
  // clear sync bit
  data = read_SPI(targets[dac], DAC_SYNC_ADDR);
  mask = (0x1 << 2);
  vector<uint32_t> msg =
      build_DAC_SPI_msg(targets[dac], {{DAC_SYNC_ADDR, data & ~mask}});
  write_SPI(msg);
}

bool APS2::run_DAC_BIST(const int &dac, const vector<int16_t> &testVec,
                        vector<uint32_t> &results) {
  /*
  Measures the DAC BIST registers for a given test vector at three stages:
  leaving the FPGA, registering
  the data on the DAC at the LVDS stage, syncronizing to the output stage on the
  DAC. It returns the BIST
  results as a vector:
  results = {IdealPhase1, IdealPhase2, FPGAPhase1, FPGAPhase2, LVDSPhase1,
  LVDSPhase2, SYNCPhase1, SYNCPhase2}
  */
  const vector<CHIPCONFIG_IO_TARGET> targets = {CHIPCONFIG_TARGET_DAC_0,
                                                CHIPCONFIG_TARGET_DAC_1};
  const vector<uint32_t> FPGA_Reg_Phase1 = {DAC_BIST_CHA_PH1_ADDR,
                                            DAC_BIST_CHB_PH1_ADDR};
  const vector<uint32_t> FPGA_Reg_Phase2 = {DAC_BIST_CHA_PH2_ADDR,
                                            DAC_BIST_CHB_PH2_ADDR};
  const vector<unsigned> FPGA_Reg_ResetBits = {DAC_BIST_CHA_RST_BIT,
                                               DAC_BIST_CHB_RST_BIT};
  uint8_t regVal;

  FILE_LOG(logINFO) << ipAddr_ << " running DAC BIST for DAC " << dac;

  // A few helper lambdas

  // Read/write DAC SPI registers
  auto read_reg = [&](const uint16_t &addr) {
    return read_SPI(targets[dac], addr);
  };
  auto write_reg = [&](const uint16_t &addr, const uint8_t &data) {
    vector<uint32_t> msg = build_DAC_SPI_msg(targets[dac], {{addr, data}});
    write_SPI(msg);
  };

  // Read the BIST signature
  auto read_BIST_sig = [&]() {
    vector<uint32_t> bistVals;
    // Read the on-board values first
    bistVals.push_back(read_memory(FPGA_Reg_Phase1[dac], 1)[0]);
    FILE_LOG(logDEBUG1) << ipAddr_ << " FPGA Phase 1 BIST "
                        << hexn<8> << bistVals.back();
    bistVals.push_back(read_memory(FPGA_Reg_Phase2[dac], 1)[0]);
    FILE_LOG(logDEBUG1) << ipAddr_ << " FPGA Phase 2 BIST "
                        << hexn<8> << bistVals.back();

    // Now the DAC registers
    // LVDS Phase 1 Reg 17 (SEL1=0; SEL0=0; SIGREAD=1; SYNC_EN=1; LVDS_EN=1)
    // Not the BIST byte ordering seems to be backwards to the data sheet
    write_reg(17, 0x26);
    bistVals.push_back((read_reg(18) << 24) | (read_reg(19) << 16) |
                       (read_reg(20) << 8) | read_reg(21));
    FILE_LOG(logDEBUG1) << ipAddr_ << " LVDS Phase 1 BIST "
                        << hexn<8> << bistVals.back();

    // LVDS Phase 2
    write_reg(17, 0x66);
    bistVals.push_back((read_reg(18) << 24) | (read_reg(19) << 16) |
                       (read_reg(20) << 8) | read_reg(21));
    FILE_LOG(logDEBUG1) << ipAddr_ << " LVDS Phase 2 BIST "
                        << hexn<8> << bistVals.back();

    // SYNC Phase 1
    write_reg(17, 0xA6);
    bistVals.push_back((read_reg(18) << 24) | (read_reg(19) << 16) |
                       (read_reg(20) << 8) | read_reg(21));
    FILE_LOG(logDEBUG1) << ipAddr_ << " SYNC Phase 1 BIST "
                        << hexn<8> << bistVals.back();

    // SYNC Phase 2
    write_reg(17, 0xE6);
    bistVals.push_back((read_reg(18) << 24) | (read_reg(19) << 16) |
                       (read_reg(20) << 8) | read_reg(21));
    FILE_LOG(logDEBUG1) << ipAddr_ << " SYNC Phase 2 BIST "
                        << hexn<8> << bistVals.back();

    return bistVals;
  };

  // Calculate expected signature
  auto calc_bist = [&](const vector<int16_t> dataVec) {
    uint32_t lfsr = 0;
    const uint32_t maskBottom14 = 0x3fff;
    for (auto v : dataVec) {
      // modulator shifts positive values down by 1
      if ((v != 0) && (v != 1)) {
        if (v > 0) {
          v -= 1;
        }
        // Shift
        lfsr = (lfsr << 1) | (lfsr >> 31);
        // Now ~xor the bottom 14 bits with the input data
        lfsr = (lfsr & ~maskBottom14) | (~(lfsr ^ v) & maskBottom14);
      }
    }
    return lfsr;
  };

  // The two different phases take the even/odd samples
  vector<int16_t> evenSamples, oddSamples;
  bool toggle = false;
  std::partition_copy(testVec.begin(), testVec.end(),
                      std::back_inserter(oddSamples),
                      std::back_inserter(evenSamples),
                      [&toggle](int) { return toggle = !toggle; });

  //
  uint32_t phase1BIST = calc_bist(oddSamples);
  uint32_t phase2BIST = calc_bist(evenSamples);

  FILE_LOG(logDEBUG) << ipAddr_ << " expected phase 1 BIST register "
                     << hexn<8> << phase1BIST;
  FILE_LOG(logDEBUG) << ipAddr_ << " expected phase 2 BIST register "
                     << hexn<8> << phase2BIST;

  // Load the test vector and setup software triggered waveform mode
  // Clear the channel data on both channels so we get the right waveform length
  clear_channel_data();
  set_waveform(dac, testVec);
  // zero out waveform on other DAC so there is no interaction through the
  // modulator
  vector<int16_t> zero_wf = vector<int16_t>(testVec.size(), 0);
  set_waveform(dac == 0 ? 1 : 0, zero_wf);
  // reset the modulator parameters
  set_waveform_frequency(0.0);
  set_mixer_phase_skew(0.0);
  set_mixer_amplitude_imbalance(1.0);
  set_channel_scale(dac, 1.0);

  // setup for software triggered waveform so we can send the test pattern
  // through on our mark
  set_run_mode(TRIG_WAVEFORM);
  set_trigger_source(SOFTWARE);
  run();

  // Following Page 45 of the AD9736 datasheet
  // Step 0: save the current state of SD and control clock divider
  auto ccd = read_reg(DAC_CONTROLLERCLOCK_ADDR);
  auto sd = read_reg(DAC_SD_ADDR);

  // Step 1: set the reset bit (Reg 0, bit 5)
  regVal = read_reg(0);
  regVal |= (1 << 5);
  write_reg(0, regVal);

  // Step 2: set the zero register to 0x0000 for signed data (note inconsistency
  // between page 44 and 45 of manual)
  set_channel_offset(dac, 0);

  // Step 3 and 4 : run data clock for 16 cycles -- should happen with ethernet
  // latency for free

  // Step 5: clear the reset bit
  regVal &= ~(1 << 5);
  write_reg(0, regVal);

  // Step 6: wait

  // Step 7: set the reset bit
  regVal |= (1 << 5);
  write_reg(0, regVal);

  // Step 8: wait

  // Step 9: clear reset bit
  regVal &= ~(1 << 5);
  write_reg(0, regVal);

  // Step 10: set operating mode
  // clock controller
  write_reg(DAC_SD_ADDR, sd);
  write_reg(DAC_CONTROLLERCLOCK_ADDR, ccd);

  // Step 11: Set CLEAR (Reg. 17, Bit 0), SYNC_EN (Reg. 17, Bit 1), and LVDS_EN
  // (Reg. 17, Bit 2) high
  write_reg(17, 0x07);
  // Also reset the FPGA BIST registers at this point
  set_register_bit(RESETS_ADDR, {FPGA_Reg_ResetBits[dac]});

  // Step 12: wait...

  // Step 13: clear the CLEAR bit
  write_reg(17, 0x06);
  clear_register_bit(RESETS_ADDR, {FPGA_Reg_ResetBits[dac]});

  // Step 14: read the BIST registers to confirm all zeros
  // Note error in part (b's) should read registers 21-18
  read_BIST_sig();

  // Step 15: Clock in a single run of the waveform
  trigger();

  // Step 16: data held at zero because outputValid is low

  // Step 17: read the BIST registers again
  read_BIST_sig();

  // Step 18: loop back to 11
  write_reg(17, 0x07);
  set_register_bit(RESETS_ADDR, {FPGA_Reg_ResetBits[dac]});
  write_reg(17, 0x06);
  clear_register_bit(RESETS_ADDR, {FPGA_Reg_ResetBits[dac]});
  read_BIST_sig();
  trigger();

  results.clear();
  results.push_back(phase1BIST);
  results.push_back(phase2BIST);
  auto readResults = read_BIST_sig();
  results.insert(results.end(), readResults.begin(), readResults.end());

  stop();

  // Pump the reset line to turn off BIST mode
  regVal = read_reg(0);
  regVal |= (1 << 5);
  write_reg(0, regVal);
  regVal &= ~(1 << 5);
  write_reg(0, regVal);

  // restore registers
  write_reg(DAC_SD_ADDR, sd);
  write_reg(DAC_CONTROLLERCLOCK_ADDR, ccd);

  // check if DAC is bitslipped and swap results accordingly
  auto bitslip =
      read_memory(dac == 0 ? BITSLIP_A_ADDR : BITSLIP_B_ADDR, 1).front();
  if (bitslip % 2) {
    FILE_LOG(logDEBUG)
        << ipAddr_ << " DAC " << dac
        << " has odd bitslip so swapping LVDS and SYNC BIST vals";
    std::swap(results[4], results[5]);
    std::swap(results[6], results[7]);
  }

  bool passed = (results[2] == phase1BIST) && (results[3] == phase2BIST) &&
                (results[4] == phase1BIST) && (results[5] == phase2BIST) &&
                (results[6] == phase1BIST) && (results[7] == phase2BIST);
  return passed;
}

void APS2::set_register_bit(const uint32_t &addr,
                            std::initializer_list<size_t> bits) {
  std::bitset<32> reg_val(read_memory(addr, 1).front());
  FILE_LOG(logDEBUG) << "Updating 0x" << std::hex << addr << " From 0x" << reg_val.to_ulong();
  for (auto bit : bits) {
    reg_val.set(bit);
  }
  FILE_LOG(logDEBUG) << "Updating 0x" << std::hex << addr << " To 0x" << reg_val.to_ulong() ;
  write_memory(addr, reg_val.to_ulong());
}

void APS2::clear_register_bit(const uint32_t &addr,
                              std::initializer_list<size_t> bits) {
  std::bitset<32> reg_val(read_memory(addr, 1).front());
  FILE_LOG(logDEBUG) << "Updating 0x" << std::hex << addr << " From 0x" << reg_val.to_ulong();
  for (auto bit : bits) {
    reg_val.reset(bit);
  }
  FILE_LOG(logDEBUG) << "Updating 0x" << std::hex << addr << " To 0x" << reg_val.to_ulong() << std::dec;
  write_memory(addr, reg_val.to_ulong());
}

void APS2::write_waveform(const int &ch, const vector<int16_t> &wf) {
  /*Write waveform data to FPGA memory
   * ch = channel (0-1)
   * wf = bits 0-13: signed 14-bit waveform data, bits 14-15: marker data
   */

  // disable/reset cache
  clear_register_bit(CACHE_CONTROL_ADDR, {CACHE_ENABLE_BIT});

  uint32_t startAddr =
      (ch == 0) ? MEMORY_ADDR + WFA_OFFSET : MEMORY_ADDR + WFB_OFFSET;
  FILE_LOG(logDEBUG2) << ipAddr_ << " loading waveform of length " << wf.size()
                      << " at address " << hexn<8> << startAddr;
  vector<uint32_t> packed_data;
  for (size_t ct = 0; ct < wf.size(); ct += 2) {
    packed_data.push_back(((uint32_t)wf[ct + 1] << 16) | (uint16_t)wf[ct]);
  }

  // SDRAM writes must be multiples of 16 bytes
  int pad_words = (4 - (packed_data.size() % 4)) % 4;
  if (pad_words) {
    FILE_LOG(logDEBUG1) << ipAddr_ << " padding waveform write with "
                        << std::dec << pad_words << " words";
    packed_data.resize(packed_data.size() + pad_words, 0xffffffff);
  }


  write_memory(startAddr, packed_data);

  // write the length register
  uint32_t length_addr = (ch == 0) ? CH_A_WF_LENGTH_ADDR : CH_B_WF_LENGTH_ADDR;
  write_memory(length_addr, wf.size());

  // enable cache
  set_register_bit(CACHE_CONTROL_ADDR, {CACHE_ENABLE_BIT});
}

void APS2::write_sequence(const vector<uint64_t> &data) {
  FILE_LOG(logDEBUG2) << ipAddr_ << " loading sequence of length "
                      << data.size();

  // pack into uint32_t vector
  vector<uint32_t> packed_instructions;
  for (size_t ct = 0; ct < data.size(); ct++) {
    packed_instructions.push_back(static_cast<uint32_t>(data[ct] & 0xffffffff));
    packed_instructions.push_back(static_cast<uint32_t>(data[ct] >> 32));
  }

  // SDRAM writes must be multiples of 16 bytes
  int pad_words = (4 - (packed_instructions.size() % 4)) % 4;
  if (pad_words) {
    FILE_LOG(logDEBUG1) << ipAddr_ << " padding instruction write with "
                        << std::dec << pad_words << " words";
    packed_instructions.resize(packed_instructions.size() + pad_words,
                               0xffffffff);
  }

  // disable/reset cache
  if (host_type == APS) {
    clear_register_bit(CACHE_CONTROL_ADDR, {CACHE_ENABLE_BIT});
  }

  int addr = 0;
  for ( auto d : packed_instructions) {
    FILE_LOG(logDEBUG) << "W Sequence[" << addr++ <<"] = 0x" << std::hex << d;
  }

  write_memory(MEMORY_ADDR + SEQ_OFFSET, packed_instructions);

  //read_sequence(MEMORY_ADDR + SEQ_OFFSET, packed_instructions.size());

  // enable cache
  if (host_type == APS) {
    set_register_bit(CACHE_CONTROL_ADDR, {CACHE_ENABLE_BIT});
  }
}

void APS2::read_sequence(const uint32_t addr, uint32_t num_words) {
  auto data = read_memory(addr, num_words);
  int laddr = 0;
  for ( auto d : data) {
    FILE_LOG(logDEBUG) << "R Sequence[" << laddr++ <<"] = 0x" << std::hex << d;
  }
}

int APS2::write_memory_map(const uint32_t &wfA, const uint32_t &wfB,
                           const uint32_t &seq) { /* see header for defaults */
  /* Writes the partitioning of external memory to registers. Takes 3 offsets
   * (in bytes) for wfA/B and seq data */
  FILE_LOG(logDEBUG2) << ipAddr_
                      << " writing memory map with offsets wfA: " << wfA
                      << ", wfB: " << wfB << ", seq: " << seq;

  write_memory(WFA_OFFSET_ADDR, MEMORY_ADDR + wfA);
  write_memory(WFB_OFFSET_ADDR, MEMORY_ADDR + wfB);
  write_memory(SEQ_OFFSET_ADDR, MEMORY_ADDR + seq);

  return 0;
}

// TODO: implement
/*
int APS2::save_state_file(string & stateFile){

        if (stateFile.length() == 0) {
                stateFile += "cache_" + ipAddr_ + ".h5";
        }

        FILE_LOG(logDEBUG) << "Writing State For Device: " << ipAddr_ << " to
hdf5 file: " << stateFile;
        H5::H5File H5StateFile(stateFile, H5F_ACC_TRUNC);
        string rootStr = "";
        write_state_to_hdf5(H5StateFile, rootStr);
        //Close the file
        H5StateFile.close();
        return 0;
}

int APS2::read_state_file(string & stateFile){

        if (stateFile.length() == 0) {
                stateFile += "cache_" + ipAddr_ + ".h5";
        }

        FILE_LOG(logDEBUG) << "Reading State For Device: " << ipAddr_ << " from
hdf5 file: " << stateFile;
        H5::H5File H5StateFile(stateFile, H5F_ACC_RDONLY);
        string rootStr = "";
        read_state_from_hdf5(H5StateFile, rootStr);
        //Close the file
        H5StateFile.close();
        return 0;
}

int APS2::write_state_to_hdf5(H5::H5File & H5StateFile, const string & rootStr){
        std::ostringstream tmpStream;
        //For now assume 4 channel data
        for(int chanct=0; chanct<4; chanct++){
                tmpStream.str("");
                tmpStream << rootStr << "/chan_" << chanct+1;
                FILE_LOG(logDEBUG) << "Writing State For Channel " << chanct + 1
<< " to hdf5 file";
                FILE_LOG(logDEBUG) << "Creating Group: " << tmpStream.str();
                H5::Group tmpGroup = H5StateFile.createGroup(tmpStream.str());
                tmpGroup.close();
                channels_[chanct].write_state_to_hdf5(H5StateFile,tmpStream.str());
        }
        return 0;
}

int APS2::read_state_from_hdf5(H5::H5File & H5StateFile, const string &
rootStr){
        //For now assume 4 channel data
        std::ostringstream tmpStream;
        for(int chanct=0; chanct<4; chanct++){
                tmpStream.str("");
                tmpStream << rootStr << "/chan_" << chanct+1;
                FILE_LOG(logDEBUG) << "Reading State For Channel " << chanct +
1<< " from hdf5 file";
                channels_[chanct].read_state_from_hdf5(H5StateFile,tmpStream.str());
        }
        return 0;
}

*/

string APS2::print_status_bank(const APSStatusBank_t &status) {
  std::ostringstream ret;

  ret << endl << endl;
  ret << "Host Firmware Version = " << hexn<8> << status.hostFirmwareVersion
      << endl;
  ret << "User Firmware Version = " << hexn<8> << status.userFirmwareVersion
      << endl;
  ret << "Configuration Source	= " << hexn<8> << status.configurationSource
      << endl;
  ret << "User Status           = " << hexn<8> << status.userStatus << endl;
  ret << "DAC 0 Status          = " << hexn<8> << status.dac0Status << endl;
  ret << "DAC 1 Status          = " << hexn<8> << status.dac1Status << endl;
  ret << "PLL Status            = " << hexn<8> << status.pllStatus << endl;
  ret << "VCXO Status           = " << hexn<8> << status.vcxoStatus << endl;
  ret << std::dec;
  ret << "Send Packet Count     = " << status.sendPacketCount << endl;
  ret << "Recv Packet Count     = " << status.receivePacketCount << endl;
  ret << "Seq Skip Count        = " << status.sequenceSkipCount << endl;
  ret << "Seq Dup.	Count       = " << status.sequenceDupCount << endl;
  ret << "FCS Overrun Count     = " << status.fcsOverrunCount << endl;
  ret << "Packet Overrun Count  = " << status.packetOverrunCount << endl;
  ret << "Uptime (s)            = " << status.uptimeSeconds << endl;
  ret << "Uptime (ns)           = " << status.uptimeNanoSeconds << endl;
  return ret.str();
}

string APS2::printAPSChipCommand(APSChipConfigCommand_t &cmd) {
  std::ostringstream ret;

  ret << std::hex << cmd.packed << " =";
  ret << " Target: " << cmd.target;
  ret << " SPICNT_DATA: " << cmd.spicnt_data;
  ret << " INSTR: " << cmd.instr;
  return ret.str();
}

string APS2::print_firmware_version(uint32_t version_reg) {
  std::ostringstream ret;

  uint32_t minor = version_reg & 0xff;
  uint32_t major = (version_reg >> 8) & 0xf;
  uint32_t sha1_nibble = (version_reg >> 12) & 0xfffff;
  string note;
  switch (sha1_nibble) {
  case 0x00000:
    note = "";
    break;
  case 0x0000a:
    note = "-dev";
    break;
  default:
    std::ostringstream sha1;
    sha1 << "-dev-" << std::hex << sha1_nibble;
    note = sha1.str();
    break;
  }
  ret << major << "." << minor << note;
  return ret.str();
}
