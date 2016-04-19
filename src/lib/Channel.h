/*
 * Channel.h
 *
 *	Created on: Jun 13, 2012
 *	Author: cryan
 */


#ifndef CHANNEL_H_
#define CHANNEL_H_

#include <vector>
using std::vector;
#include <cstddef> //size_t
#include <cstdint> //fixed width integers
#include <string>
using std::string;
#include <math.h> //ceil
#include <algorithm> //max_element
#include "H5Cpp.h"

#include "constants.h"
#include "helpers.h"

class Channel {
public:
	Channel();
	Channel(int);
	~Channel();
	int number;

	int set_enabled(const bool &);
	bool get_enabled() const;
	size_t get_length() const;

	int set_waveform(const vector<float> &);
	int set_waveform(const vector<int16_t> &);
	int set_markers(const vector<uint8_t> &);
	vector<int16_t> prep_waveform() const;

	int clear_data();

	int write_state_to_hdf5( H5::H5File & , const string & );
	int read_state_from_hdf5(H5::H5File & , const string & );

	friend class APS2;
	friend class BankBouncerThread;

private:
	bool enabled_;
	vector<float> waveform_;
	vector<uint8_t> markers_;
	int trigDelay_;
};

#endif /* CHANNEL_H_ */
