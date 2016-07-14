#include <iostream>
#include <vector>

#include "concol.h"

using std::cout;
using std::cin;
using std::endl;
using std::string;
using std::vector;

vector<string> enumerate_slices() {
	/*
	Helper function to return connected slices.
	*/
	cout << concol::CYAN << "Enumerating slices" << concol::RESET << endl;

	unsigned numDevices = 0;
	get_numDevices(&numDevices);

	cout << concol::CYAN << numDevices << " APS2 slices" << (numDevices > 1 ? "s": "")	<< " found" << concol::RESET << endl;

	if (numDevices < 1)
		return {""};

	const char ** ip_addr_buffers = new const char*[numDevices];
	get_device_IPs(ip_addr_buffers);

	vector<string> ip_addrs;
	for (unsigned ct=0; ct < numDevices; ct++) {
		ip_addrs.push_back(string(ip_addr_buffers[ct]));
		cout << concol::CYAN << "Device " << ct+1 << " IP address: " << ip_addr_buffers[ct] << concol::RESET << endl;
	}

	delete[] ip_addr_buffers;

	return ip_addrs;
}

string get_device_id() {
	/*
	Helper function than enumerates and asks for a single APS2 to talk to.
	*/

	vector<string> ip_addrs = enumerate_slices();
	if ( ip_addrs.size() == 0 ) {
		return "";
	}

	string selected_ip = string(ip_addrs[0]);
	if ( ip_addrs.size() > 1 ) {
		cout << concol::YELLOW << "Choose device ID [1]: " << concol::RESET;
		string input = "";
		getline(cin, input);

		if (input.length() != 0) {
			selected_ip = string(ip_addrs[stoi(input)-1]);
		}
	}

	return selected_ip;
}

vector<string> get_device_ids() {
	/*
	Helper function than enumerates and asks for which (potentially "all") APS to talk to.
	*/
	vector<string> ip_addrs = enumerate_slices();
	if ( ip_addrs.size() == 0 ) {
		return {""};
	}

	vector<string> selected_ips = {string(ip_addrs[0])};
	if (ip_addrs.size() > 1) {
		cout << concol::YELLOW << "Choose device ID (or \"all\") [1]: " << concol::RESET;
		string input = "";
		getline(cin, input);

		if (input.length() != 0) {
			if (input.compare("all") == 0) {
				for (size_t ct = 0; ct < ip_addrs.size(); ct++) {
					selected_ips.push_back(ip_addrs[ct]);
				}
			} else {
				selected_ips = {ip_addrs[stoi(input)-1]};
			}
		}
	}

	return selected_ips;
}

void print_title(const string & title){
	concol::concolinit();
	cout << endl;
	cout << concol::BOLDMAGENTA << title << concol::RESET << endl;
	cout << endl;
}
