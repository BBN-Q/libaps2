import sys
import os
import numpy as np

#assume this is run from /examples and add ../src/python to path
import sys
import os
examples_path = os.path.abspath(os.path.dirname(__file__))
aps2_module_path = os.path.join(os.path.dirname(examples_path), "src/python")
if aps2_module_path not in sys.path:
    sys.path.append(aps2_module_path)

import aps2

aps = aps2.APS2()
aps.connect(str(sys.argv[1]))

uptime = aps.get_uptime()
firmware_version = aps.get_firmware_version()
print("Connected to APS2 running firmware version {} and up {} seconds".format(firmware_version[3], uptime))

aps.init()
aps.set_trigger_source(aps2.INTERNAL)
aps.set_trigger_interval(20e-3)

print("Playing ramp waveforms")
aps.set_waveform_float(0, np.linspace(-1,1, 2**14, dtype=np.float32))
aps.set_waveform_float(1, np.linspace(1,-1, 2**14, dtype=np.float32))
aps.set_run_mode(aps2.TRIG_WAVEFORM)
aps.run()
input("Press Enter to continue...")
aps.stop()

print("Playing modulated CW waveforms")
aps.set_waveform_float(0, np.ones(1200, dtype=np.float32))
aps.set_waveform_float(1, np.zeros(1200, dtype=np.float32))
aps.set_waveform_frequency(10e6)
aps.set_run_mode(aps2.CW_WAVEFORM)
aps.run()
input("Press Enter to continue...")
aps.stop()

aps.set_run_mode(aps2.RUN_SEQUENCE)
def demo_sequence(display_text, sequence_file):
    print(display_text)
    aps.load_sequence_file(os.path.join(examples_path, sequence_file+".h5"))
    aps.run()
    input("Press Enter to continue...")
    aps.stop()

demo_sequence("Playing Ramsey sequence with slipped markers", "ramsey_slipped")

demo_sequence("Playing Ramsey sequence with TPPI (modulation instructions)", "ramsey_tppi")

demo_sequence('Playing TPPI Ramsey sequence with SSB (modulation instructions)', 'ramsey_tppi_ssb');

demo_sequence("Playing Rabi width sweep (waveform prefetching)", "rabi_width")

demo_sequence("Playing CPMG (repeats and subroutines)", "cpmg")

demo_sequence("Playing flip-flops (instruction prefetching)", "instr_prefetch")

aps.disconnect()
