/*
 * APS2.h
 *
 * APS2 Specfic Structures and tools
 */

#ifndef APS2_H
#define APS2_H

#include <memory>
using std::shared_ptr;

#include "APS2Ethernet.h"
#include "Channel.h"
#include "APS2_errno.h"
#include "APS2_enums.h"

class APS2 {

public:

	static const int NUM_CHANNELS = 2;

	//Constructors
	APS2();
	APS2(string);
	~APS2();

	void connect(shared_ptr<APS2Ethernet> &&);
	void disconnect();

	APS2_STATUS init(const bool & = false, const int & bitFileNum = 0);
	void reset(APS2_RESET_MODE);

	int setup_VCXO() const;
	int setup_PLL() const;
	int setup_DACs();
	void run_chip_config(uint32_t addr = 0x0);

	APSStatusBank_t read_status_registers();
	uint32_t read_status_register(const STATUS_REGISTERS &);

	uint32_t get_firmware_version();
	uint32_t get_firmware_git_sha1();
	uint32_t get_firmware_build_timestamp();
	double get_uptime();
	float get_fpga_temperature();

	void set_sampleRate(const unsigned int &);
	unsigned int get_sampleRate();

	void set_trigger_source(const APS2_TRIGGER_SOURCE &);
	APS2_TRIGGER_SOURCE get_trigger_source();
	void set_trigger_interval(const float &);
	float get_trigger_interval();
	void trigger();

	void set_channel_enabled(const int &, const bool &);
	bool get_channel_enabled(const int &) const;
	void set_channel_offset(const int &, const float &);
	float get_channel_offset(const int &) const;
	void set_channel_scale(const int &, const float &);
	float get_channel_scale(const int &) const;
	int set_offset_register(const int &, const float &);

	template <typename T>
	void set_waveform(const int & dac, const vector<T> & data){
		channels_[dac].set_waveform(data);
		write_waveform(dac, channels_[dac].prep_waveform());
	}

	void set_markers(const int &, const vector<uint8_t> &);

	void set_run_mode(const APS2_RUN_MODE &);

	void write_sequence(const vector<uint64_t> &);
	void clear_channel_data();

	void load_sequence_file(const string &);

	void run();
	void stop();
	APS2_RUN_STATE get_runState();

	APS2_RUN_STATE runState;
	APS2_HOST_TYPE host_type;
	bool legacy_firmware;

	//Pretty printers
	static string print_status_bank(const APSStatusBank_t & status);
	static string printAPSChipCommand(APSChipConfigCommand_t & command);
	static string print_firmware_version(uint32_t);

	//Memory read/write
	void write_memory(const uint32_t & addr, const vector<uint32_t> & data);
	void write_memory(const uint32_t & addr, const uint32_t & data);
	vector<uint32_t> read_memory(uint32_t, uint32_t);

	//SPI read/write
	void write_SPI(vector<uint32_t> &);
	uint32_t read_SPI(const CHIPCONFIG_IO_TARGET &, const uint16_t &);

	//Configuration SDRAM read/write
	void write_configuration_SDRAM(uint32_t addr, const vector<uint32_t> & data);
	vector<uint32_t> read_configuration_SDRAM(uint32_t, uint32_t);

	//Flash read/write
	void write_flash(uint32_t, vector<uint32_t> &);
	vector<uint32_t> read_flash(uint32_t, uint32_t);
	std::atomic<APS2_FLASH_TASK> flash_task;
	std::atomic<double> flash_task_progress;

	//MAC and IP addresses
	uint64_t get_mac_addr();
	void set_mac_addr(const uint64_t &);
	uint32_t get_ip_addr();
	void set_ip_addr(const uint32_t &);

	// dhcp enable
	bool get_dhcp_enable();
	void set_dhcp_enable(const bool &);

	//Create/restore setup SPI sequence
	void write_SPI_setup();

	//bitfile loading
	void write_bitfile(const string &, uint32_t, APS2_BITFILE_STORAGE_MEDIA);
	void program_bitfile(uint32_t);

	// DAC BIST test
	int run_DAC_BIST(const int &, const vector<int16_t> &, vector<uint32_t> &);
	void set_DAC_SD(const int &, const uint8_t &);



private:

	string ipAddr_;
	bool connected_;
	vector<Channel> channels_;
	shared_ptr<APS2Ethernet> ethernetRM_;
	unsigned samplingRate_;
	MACAddr macAddr_;

	void erase_flash(uint32_t, uint32_t);

	vector<uint32_t> build_DAC_SPI_msg(const CHIPCONFIG_IO_TARGET &, const vector<SPI_AddrData_t> &);
	vector<uint32_t> build_PLL_SPI_msg(const vector<SPI_AddrData_t> &);
	vector<uint32_t> build_VCXO_SPI_msg(const vector<uint8_t> &);

	// PLL methods
	int setup_PLL();
	int set_PLL_freq(const int &);
	int test_PLL_sync();
	void check_clocks_status();
	int get_PLL_freq();
	void enable_DAC_clock(const int &);
	void disable_DAC_clock(const int &);

	// VCXO methods
	void setup_VCXO();

	// DAC methods
	void setup_DAC(const int &);
	void enable_DAC_FIFO(const int &);
	void disable_DAC_FIFO(const int &);

	// int trigger();
	// int disable();

	void set_bit(const uint32_t &, std::initializer_list<int>);
	void clear_bit(const uint32_t &, std::initializer_list<int>);

	void write_waveform(const int &, const vector<int16_t> &);

	int write_memory_map(const uint32_t & wfA = WFA_OFFSET, const uint32_t & wfB = WFB_OFFSET, const uint32_t & seq = SEQ_OFFSET);

	int save_state_file(string &);
	int read_state_file(string &);
	int write_state_to_hdf5(	H5::H5File & , const string & );
	int read_state_from_hdf5( H5::H5File & , const string & );

	//Non-exported functions
	shared_ptr<APS2Ethernet> get_interface();

	void write_macip_flash(const uint64_t &, const uint32_t &, const bool &);

}; //end class APS2



#endif /* APS2_H_ */
