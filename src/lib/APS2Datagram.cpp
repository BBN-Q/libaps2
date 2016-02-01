// Datagrams that are sent/received from the APS2
// Consists of a vector of 32 bit words
// 1. command word - see APSCommand_t
// 2. address
// 3. payload
//
// Original author: Colm Ryan
// Copyright 2016 Raytheon BBN Technologi

#include "APS2Datagram.h"

#include "constants.h"
#include "APS2_errno.h"
#include "helpers.h"

vector<APS2Datagram> APS2Datagram::chunk(APS2Command cmd, uint32_t addr, const vector<uint32_t>& data, uint16_t chunk_size) {
	//Create multiple datagrams of a maximum payload size
	vector<APS2Datagram> chunks;
	auto start_it = data.begin();
	while (start_it != data.end()) {
		auto end_it = (std::distance(start_it, data.end()) > chunk_size) ? start_it + chunk_size : data.end();
		auto cur_chunk_size = std::distance(start_it, end_it);
		cmd.cnt = cur_chunk_size;
		chunks.push_back(APS2Datagram{cmd, addr, vector<uint32_t>(start_it, end_it)});
		std::advance(start_it, cur_chunk_size);
		addr += 4*cur_chunk_size;
	}
	return chunks;
}

vector<uint32_t> APS2Datagram::data() const {
	//Construct the datagram
	vector<uint32_t> data;
	data.reserve(2+payload.size());
	data.push_back(cmd.packed);
	data.push_back(addr);
	std::copy(payload.begin(), payload.end(), std::back_inserter(data));
	return data;
}

void APS2Datagram::check_ack(const APS2Datagram & ack, bool legacy_firmware) const {

	FILE_LOG(logDEBUG2) << "Checking APS2 acknowledge datagram: "	<< hexn<8> << ack.cmd.packed << " " << hexn<8> << ack.addr;

	//axi memory writes mode_stat reports datamover status/tag
	if ( (cmd.r_w == 0) && (APS_COMMANDS(cmd.cmd) == APS_COMMANDS::USERIO_ACK) ) {
		//TODO: generalize to tag
		uint32_t expected_mode_stat = legacy_firmware ? 0x80 : 0x81;
		if ( (ack.cmd.mode_stat != expected_mode_stat) ) {
			FILE_LOG(logERROR) << "APS2 datamover reported error/tag code: " << hexn<2> << ack.cmd.mode_stat;
			throw APS2_COMMS_ERROR;
		}

		//legacy firmware does not report address
		uint32_t expected_addr = legacy_firmware ? 0 : addr;
		if ( (ack.addr != expected_addr) ) {
			FILE_LOG(logERROR) << "acknowledge datagram has unexpected address: expected " << hexn<8> << expected_addr << " got " << ack.addr;
			throw APS2_COMMS_ERROR;
		}
	}

	//configuration SDRAM  writes
	if ( (cmd.r_w == 0) && (APS_COMMANDS(cmd.cmd) == APS_COMMANDS::FPGACONFIG_ACK) ) {
		if ( ack.cmd.mode_stat != 0x00 ) {
			FILE_LOG(logERROR) << "APS2 CPLD reported error mode/stat " << hexn<2> << ack.cmd.mode_stat;
			throw APS2_COMMS_ERROR;
		}
	}

	//EPROM erases and writes
	if ( (cmd.r_w == 0) && (APS_COMMANDS(cmd.cmd) == APS_COMMANDS::EPROMIO) ) {
		if ( ack.cmd.mode_stat != 0x00 ) {
			FILE_LOG(logERROR) << "APS2 CPLD reported error mode/stat " << hexn<2> << ack.cmd.mode_stat;
			if ( cmd.mode_stat == 0x01 ) {
				throw APS2_ERPOM_ERASE_FAILURE;
			}
			throw APS2_COMMS_ERROR;
		}
	}

	//chip config SPI writes
	if ( (cmd.r_w == 0) && (APS_COMMANDS(cmd.cmd) == APS_COMMANDS::CHIPCONFIGIO) ) {
		if ( ack.cmd.mode_stat != 0x00 ) {
			FILE_LOG(logERROR) << "APS2 CPLD reported error mode/stat " << hexn<2> << ack.cmd.mode_stat;
			throw APS2_COMMS_ERROR;
		}
	}


}
