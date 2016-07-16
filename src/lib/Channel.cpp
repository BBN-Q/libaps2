/*
 * Channel.cpp
 *
 *	Created on: Jun 13, 2012
 *			Author: cryan
 */

#include "Channel.h"

Channel::Channel() : number{-1}, enabled_{true}, waveform_(0), trigDelay_{0} {}

Channel::Channel(int number)
    : number{number}, enabled_{true}, waveform_(0), trigDelay_{0} {}

Channel::~Channel() {
  // TODO Auto-generated destructor stub
}

int Channel::set_enabled(const bool &enable) {
  enabled_ = enable;
  return 0;
}

bool Channel::get_enabled() const { return enabled_; }

size_t Channel::get_length() const { return waveform_.size(); }

void Channel::set_waveform(const vector<float> &data) {
  // Check whether we need to resize the waveform vector
  if (data.size() > size_t(MAX_WF_LENGTH)) {
    FILE_LOG(logINFO)
        << "Warning: waveform too large to fit into memory. Waveform length: "
        << data.size();
  }

  // Copy over the waveform data
  // Waveform length must be a integer multiple of WF_MODULUS so resize to that
  waveform_.resize(size_t(WF_MODULUS * ceil(float(data.size()) / WF_MODULUS)),
                   0);
  markers_.resize(waveform_.size());
  std::copy(data.begin(), data.end(), waveform_.begin());
}

void Channel::set_waveform(const vector<int16_t> &data) {
  if (data.size() > size_t(MAX_WF_LENGTH)) {
    FILE_LOG(logINFO)
        << "Warning: waveform too large to fit into memory. Waveform length: "
        << data.size();
  }

  // Waveform length must be a integer multiple of WF_MODULUS so resize to that
  waveform_.resize(size_t(WF_MODULUS * ceil(float(data.size()) / WF_MODULUS)),
                   0);
  markers_.resize(waveform_.size());

  // Copy over the waveform data and convert to scaled floats
  for (size_t ct = 0; ct < data.size(); ct++) {
    waveform_[ct] = float(data[ct]) / MAX_WF_AMP;
  }
}

int Channel::set_markers(const vector<uint8_t> &data) {
  if (data.size() > markers_.size()) {
    FILE_LOG(logDEBUG) << "Marker data length does not match previously "
                          "uploaded waveform data: "
                       << data.size();
    markers_.resize(size_t(WF_MODULUS * ceil(float(data.size()) / WF_MODULUS)),
                    0);
  }

  std::copy(data.begin(), data.end(), markers_.begin());
  return 0;
}

vector<int16_t> Channel::prep_waveform() const {
  // Apply the scale,offset and covert to integer format
  vector<int16_t> prepVec(waveform_.size());
  for (size_t ct = 0; ct < prepVec.size(); ct++) {
    prepVec[ct] = int16_t(MAX_WF_AMP * waveform_[ct]);
  }

  // Clip to the max and min values allowed
  // Signed integer data is asymmetric: can go to -8192 but only up to 8191.
  if ((prepVec.size() > 0) &&
      (*max_element(prepVec.begin(), prepVec.end()) > MAX_WF_AMP)) {
    FILE_LOG(logWARNING) << "Waveform element too positive. Clipping to max.";
    for (int16_t &tmpVal : prepVec) {
      if (tmpVal > MAX_WF_AMP)
        tmpVal = MAX_WF_AMP;
    }
  }
  if ((prepVec.size() > 0) &&
      (*min_element(prepVec.begin(), prepVec.end()) < -(MAX_WF_AMP + 1))) {
    FILE_LOG(logWARNING) << "Waveform element too negative. Clipping to min.";
    for (int16_t &tmpVal : prepVec) {
      if (tmpVal < -MAX_WF_AMP)
        tmpVal = -MAX_WF_AMP;
    }
  }
  // merge 14-bit DAC data with 2-bit marker data
  for (size_t ct = 0; ct < prepVec.size(); ct++) {
    prepVec[ct] =
        (prepVec[ct] & 0x3FFF) | (static_cast<uint16_t>(markers_[ct]) << 14);
  }
  return prepVec;
}

int Channel::clear_data() {
  waveform_.clear();
  return 0;
}

int Channel::write_state_to_hdf5(H5::H5File &H5StateFile,
                                 const string &rootStr) {

  // write waveform data
  FILE_LOG(logDEBUG) << "Writing Waveform: " << rootStr + "/waveformLib";
  vector2h5array<float>(waveform_, &H5StateFile, rootStr + "/waveformLib",
                        rootStr + "/waveformLib", H5::PredType::NATIVE_FLOAT);

  // add channel state information to root group
  H5::Group tmpGroup = H5StateFile.openGroup(rootStr);

  element2h5attribute<bool>("enabled", enabled_, &tmpGroup,
                            H5::PredType::NATIVE_UINT);
  element2h5attribute<int>("trigDelay", trigDelay_, &tmpGroup,
                           H5::PredType::NATIVE_INT);

  tmpGroup.close();

  // Save the linklist data

  // save number of banks to rootStr + /linkListData attribute "numBanks"
  //	USHORT numBanks;
  //	numBanks = banks_.size();//get number of banks from channel
  //
  //	// set attribute
  //	FILE_LOG(logDEBUG) << "Creating Group: " << rootStr + "/linkListData";
  //	tmpGroup = H5StateFile.createGroup(rootStr + "/linkListData");
  //	element2h5attribute<USHORT>("numBanks",	numBanks,
  //&tmpGroup,H5::PredType::NATIVE_UINT16);
  //	tmpGroup.close();
  //
  //	std::ostringstream tmpStream;
  //	//Now loop over the number of banks found and add the bank
  //	for (USHORT bankct=0; bankct<numBanks; bankct++) {
  //		tmpStream.str("");
  //		tmpStream << rootStr << "/linkListData/bank" << bankct+1 ;
  //		FILE_LOG(logDEBUG) << "Writing State Bank: " << bankct+1 << "
  //from
  // hdf5";
  //		banks_[bankct].write_state_to_hdf5(H5StateFile, tmpStream.str()
  //);
  //	}
  return 0;
}

int Channel::read_state_from_hdf5(H5::H5File &H5StateFile,
                                  const string &rootStr) {
  clear_data();
  // read waveform data
  waveform_ = h5array2vector<float>(&H5StateFile, rootStr + "/waveformLib",
                                    H5::PredType::NATIVE_INT16);

  // load state information
  H5::Group tmpGroup = H5StateFile.openGroup(rootStr);
  enabled_ =
      h5element2element<bool>("enabled", &tmpGroup, H5::PredType::NATIVE_UINT);
  trigDelay_ =
      h5element2element<int>("trigDelay", &tmpGroup, H5::PredType::NATIVE_INT);

  // Load the linklist data
  // TODO
  return 0;
}
