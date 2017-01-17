/*
Tests the analog output data integrity in three different domains.

FPGA: before serialization on the FPGA
LVDS: arriving at DAC
SYNC: on output clock on DAC
*/

#include <functional>
#include <iostream>
#include <random>

#include "../C++/helpers.h"
#include "concol.h"
#include "libaps2.h"

using std::cout;
using std::endl;
using std::string;
using std::vector;

void print_results(const vector<uint32_t> &results) {

  auto print_pass_fail = [](const bool &result) {
    if (result) {
      cout << concol::GREEN << "pass" << concol::RESET;
    } else {
      cout << concol::RED << "fail" << concol::RESET;
    }
  };

  cout << "FPGA: ";
  print_pass_fail(results[2] == results[0]);
  cout << " / ";
  print_pass_fail(results[3] == results[1]);

  cout << " LVDS: ";
  print_pass_fail(results[4] == results[2]);
  cout << " / ";
  print_pass_fail(results[5] == results[3]);

  cout << " SYNC: ";
  print_pass_fail(results[6] == results[4]);
  cout << " / ";
  print_pass_fail(results[7] == results[5]);
}

int main(int argc, char const *argv[]) {

  print_title("BBN APS2 Analog Data integrity Test");

  // Poll for which device to test
  string deviceSerial = get_device_id();
  if (deviceSerial.empty()) {
    cout << concol::RED << "No APS2 devices connected! Exiting..."
         << concol::RESET << endl;
    return 0;
  }

  set_logging_level(logDEBUG1);

  connect_APS(deviceSerial.c_str());
  stop(deviceSerial.c_str());

  // initialize device
  init_APS(deviceSerial.c_str(), 0);

  // Generate random bit values
  std::default_random_engine generator;
  std::uniform_int_distribution<int> bitDistribution(0, 1);
  std::uniform_int_distribution<int16_t> wordDistribution(-8192, 8191);
  auto randbit = std::bind(bitDistribution, generator);
  auto randWord = std::bind(wordDistribution, generator);

  vector<int16_t> testVec;
  vector<uint32_t> results(8);

  // Loop through DACs
  for (int dac = 0; dac < 2; ++dac) {
    cout << concol::CYAN << "DAC " << dac << concol::RESET << endl;

    for (int bit = 0; bit < 14; ++bit) {
      cout << "Testing bit " << bit << ": ";
      testVec.clear();
      for (int ct = 0; ct < 1000; ++ct) {
        testVec.push_back((randbit() << bit) * (bit == 13 ? -1 : 1));
      }
      run_DAC_BIST(deviceSerial.c_str(), dac, testVec.data(), testVec.size(),
                   results.data());

      print_results(results);
      cout << endl;
    }
    cout << endl;
    cout << "Testing random waveform... ";
    testVec.clear();
    for (int ct = 0; ct < (1 << 17); ++ct) {
      testVec.push_back(randWord());
    }
    run_DAC_BIST(deviceSerial.c_str(), dac, testVec.data(), testVec.size(),
                 results.data());

    print_results(results);

    cout << endl;
  }

  disconnect_APS(deviceSerial.c_str());

  return 0;
}
