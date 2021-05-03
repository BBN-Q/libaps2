/*
 * constants.h
 *
 *	Created on: Jul 3, 2012
 *	Author: cryan
 */

#include <vector>
using std::vector;
#include <chrono>
#include <cstdlib> //size_t

#include <plog/Log.h>

#ifndef CONSTANTS_H_
#define CONSTANTS_H_

// Some maximum sizes of things we can fit
const int MAX_APS_CHANNELS = 2;

const int APS_WAVEFORM_UNIT_LENGTH = 4;

const size_t MAX_WF_LENGTH =
    1 << 26; // only use 256MB for now - 256MB / 2 channels / 2 bytes per sample
const int MAX_WF_AMP = (1 << 13) - 1; // 14 bit signed DAC
const int WF_MODULUS = 4;
const size_t MAX_LL_LENGTH = (1 << 24);
const unsigned CORRECTION_MATRIX_SCALING = (1 << 13); //Q2.13

const std::chrono::seconds COMMS_TIMEOUT = std::chrono::seconds(3);

const int MAX_DAC_CLOCK_PHASE_TEST_TRIES = 20;

// Chip config SPI commands for setting up DAC,PLL,VXCO
// Possible target bytes
// 0x00 ............Pause commands stream for 100ns times the count in D<23:0>
// 0xC0/0xC8 .......DAC Channel 0 Access (AD9736)
// 0xC1/0xC9 .......DAC Channel 1 Access (AD9736)
// 0xD0/0xD8 .......PLL Clock Generator Access (AD518-1)
// 0xE0 ............VCXO Controller Access (CDC7005)
// 0xFF ............End of list
union APSChipConfigCommand_t {
  struct {
    uint32_t instr : 16;      // SPI instruction for DAC, PLL instruction, or 0
    uint32_t spicnt_data : 8; // data byte for single byte or SPI insruction
    uint32_t target : 8;
  };
  uint32_t packed;
  // TODO: sort out the default initialization and whether this is necessary
  APSChipConfigCommand_t() { this->packed = 0; };
};

// PLL commands
// INSTR<12..0> ......ADDR. Specifies the address of the register to read or
// write.
// INSTR<14..13> .....W<1..0>. Specified transfer length. 00 = 1, 01 = 2, 10 =
// 3, 11 = stream
// INSTR<15> .........R/W. Read/Write select. Read = 1, Write = 0.
union PLLCommand_t {
  struct {
    uint16_t addr : 13;
    uint16_t W : 2;
    uint16_t r_w : 1;
  };
  uint16_t packed;
  // TODO: sort out the default initialization and whether this is necessary
  PLLCommand_t() { this->packed = 0; };
};

// DAC Commands
// INSTR<4..0> ......ADDR. Specifies the address of the register to read or
// write.
// INSTR<6..5> ......N<1..0>. Specified transfer length. Only 00 = single byte
// mode supported.
// INSTR<7> ..........R/W. Read/Write select. Read = 1, Write = 0.
union DACCommand_t {
  struct {
    uint8_t addr : 5;
    uint8_t N : 2;
    uint8_t r_w : 1;
  };
  uint8_t packed;
  // TODO: sort out the default initialization and whether this is necessary
  DACCommand_t() { this->packed = 0; };
};

const uint16_t NUM_STATUS_REGISTERS = 16;

enum class APS_COMMANDS : uint32_t {
  RESET = 0x0,
  USERIO_ACK = 0x1,
  USERIO_NACK = 0x9,
  EPROMIO = 0x2,
  CHIPCONFIGIO = 0x3,
  RUNCHIPCONFIG = 0x4,
  FPGACONFIG_ACK = 0x5,
  FPGACONFIG_NACK = 0xD,
  FPGACONFIG_CTRL = 0x6,
  STATUS = 0x7
};

// Helper function to decide if aps command needs address
inline bool needs_address(APS_COMMANDS cmd) {
  switch (cmd) {
  case APS_COMMANDS::RESET:
  case APS_COMMANDS::STATUS:
    return false;
  default:
    return true;
  }
}

enum APS_STATUS {
  APS_STATUS_HOST = 0,
  APS_STATUS_VOLT_A = 1,
  APS_STATUS_VOLT_B = 2,
  APS_STATUS_TEMP = 3
};

enum APS_ERROR_CODES {
  APS_SUCCESS = 0,
  APS_INVALID_CNT = 1,
};

enum USERIO_MODE_STAT {
  USERIO_SUCCESS = APS_SUCCESS,
  USERIO_INVALID_CNT = APS_INVALID_CNT,
  USERIO_USER_LOGIC_TIMEOUT = 2,
  USERIO_RESERVED = 3,
};

enum EPROMIO_MODE_STAT {
  EPROM_RW = 0,
  EPROM_ERASE = 1,
  EPROM_SUCCESS = 0,
  EPROM_INVALID_CNT = 1,
  EPROM_OPERATION_FAILED = 4
};

enum CHIPCONFIGIO_MODE_STAT {
  CHIPCONFIG_SUCCESS = APS_SUCCESS,
  CHIPCONFIG_INVALID_CNT = APS_INVALID_CNT,
  CHIPCONFIG_INVALID_TARGET = 2,
};

enum CHIPCONFIG_IO_TARGET {
  CHIPCONFIG_TARGET_PAUSE = 0,
  CHIPCONFIG_TARGET_DAC_0 = 1,
  CHIPCONFIG_TARGET_DAC_1 = 2,
  CHIPCONFIG_TARGET_PLL = 3,
  CHIPCONFIG_TARGET_VCXO = 4
};

enum CHIPCONFIG_IO_TARGET_CMD {
  CHIPCONFIG_IO_TARGET_PAUSE = 0,
  CHIPCONFIG_IO_TARGET_DAC_0 = 0xC0,        // multiple byte length in SPI cnt
  CHIPCONFIG_IO_TARGET_DAC_1 = 0xC1,        // multiple byte length in SPI cnt
  CHIPCONFIG_IO_TARGET_PLL = 0xD0,          // multiple byte length in SPI cnt
  CHIPCONFIG_IO_TARGET_DAC_0_SINGLE = 0xC8, // single byte payload
  CHIPCONFIG_IO_TARGET_DAC_1_SINGLE = 0xC9, // single byte payload
  CHIPCONFIG_IO_TARGET_PLL_SINGLE = 0xD8,   // single byte payload
  CHIPCONFIG_IO_TARGET_VCXO = 0xE0,
  CHIPCONFIG_IO_TARGET_EOL = 0xFF, // end of list
};

enum RUNCHIPCONFIG_MODE_STAT {
  RUNCHIPCONFIG_SUCCESS = APS_SUCCESS,
  RUNCHIPCONFIG_INVALID_CNT = APS_INVALID_CNT,
  RUNCHIPCONFIG_INVALID_OFFSET = 2,
};

enum FPGACONFIG_MODE_STAT {
  FPGACONFIG_SUCCESS = APS_SUCCESS,
  FPGACONFIG_INVALID_CNT = APS_INVALID_CNT,
  FPGACONFIG_INVALID_OFFSET = 2,
};

enum class STATUS_REGISTERS {
  HOST_FIRMWARE_VERSION = 0,
  USER_FIRMWARE_VERSION = 1,
  CONFIGURATION_SOURCE = 2,
  USER_STATUS = 3,
  DAC0_STATUS = 4,
  DAC1_STATUS = 5,
  PLL_STATUS = 6,
  VCXO_STATUS = 7,
  SEND_PACKET_COUNT = 8,
  RECEIVE_PACKET_COUNT = 9,
  SEQUENCE_SKIP_COUNT = 0xA,
  SEQUENCE_DUP_COUNT = 0xB,
  FCS_OVERRUN_COUNT = 0xC,
  PACKET_OVERRUN_COUNT = 0xD,
  UPTIME_SECONDS = 0xE,
  UPTIME_NANOSECONDS = 0xF,
};

typedef union {
  struct {
    uint32_t hostFirmwareVersion;
    uint32_t userFirmwareVersion;
    uint32_t configurationSource;
    uint32_t userStatus;
    uint32_t dac0Status;
    uint32_t dac1Status;
    uint32_t pllStatus;
    uint32_t vcxoStatus;
    uint32_t sendPacketCount;
    uint32_t receivePacketCount;
    uint32_t sequenceSkipCount;
    uint32_t sequenceDupCount;
    uint32_t fcsOverrunCount;
    uint32_t packetOverrunCount;
    uint32_t uptimeSeconds;
    uint32_t uptimeNanoSeconds;
  };
  uint32_t array[16];
} APSStatusBank_t;

enum CONFIGURATION_SOURCE {
  BASELINE_IMAGE = 0xBBBBBBBB,
  USER_EPROM_IMAGE = 0xEEEEEEEE
};

// APS2 registers
const uint32_t CSR_AXI_OFFSET = 0x44A00000u;
const uint32_t PLL_STATUS_ADDR = CSR_AXI_OFFSET + 0 * 4;
const uint32_t PHASE_COUNT_A_ADDR = CSR_AXI_OFFSET + 1 * 4;
const uint32_t PHASE_COUNT_B_ADDR = CSR_AXI_OFFSET + 2 * 4;
const uint32_t CACHE_STATUS_ADDR = CSR_AXI_OFFSET + 3 * 4;
const uint32_t CACHE_CONTROL_ADDR = CSR_AXI_OFFSET + 4 * 4;
const uint32_t WFA_OFFSET_ADDR = CSR_AXI_OFFSET + 5 * 4;
const uint32_t WFB_OFFSET_ADDR = CSR_AXI_OFFSET + 6 * 4;
const uint32_t SEQ_OFFSET_ADDR = CSR_AXI_OFFSET + 7 * 4;
const uint32_t RESETS_ADDR = CSR_AXI_OFFSET + 8 * 4;
const uint32_t CONTROL_REG_ADDR = CSR_AXI_OFFSET + 9 * 4;
const uint32_t CHANNEL_OFFSET_ADDR = CSR_AXI_OFFSET + 10 * 4;
const uint32_t TRIGGER_WORD_ADDR = CSR_AXI_OFFSET + 11 * 4;
const uint32_t TRIGGER_INTERVAL_ADDR = CSR_AXI_OFFSET + 12 * 4;
const uint32_t DAC_BIST_CHA_PH1_ADDR = CSR_AXI_OFFSET + 13 * 4;
const uint32_t DAC_BIST_CHA_PH2_ADDR = CSR_AXI_OFFSET + 14 * 4;
const uint32_t DAC_BIST_CHB_PH1_ADDR = CSR_AXI_OFFSET + 15 * 4;
const uint32_t DAC_BIST_CHB_PH2_ADDR = CSR_AXI_OFFSET + 16 * 4;
const uint32_t DMA_STATUS_ADDR = CSR_AXI_OFFSET + 17 * 4;
const uint32_t SATA_STATUS_ADDR = CSR_AXI_OFFSET + 18 * 4;
const uint32_t INIT_STATUS_ADDR = CSR_AXI_OFFSET + 19 * 4;
const uint32_t UPTIME_SECONDS_ADDR = CSR_AXI_OFFSET + 20 * 4;
const uint32_t UPTIME_NANOSECONDS_ADDR = CSR_AXI_OFFSET + 21 * 4;
const uint32_t FIRMWARE_VERSION_ADDR = CSR_AXI_OFFSET + 22 * 4;
const uint32_t TEMPERATURE_ADDR = CSR_AXI_OFFSET + 23 * 4;
const uint32_t FIRMWARE_GIT_SHA1_ADDR = CSR_AXI_OFFSET + 24 * 4;
const uint32_t FIRMWARE_BUILD_TIMESTAMP_ADDR = CSR_AXI_OFFSET + 25 * 4;
const uint32_t CORRECTION_MATRIX_ROW0_ADDR = CSR_AXI_OFFSET + 26 * 4;
const uint32_t CORRECTION_MATRIX_ROW1_ADDR = CSR_AXI_OFFSET + 27 * 4;
const uint32_t CH_A_SCALE_ADDR = CSR_AXI_OFFSET + 28 * 4;
const uint32_t CH_B_SCALE_ADDR = CSR_AXI_OFFSET + 29 * 4;
const uint32_t MIXER_AMP_IMBALANCE_ADDR = CSR_AXI_OFFSET + 30 * 4;
const uint32_t MIXER_PHASE_SKEW_ADDR = CSR_AXI_OFFSET + 31 * 4;
const uint32_t CH_A_WF_LENGTH_ADDR = CSR_AXI_OFFSET + 32 * 4;
const uint32_t CH_B_WF_LENGTH_ADDR = CSR_AXI_OFFSET + 33 * 4;
const uint32_t WF_SSB_FREQ_ADDR = CSR_AXI_OFFSET + 34 * 4;
const uint32_t BITSLIP_A_ADDR = CSR_AXI_OFFSET + 35 * 4;
const uint32_t BITSLIP_B_ADDR = CSR_AXI_OFFSET + 36 * 4;

// TDM specific registers
// .. none yet

// APS2 memory map
const uint32_t MEMORY_ADDR = 0x00000000u;
const uint32_t WFA_OFFSET = 0;
const uint32_t WFB_OFFSET = 0x10000000u;
const uint32_t SEQ_OFFSET = 0x20000000u;

// sequencer control bits
const unsigned SM_ENABLE_BIT = 0; // state machine enable
const unsigned TRIGSRC_BIT =
    1; // trigger source (0 = external, 1 = internal, 2 = software)
const unsigned SOFT_TRIG_BIT = 3;
const unsigned TRIGGER_ENABLE_BIT = 4;

// cache control bits
const unsigned CACHE_ENABLE_BIT = 0;

// PLL bits
const unsigned PLL_CHA_RST_BIT = 8;
const unsigned PLL_CHB_RST_BIT = 9;
const unsigned IO_CHA_RST_BIT = 10;
const unsigned IO_CHB_RST_BIT = 11;
const unsigned DAC_BIST_CHA_RST_BIT = 12;
const unsigned DAC_BIST_CHB_RST_BIT = 13;

// HOST_STATUS bits
const int APS2_HOST_TYPE_BIT = 24;

// USER_STATUS bits
const unsigned MMCM_SYS_LOCK_BIT = 31;
const unsigned MMCM_CFG_LOCK_BIT = 30;
const unsigned MIG_C0_LOCK_BIT = 24;
const unsigned MIG_C0_CAL_BIT = 25;
const unsigned MIG_C1_LOCK_BIT = 26;
const unsigned MIG_C1_CAL_BIT = 27;
const unsigned AXI_RESET_BIT = 21;
const unsigned AXI_RESETN_BIT = 20;

// TDM reset control bits
const unsigned TDM_TRIGGER_RESET_BIT = 0;

// DAC SPI Addresses
const uint8_t DAC_SYNC_ADDR = 0x0;
const uint8_t DAC_INTERRUPT_ADDR = 0x1; // LVDS[7] SYNC[6]
const uint8_t DAC_MSDMHD_ADDR = 0x4;    // MSD[7:4] MHD[3:0]
const uint8_t DAC_SD_ADDR = 0x5;        // SD[7:4] CHECK[0]
const uint8_t DAC_CONTROLLER_ADDR =
    0x6; // LSURV[7] LAUTO[6] LFLT[5:2] LTRH[1:0]
const uint8_t DAC_FIFOSTAT_ADDR =
    0x7; // FIFOSTAT[7] FIFOPHASE[6:4] VALID[3] CHANGE[2] OFFSET[1:0]
const uint8_t DAC_CONTROLLERCLOCK_ADDR = 0x16;
const uint8_t DAC_FSC_1_ADDR = 0x02;
const uint8_t DAC_FSC_2_ADDR = 0x03;

// EPROM MEMORY MAP
const uint32_t EPROM_SPI_CONFIG_ADDR = 0x0;
const uint32_t EPROM_USER_IMAGE_ADDR = 0x00010000;
const uint32_t EPROM_BASE_IMAGE_ADDR = 0x01000000;
const uint32_t EPROM_MACIP_ADDR = 0x00FF0000;
const uint32_t EPROM_IP_OFFSET = 8;
const uint32_t EPROM_DHCP_OFFSET = 12;

// APS ethernet type
const uint16_t APS_PROTO = 0xBB4E;

const uint16_t UDP_PORT_OLD = 0xbb4e;
const uint16_t UDP_PORT = 0xbb4f;
const uint16_t TCP_PORT = 0xbb4e;

// PLL/DAC SPI routines go through sets of address/data pairs
typedef std::pair<uint16_t, uint8_t> SPI_AddrData_t;

// Startup sequences

// PLL setup sequence (modified for 600 MHz FPGA sys_clk and 1.2 GHz DACs)
const vector<SPI_AddrData_t> PLL_INIT = {
    {0x0, 0x99},  // Use SDO, Long instruction mode
    {0x10, 0x7C}, // Enable PLL, set charge pump to 4.8ma
    {0x11, 0x5},  // Set reference divider R to 5 to divide 125 MHz reference to 25 MHz
    {0x14, 0x6},  // Set B counter to 6
    {0x16, 0x5},  // Set P prescaler to 16 and enable B counter (N = P*B = 96 to
                  // divide 2400 MHz to 25 MHz)
    {0x17, 0xAD}, // Status of VCO frequency; active high on STATUS bit in
                  // Status/Control register; antibacklash pulse to 1.3ns
    {0x18, 0x74}, // Calibrate VCO with 8 divider, set lock detect count to 255,
                  // set low range
    {0x1A, 0x2D}, // Selects readback of PLL Lock status on LOCK bit in
                  // Status/Control register
    {0x1B, 0xA5}, // Enable VCO & REF1 monitors; REFMON pin control set to
                  // "Status of selected reference (status of differential
                  // reference); active high"
    {0x1C, 0x7},  // Enable differential reference, enable REF1/REF2 power,
                  // disable reference switching
    {0xF0, 0x00}, // Enable un-inverted 400mV clock on OUT0 (goes to DACA)
    {0xF1, 0x00}, // Enable un-inverted 400mV clock on OUT1 (goes to DACB)
    {0xF2, 0x02}, // Disable OUT2
    {0xF3, 0x00}, // Enable un-inverted 400mV clock on OUT3 (goes to FPGA sys_clk)
    {0xF4, 0x02}, // Disable OUT4
    {0xF5, 0x00}, // Enable un-inverted 400mV clock on OUT5 (goes to FPGA mem_clk)
    {0x190, 0x00}, // channel 0: no division
    {0x191, 0x80}, // Bypass 0 divider
    // {0x193, 0x11}, // channel 1: (2 high, 2 low = 1.2 GHz / 4 = 300 MHz sys_clk)
    {0x193, 0x00}, // channel 1: (1 high, 1 low = 1.2 GHz / 2 = 600 MHz sys_clk)
    {0x196, 0x10}, // channel 2: (2 high, 1 low = 1.2 GHz / 3 = 400 MHz mem_clk)
    {0x1E0, 0x0},  // Set VCO post divide to 2
    {0x1E1, 0x2},  // Select VCO as clock source for VCO divider
    {0x232, 0x1},  // Set bit 0 to simultaneously update all registers with
                   // pending writes.
    {0x18, 0x75},  // Initiate Calibration.	Must be followed by Update
                   // Registers Command
    {0x232, 0x1}   // Update registers with pending writes.
};

const vector<SPI_AddrData_t> PLL_SET_CAL_FLAG = {
    {0x18, 0x75}, // Initiate Calibration.	Must be followed by Update
                  // Registers Command
    {0x232, 0x1}  // Set bit 0 to 1 to simultaneously update all registers with
                  // pending writes.
};

const vector<SPI_AddrData_t> PLL_CLEAR_CAL_FLAG = {
    {0x18, 0x74}, // Initiate Calibration.	Must be followed by Update
                  // Registers Command
    {0x232, 0x1}  // Set bit 0 to 1 to simultaneously update all registers with
                  // pending writes.
};

// VCXO setup sequence
const vector<uint8_t> VCXO_INIT = {0x8, 0x60, 0x0, 0x4, 0x64, 0x91, 0x0, 0x61};

// "waveform mode" sequence
const vector<uint64_t> WF_SEQ_TRIG = {
    0xa100610000000000L, // set NCO 0 frequency
    0xa100210000000000L, // reset phase
    0x2100400000000000L, // WAIT for trig
    // insert waveform instructions here
    0x6000000000000001L //  GOTO 1 to reset phase and wait for trigger again
};

const vector<uint64_t> WF_SEQ_CW = {
    0xa100610000000000L, // set NCO 0 frequency
    0x9100800000000000L, // WAIT for sync to implement NCO frequency update
    // insert waveform instructions here
    0x6000000000000002L //  GOTO 2 for continuous WF output
};

#endif /* CONSTANTS_H_ */
