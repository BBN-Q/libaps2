/*
 * APS.h
 *
 *  Created on: Jun 13, 2012
 *      Author: cryan
 */

#include "headings.h"

#ifndef APS_H_
#define APS_H_

class APS {
public:
	APS();
	APS(int, string);
	~APS();

	int connect();
	int disconnect();

	int init(const string &, const bool &);

	int setup_VCXO() const;
	int setup_PLL() const;
	int setup_DACs() const;

	int program_FPGA(const string &, const UCHAR &, const int &) const;
	int read_bitfile_version(const UCHAR &) const;

	int set_sampleRate(const int &, const int &, const bool &);
	int get_sampleRate(const int & fpga) const;

	template <typename T>
	int set_waveform(const int & dac, const vector<T> & data){
		_channels[dac].set_waveform(data);
		return FPGA::write_waveform(_handle, dac, _channels[dac].prep_waveform(), _checksums);
	}

	int set_LL_mode(const int &, const bool &, const bool &);

	int trigger_FPGA(const int &, const int &) const;
	int disable_FPGA(const int &) const;

	//The owning APSRack needs access to some private members
	friend class APSRack;

private:
	int _deviceID;
	string _deviceSerial;
	FT_HANDLE _handle;
	vector<Channel> _channels;
	vector<CheckSum> _checksums;



};

#endif /* APS_H_ */
