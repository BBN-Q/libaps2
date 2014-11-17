#include <iostream>

#include "headings.h"
#include "libaps2.h"
#include "constants.h"

#include <concol.h>

#include "helpers.h"

using namespace std;


vector<uint64_t> read_seq_file(string fileName) {
  std::ifstream FID (fileName, std::ios::in);
  if (!FID.is_open()){
    throw runtime_error("Unable to open file.");
  }
  vector<uint64_t> data;
  uint64_t instruction;
  string line;
  while (FID >> line) {
    stringstream(line) >> std::hex >> instruction;
    data.push_back(instruction);
  }
  FID.close();
  return data;
}


int main (int argc, char* argv[])
{

  concol::concolinit();
  cout << concol::RED << "BBN AP2 Test Executable" << concol::RESET << endl;


  //Logging level
  TLogLevel logLevel = logDEBUG1;
  // if (options[LOG_LEVEL]) {
  //   logLevel = TLogLevel(atoi(options[LOG_LEVEL].arg));
  // }
  set_logging_level(logLevel);
  set_log("stdout");

  string deviceSerial = get_device_id();

  connect_APS(deviceSerial.c_str());

  double uptime;
  get_uptime(deviceSerial.c_str(), &uptime);

  cout << concol::RED << "Uptime for device " << deviceSerial << " is " << uptime << " seconds" << concol::RESET << endl;

  // force initialize device
  init_APS(deviceSerial.c_str(), 1);

  // check that memory map was written
  uint32_t testInt;
  read_memory(deviceSerial.c_str(), WFA_OFFSET_ADDR, &testInt, 1);
  cout << "wfA offset: " << hexn<8> << testInt - MEMORY_ADDR << endl;
  read_memory(deviceSerial.c_str(), WFB_OFFSET_ADDR, &testInt, 1);
  cout << "wfB offset: " << hexn<8> << testInt - MEMORY_ADDR << endl;
  read_memory(deviceSerial.c_str(), SEQ_OFFSET_ADDR, &testInt, 1);
  cout << "seq offset: " << hexn<8> << testInt - MEMORY_ADDR << endl;

  // stop pulse sequencer and cache controller
  stop(deviceSerial.c_str());
  testInt = 0;
  write_memory(deviceSerial.c_str(), CACHE_CONTROL_ADDR, &testInt, 1);

  read_memory(deviceSerial.c_str(), CACHE_CONTROL_ADDR, &testInt, 1);
  cout << "Initial cache control reg: " << hexn<8> << testInt << endl;
  read_memory(deviceSerial.c_str(), CACHE_STATUS_ADDR, &testInt, 1);
  cout << "Initial cache status reg: " << hexn<8> << testInt << endl;
  read_memory(deviceSerial.c_str(), PLL_STATUS_ADDR, &testInt, 1);
  cout << "Initial DMA status reg: " << hexn<8> << testInt << endl;

  // upload test waveforms to A and B

  // square waveforms of increasing amplitude
  // vector<short> wfmA;
  // vector<uint16_t> wfmA;
  // for (short a = 0; a < 8; a++) {
  //   for (int ct=0; ct < 32; ct++) {
  //     wfmA.push_back(a*1000);
  //   }
  // }
  // ramp
  // for (short ct=0; ct < 1024; ct++) {
  //   wfmA.push_back(8*ct);
  // }

  // cout << concol::RED << "Uploading square waveforms to Ch A" << concol::RESET << endl;
  // set_waveform_int(deviceSerial.c_str(), 0, wfmA.data(), wfmA.size());

  // read_memory(deviceSerial.c_str(), CACHE_STATUS_ADDR, &testInt, 1);
  // cout << "Cache status reg after wfA write: " << hexn<8> << testInt << endl;
  // read_memory(deviceSerial.c_str(), PLL_STATUS_ADDR, &testInt, 1);
  // cout << "DMA status reg after wfA write: " << hexn<8> << testInt << endl;
  
  // ramp waveform
  // vector<short> wfmB;
  // for (short ct=0; ct < 1024; ct++) {
  //   wfmB.push_back(8*ct);
  // }
  // cout << concol::RED << "Uploading ramp waveform to Ch B" << concol::RESET << endl;
  // set_waveform_int(deviceSerial.c_str(), 1, wfmB.data(), wfmB.size());

  // read_memory(deviceSerial.c_str(), CACHE_STATUS_ADDR, &testInt, 1);
  // cout << "Cache status reg after wfB write: " << hexn<8> << testInt << endl;
  // read_memory(deviceSerial.c_str(), PLL_STATUS_ADDR, &testInt, 1);
  // cout << "DMA status reg after wfB write: " << hexn<8> << testInt << endl;

  // // check that cache controller was enabled
  // read_memory(deviceSerial.c_str(), CACHE_CONTROL_ADDR, &testInt, 1);
  // cout << "Cache control reg: " << hexn<8> << testInt << endl;

  // // this data should appear in the cache a few microseconds later... read back the cache data??

  // // uint32_t offset = 0xC6000000;
  // uint32_t offset;

  // // test wfA cache
  // size_t numRight = 0;
  // offset = 0xC4000000u;
  // for (size_t ct = 0; ct < 64; ct++)
  // {
  //   read_memory(deviceSerial.c_str(), offset + 4*ct, &testInt, 1);
  //   // cout << hexn<8> << testInt << endl;
  //   if ( testInt != ((static_cast<uint32_t>(wfmA[ct*2]) << 16) | (static_cast<uint32_t>(wfmA[ct*2+1])) ) ) {
  //     cout << concol::RED << "Failed read test at offset " << ct << concol::RESET << endl;
  //   }
  //   else{
  //     numRight++;
  //   }
  // }
  // cout << concol::RED << "Waveform A single word write/read " << 100*static_cast<double>(numRight)/64 << "% correct" << concol::RESET << endl;;
  
  // // test wfB cache
  // offset = 0xC6000000u;
  // numRight = 0;
  // for (size_t ct = 0; ct < 64; ct++)
  // {
  //   read_memory(deviceSerial.c_str(), offset + 4*ct, &testInt, 1);
  //   if ( testInt != ((static_cast<uint32_t>(wfmB[ct*2] << 16)) | static_cast<uint32_t>(wfmB[ct*2+1])) ) {
  //     cout << concol::RED << "Failed read test at offset " << ct << concol::RESET << endl;
  //   }
  //   else{
  //     numRight++;
  //   }
  // }
  // cout << concol::RED << "Waveform B single word write/read " << 100*static_cast<double>(numRight)/64 << "% correct" << concol::RESET << endl;;
  
  // cout << concol::RED << "Writing sequence data" << concol::RESET << endl;

  // vector<uint64_t> seq = read_seq_file("../examples/simpleSeq.dat");
  // write_sequence(deviceSerial.c_str(), seq.data(), seq.size());

  // read_memory(deviceSerial.c_str(), CACHE_STATUS_ADDR, &testInt, 1);
  // cout << "Cache status reg after seq write: " << hexn<8> << testInt << endl;
  // read_memory(deviceSerial.c_str(), PLL_STATUS_ADDR, &testInt, 1);
  // cout << "DMA status reg after seq write: " << hexn<8> << testInt << endl;

  // // test sequence cache
  // offset = 0xC2000000u;
  // numRight = 0;
  // uint32_t testInt2 = 0;
  // for (size_t ct = 0; ct < seq.size(); ct++)
  // {
  //   // put together the 64-bit word from two 32-bit pieces
  //   read_memory(deviceSerial.c_str(), offset + 4 * (2*ct), &testInt, 1);
  //   read_memory(deviceSerial.c_str(), offset + 4 * (2*ct + 1), &testInt2, 1);
  //   if ( ((static_cast<uint64_t>(testInt2) << 32) | testInt) != seq[ct] ) {
  //     cout << concol::RED << "Failed read test at offset " << ct << concol::RESET << endl;
  //   }
  //   else{
  //     numRight++;
  //   }
  // }
  // cout << concol::RED << "Sequence single word write/read " << 100*static_cast<double>(numRight)/seq.size() << "% correct" << concol::RESET << endl;;

  // cout << concol::RED << "Set internal trigger interval to 100us" << concol::RESET << endl;

  set_trigger_source(deviceSerial.c_str(), INTERNAL);
  set_trigger_interval(deviceSerial.c_str(), 10e-3);

  // cout << concol::RED << "Starting" << concol::RESET << endl;

  // run(deviceSerial.c_str());

  // std::this_thread::sleep_for(std::chrono::seconds(1));

  // read_memory(deviceSerial.c_str(), CACHE_STATUS_ADDR, &testInt, 1);
  // cout << "Cache status reg after pulse sequencer enable: " << hexn<8> << testInt << endl;
  // read_memory(deviceSerial.c_str(), PLL_STATUS_ADDR, &testInt, 1);
  // cout << "DMA status reg after pulse sequencer enable: " << hexn<8> << testInt << endl;

  // cout << concol::RED << "Stopping" << concol::RESET << endl;
  // stop(deviceSerial.c_str());

  cout << concol::RED << "Loading Ramsey sequence" << concol::RESET << endl;
  load_sequence_file(deviceSerial.c_str(), "../examples/bigrabi.h5");
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  read_memory(deviceSerial.c_str(), CACHE_STATUS_ADDR, &testInt, 1);
  cout << "Cache status reg: " << hexn<8> << testInt << endl;
  read_memory(deviceSerial.c_str(), PLL_STATUS_ADDR, &testInt, 1);
  cout << "DMA status reg: " << hexn<8> << testInt << endl;


  // read back instruction data
  cout << concol::RED << "Deep memory instruction data" << concol::RESET << endl;
  uint32_t offset = 0x20000000u;
  uint32_t dummy;
  for (size_t ct = 0; ct < 33*2; ct++)
  {
    read_memory(deviceSerial.c_str(), offset + 4*ct, &testInt, 1);
    if (ct % 2 == 0) {
      cout << endl << "instruction [" << std::dec << ct/2 << "]: ";
      dummy = testInt;
    } else {
      cout << hexn<8> << testInt << " " << hexn<8> << dummy << " ";
    }
  }
  cout << endl;

 // read back cached instruction data
  cout << concol::RED << "Cached instruction data" << concol::RESET << endl;
  offset = 0xC2000000u;
  for (size_t ct = 0; ct < 33*2; ct++)
  {
    read_memory(deviceSerial.c_str(), offset + 4*ct, &testInt, 1);
    if (ct % 2 == 0) {
      cout << endl << "instruction [" << std::dec << ct/2 << "]: ";
      dummy = testInt;
    }
    else{
      cout << hexn<8> << testInt << " " << hexn<8> << dummy << " ";
    }
  }
  cout << endl;



  cout << concol::RED << "Starting" << concol::RESET << endl;
  run(deviceSerial.c_str());

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  read_memory(deviceSerial.c_str(), CACHE_STATUS_ADDR, &testInt, 1);
  cout << "Cache status reg after pulse sequencer enable: " << hexn<8> << testInt << endl;
  read_memory(deviceSerial.c_str(), PLL_STATUS_ADDR, &testInt, 1);
  cout << "DMA status reg after pulse sequencer enable: " << hexn<8> << testInt << endl;

  for (size_t ct=0; ct < 3; ct++) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    read_memory(deviceSerial.c_str(), CACHE_STATUS_ADDR, &testInt, 1);
    cout << "Cache status reg: " << hexn<8> << testInt << endl;
  }
  read_memory(deviceSerial.c_str(), PLL_STATUS_ADDR, &testInt, 1);
  cout << "DMA status reg: " << hexn<8> << testInt << endl;

  disconnect_APS(deviceSerial.c_str());
  
  cout << concol::RED << "Finished!" << concol::RESET << endl;
 
  return 0;
}
