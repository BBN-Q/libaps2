/*
 * APS2.h
 *
 * APS2 Specfic Structures and tools
 */

#ifndef APS2_H
#define APS2_H

#include "headings.h"
#include "APSEthernet.h"
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

	void connect(shared_ptr<APSEthernet> &&);
	void disconnect();

	APS2_STATUS init(const bool & = false, const int & bitFileNum = 0);
	void reset(const APS_RESET_MODE_STAT & resetMode = APS_RESET_MODE_STAT::SOFT_RESET);

	void store_image(const string & bitFile, const int & position = 0);
	int select_image(const int &);
	int program_FPGA(const string &);

	int setup_VCXO() const;
	int setup_PLL() const;
	int setup_DACs();
	int run_chip_config(const uint32_t & addr = 0x0);

	APSStatusBank_t read_status_registers();
	uint32_t read_status_register(const STATUS_REGISTERS &);

	uint32_t get_firmware_version();
	double get_uptime();
	double get_fpga_temperature();

	void set_sampleRate(const unsigned int &);
	unsigned int get_sampleRate();

	void set_trigger_source(const TRIGGER_SOURCE &);
	TRIGGER_SOURCE get_trigger_source();
	void set_trigger_interval(const double &);
	double get_trigger_interval();
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

	void set_run_mode(const RUN_MODE &);

	void write_sequence(const vector<uint64_t> &);
	void clear_channel_data();

	void load_sequence_file(const string &);

	void run();
	void stop();
	RUN_STATE get_runState();

	//Whether the APS connection is open
	bool isOpen;

	RUN_STATE runState;

	//Pretty printers
	static string print_status_bank(const APSStatusBank_t & status);
	static string printAPSChipCommand(APSChipConfigCommand_t & command);

	//Memory read/write
	void write_memory(const uint32_t & addr, const vector<uint32_t> & data);
	void write_memory(const uint32_t & addr, const uint32_t & data);
	vector<uint32_t> read_memory(const uint32_t &, const uint32_t &);

	//SPI read/write
	void write_SPI(vector<uint32_t> &);
	uint32_t read_SPI(const CHIPCONFIG_IO_TARGET &, const uint16_t &);

	//Flash read/write
	int write_flash(const uint32_t &, vector<uint32_t> &);
	vector<uint32_t> read_flash(const uint32_t &, const uint32_t &);

	//MAC and IP addresses
	uint64_t get_mac_addr();
	int set_mac_addr(const uint64_t &);
	uint32_t get_ip_addr();
	int set_ip_addr(const uint32_t &);

	// dhcp enable
	bool get_dhcp_enable();
	int set_dhcp_enable(const bool &);

	//CLPD DRAM
	int write_bitfile(const uint32_t &, const string &);
	int load_bitfile(const uint32_t &);

	//Create/restore setup SPI sequence
	int write_SPI_setup();

	// DAC BIST test
	int run_DAC_BIST(const int &, const vector<int16_t> &, vector<uint32_t> &);
	void set_DAC_SD(const int &, const uint8_t &);

private:

	string deviceSerial_;
	vector<Channel> channels_;
	shared_ptr<APSEthernet> ethernetRM_;
	unsigned samplingRate_;
	MACAddr macAddr_;

	//Read/Write commands
	int write_command(const APSCommand_t &, const uint32_t & addr = 0, const bool & checkResponse = true);
	vector<APSEthernetPacket> pack_data(const uint32_t &, const vector<uint32_t> &, const APS_COMMANDS & cmdtype = APS_COMMANDS::USERIO_ACK);
	vector<APSEthernetPacket> read_packets(const size_t &);

	int erase_flash(uint32_t, uint32_t);

	//Single packet query
	vector<APSEthernetPacket> query(const APSCommand_t &, const uint32_t & addr = 0);
	vector<APSEthernetPacket> query(const APSEthernetPacket &);

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
	int write_state_to_hdf5(  H5::H5File & , const string & );
	int read_state_from_hdf5( H5::H5File & , const string & );

	//Non-exported functions
	shared_ptr<APSEthernet> get_interface();

	int write_macip_flash(const uint64_t &, const uint32_t &, const bool &);
	
}; //end class APS2




#endif /* APS2_H_ */
