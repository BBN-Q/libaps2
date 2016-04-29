import numpy as np
import numpy.ctypeslib as npct
from ctypes import c_int, c_uint, c_ulong, c_ulonglong, c_float, c_double, c_char, c_char_p, byref, POINTER

libaps2 = npct.load_library("libaps2", ".")

np_double_1D = npct.ndpointer(dtype=np.double,  ndim=1, flags='CONTIGUOUS')
np_float_1D  = npct.ndpointer(dtype=np.float,   ndim=1, flags='CONTIGUOUS')
np_int16_1D  = npct.ndpointer(dtype=np.int16,   ndim=1, flags='CONTIGUOUS')
np_int8_1D   = npct.ndpointer(dtype=np.int8,    ndim=1, flags='CONTIGUOUS')
np_uint64_1D = npct.ndpointer(dtype=np.uint64,  ndim=1, flags='CONTIGUOUS')
np_uint32_1D = npct.ndpointer(dtype=np.uint32,  ndim=1, flags='CONTIGUOUS')
np_uint16_1D = npct.ndpointer(dtype=np.uint16,  ndim=1, flags='CONTIGUOUS')
np_uint8_1D  = npct.ndpointer(dtype=np.uint8,   ndim=1, flags='CONTIGUOUS')

class Flag(object):
    flags = [(0x1, 'fun'), (0x2, 'toy')]
    @classmethod
    def from_param(cls, data):
        return c_uint(encode_flags(self.flags, data))

# APS2_TRIGGER_SOURCE
EXTERNAL = 0
INTERNAL = 1
SOFTWARE = 2
SYSTEM   = 3

trigger_dict = {
	0: "EXTERNAL",
	1: "INTERNAL",
	2: "SOFTWARE",
	3: "SYSTEM"
}

# APS2_RUN_MODE 
RUN_SEQUENCE  = 0
TRIG_WAVEFORM = 1
CW_WAVEFORM   = 2

run_mode_dict = {
	0: "RUN_SEQUENCE",
	1: "TRIG_WAVEFORM",
	2: "CW_WAVEFORM"
}

# APS2_RUN_STATE
STOPPED = 0
PLAYING = 1

run_state_dict = {
	0: "STOPPED",
	1: "PLAYING",
}

# enum APS2_STATUS {
APS2_OK                          = 0
APS2_UNKNOWN_ERROR               = -1
APS2_NO_DEVICE_FOUND             = -2
APS2_UNCONNECTED                 = -3
APS2_RESET_TIMEOUT               = -4
APS2_FILELOG_ERROR               = -5
APS2_SEQFILE_FAIL                = -6
APS2_PLL_LOST_LOCK               = -7
APS2_MMCM_LOST_LOCK              = -8
APS2_UNKNOWN_RUN_MODE            = -9
APS2_FAILED_TO_CONNECT           = -10
APS2_INVALID_DAC                 = -11
APS2_NO_SUCH_BITFILE             = -12
APS2_MAC_ADDR_VALIDATION_FAILURE = -13
APS2_IP_ADDR_VALIDATION_FAILURE  = -14
APS2_DHCP_VALIDATION_FAILURE     = -15
APS2_RECEIVE_TIMEOUT             = -16
APS2_SOCKET_FAILURE              = -17
APS2_INVALID_IP_ADDR             = -18
APS2_COMMS_ERROR                 = -19
APS2_UNALIGNED_MEMORY_ACCESS     = -20
APS2_ERPOM_ERASE_FAILURE         = -21
APS2_BITFILE_VALIDATION_FAILURE  = -22
APS2_BAD_PLL_VALUE               = -23

status_dict = {
	   0:  "APS2_OK",
	  -1:  "APS2_UNKNOWN_ERROR",
	  -2:  "APS2_NO_DEVICE_FOUND",
	  -3:  "APS2_UNCONNECTED",
	  -4:  "APS2_RESET_TIMEOUT",
	  -5:  "APS2_FILELOG_ERROR",
	  -6:  "APS2_SEQFILE_FAIL",
	  -7:  "APS2_PLL_LOST_LOCK",
	  -8:  "APS2_MMCM_LOST_LOCK",
	  -9:  "APS2_UNKNOWN_RUN_MODE",
	 -10:  "APS2_FAILED_TO_CONNECT",
	 -11:  "APS2_INVALID_DAC",
	 -12:  "APS2_NO_SUCH_BITFILE",
	 -13:  "APS2_MAC_ADDR_VALIDATION_FAILURE",
	 -14:  "APS2_IP_ADDR_VALIDATION_FAILURE",
	 -15:  "APS2_DHCP_VALIDATION_FAILURE",
	 -16:  "APS2_RECEIVE_TIMEOUT",
	 -17:  "APS2_SOCKET_FAILURE",
	 -18:  "APS2_INVALID_IP_ADDR",
	 -19:  "APS2_COMMS_ERROR",
	 -20:  "APS2_UNALIGNED_MEMORY_ACCESS",
	 -21:  "APS2_ERPOM_ERASE_FAILURE",
	 -22:  "APS2_BITFILE_VALIDATION_FAILURE",
	 -23:  "APS2_BAD_PLL_VALUE",
}

def check(status_code):
	if status_code is 0:
		return
	else:
		raise Exception("APS Error: {}".format(status_dict[status_code]))

libaps2.get_error_msg.argtypes = [c_int]
libaps2.get_error_msg.restype  = c_int
def get_error_msg():
	check(libaps2.get_error_msg())

libaps2.get_numDevices.argtypes = [POINTER(c_uint)]
libaps2.get_numDevices.restype  = c_int
def get_numDevices():
	n = c_uint()
	check(libaps2.get_numDevices(byref(n)))
	return n.value

libaps2.get_device_IPs.argtypes = [POINTER(c_char_p)]
libaps2.get_device_IPs.restype  = c_int
def get_device_IPs():
	check(libaps2.get_device_IPs())

libaps2.connect_APS.argtypes    = [c_char_p]
libaps2.connect_APS.restype     = c_int
def connect_APS(ip_address):
	check(libaps2.connect_APS(ip_address.encode('utf-8')))

libaps2.disconnect_APS.argtypes = [c_char_p]
libaps2.disconnect_APS.restype  = c_int
def disconnect_APS(ip_address):
	check(libaps2.disconnect_APS(ip_address.encode('utf-8')))

libaps2.reset.argtypes    = [c_char_p, c_int]
libaps2.reset.restype     = c_int
def reset(ip_address):
	check(libaps2.reset(ip_address.encode('utf-8')))

libaps2.init_APS.argtypes = [c_char_p, c_int]
libaps2.init_APS.restype  = c_int
def init_APS(ip_address):
	check(libaps2.init_APS(c_char_pip_address.encode('utf-8')))

libaps2.get_firmware_version.argtypes = [c_char_p, POINTER(c_ulong), POINTER(c_ulong), POINTER(c_ulong), c_char_p]
libaps2.get_firmware_version.restype  = c_int
def get_firmware_version(ip_address):
	check(libaps2.get_firmware_version(ip_address.encode('utf-8')))

libaps2.get_uptime.argtypes           = [c_char_p, POINTER(c_double)]
libaps2.get_uptime.restype            = c_int
def get_uptime(ip_address):
	time = c_double()
	check(libaps2.get_uptime(ip_address.encode('utf-8'), byref(time)))
	return time.value

libaps2.get_fpga_temperature.argtypes = [c_char_p, POINTER(c_double)]
libaps2.get_fpga_temperature.restype  = c_int
def get_fpga_temperature(ip_address):
	temp = c_double()
	check(libaps2.get_fpga_temperature(ip_address.encode('utf-8'), byref(temp)))
	return temp.value

libaps2.set_sampleRate.argtypes = [c_char_p, c_uint]
libaps2.set_sampleRate.restype  = c_int
def set_sampleRate(ip_address, rate):
	check(libaps2.set_sampleRate(ip_address.encode('utf-8'), rate))

libaps2.get_sampleRate.argtypes = [c_char_p, POINTER(c_uint)]
libaps2.get_sampleRate.restype  = c_int
def get_sampleRate(ip_address):
	sample_rate = c_uint()
	check(libaps2.get_sampleRate(ip_address.encode('utf-8'), byref(sample_rate)))
	return sample_rate.value

libaps2.set_channel_offset.argtypes  = [c_char_p, c_int, c_float]
libaps2.set_channel_offset.restype   = c_int
def set_channel_offset(ip_address, channel, offset):
	check(libaps2.set_channel_offset(ip_address.encode('utf-8'), channel, offset))

libaps2.get_channel_offset.argtypes  = [c_char_p, c_int, POINTER(c_float)]
libaps2.get_channel_offset.restype   = c_int
def get_channel_offset(ip_address, channel):
	offset = c_float()
	check(libaps2.get_channel_offset(ip_address.encode('utf-8'), channel, byref(offset)))
	return offset.value

libaps2.set_channel_scale.argtypes   = [c_char_p, c_int, c_float]
libaps2.set_channel_scale.restype    = c_int
def set_channel_scale(ip_address, channel, scale):
	check(libaps2.set_channel_scale(ip_address.encode('utf-8'), channel, scale))

libaps2.get_channel_scale.argtypes   = [c_char_p, c_int, POINTER(c_float)]
libaps2.get_channel_scale.restype    = c_int
def get_channel_scale(ip_address, channel):
	scale = c_float()
	check(libaps2.get_channel_scale(ip_address.encode('utf-8'), channel, byref(scale)))
	return scale.value

libaps2.set_channel_enabled.argtypes = [c_char_p, c_int, c_int]
libaps2.set_channel_enabled.restype  = c_int
def set_channel_enabled(ip_address, channel, enabled):
	check(libaps2.set_channel_enabled(ip_address.encode('utf-8'), channel, enabled))

libaps2.get_channel_enabled.argtypes = [c_char_p, c_int, POINTER(c_int)]
libaps2.get_channel_enabled.restype  = c_int
def get_channel_enabled(ip_address, channel):
	enabled = c_int()
	check(libaps2.get_channel_enabled(ip_address.encode('utf-8'), channel, byref(enabled)))
	return bool(enabled.value)

libaps2.set_trigger_source.argtypes   = [c_char_p, c_int]
libaps2.set_trigger_source.restype    = c_int
def set_trigger_source(ip_address, source):
	check(libaps2.set_trigger_source(ip_address.encode('utf-8'), source))

libaps2.get_trigger_source.argtypes   = [c_char_p, POINTER(c_int)]
libaps2.get_trigger_source.restype    = c_int
def get_trigger_source(ip_address):
	source = c_int()
	check(libaps2.get_trigger_source(ip_address.encode('utf-8'), byref(source)))
	return trigger_dict[source.value]

libaps2.set_trigger_interval.argtypes = [c_char_p, c_double]
libaps2.set_trigger_interval.restype  = c_int
def set_trigger_interval(ip_address):
	check(libaps2.set_trigger_interval(ip_address.encode('utf-8')))

libaps2.get_trigger_interval.argtypes = [c_char_p, np_double_1D]
libaps2.get_trigger_interval.restype  = c_int
def get_trigger_interval(ip_address):
	check(libaps2.get_trigger_interval(ip_address.encode('utf-8')))

libaps2.trigger.argtypes              = [c_char_p]
libaps2.trigger.restype               = c_int
def trigger(ip_address):
	check(libaps2.trigger(ip_address.encode('utf-8')))

libaps2.set_waveform_float.argtypes = [c_char_p, c_int, np_float_1D, c_int]
libaps2.set_waveform_float.restype  = c_int
def set_waveform_float(ip_address):
	check(libaps2.set_waveform_float(ip_address.encode('utf-8')))

libaps2.set_waveform_int.argtypes   = [c_char_p, c_int, np_int16_1D, c_int]
libaps2.set_waveform_int.restype    = c_int
def set_waveform_int(ip_address):
	check(libaps2.set_waveform_int(ip_address.encode('utf-8')))

libaps2.set_markers.argtypes        = [c_char_p, c_int, np_int8_1D,  c_int]
libaps2.set_markers.restype         = c_int
def set_markers(ip_address):
	check(libaps2.set_markers(ip_address.encode('utf-8')))

libaps2.write_sequence.argtypes = [c_char_p, np_uint64_1D, c_ulong]
libaps2.write_sequence.restype  = c_int
def write_sequence(ip_address):
	check(libaps2.write_sequence(ip_address.encode('utf-8')))

libaps2.set_run_mode.argtypes = [c_char_p, c_int]
libaps2.set_run_mode.restype  = c_int
def set_run_mode(ip_address):
	check(libaps2.set_run_mode(ip_address.encode('utf-8')))

libaps2.load_sequence_file.argtypes = [c_char_p, c_char_p]
libaps2.load_sequence_file.restype  = c_int
def load_sequence_file(ip_address):
	check(libaps2.load_sequence_file(ip_address.encode('utf-8')))

libaps2.clear_channel_data.argtypes = [c_char_p]
libaps2.clear_channel_data.restype  = c_int
def clear_channel_data(ip_address):
	check(libaps2.clear_channel_data(ip_address.encode('utf-8')))

libaps2.run.argtypes          = [c_char_p]
libaps2.run.restype           = c_int
def run(ip_address):
	check(libaps2.run(ip_address.encode('utf-8')))

libaps2.stop.argtypes         = [c_char_p]
libaps2.stop.restype          = c_int
def stop(ip_address):
	check(libaps2.stop(ip_address.encode('utf-8')))

libaps2.get_runState.argtypes = [c_char_p, POINTER(c_int)]
libaps2.get_runState.restype  = c_int
def get_runState(ip_address):
	check(libaps2.get_runState(ip_address.encode('utf-8')))

libaps2.set_log.argtypes           = [c_char_p]
libaps2.set_log.restype            = c_int
def set_log():
	check(libaps2.set_log(ip_address.encode('utf-8')))

libaps2.set_logging_level.argtypes = [c_int]
libaps2.set_logging_level.restype  = c_int
def set_logging_level():
	check(libaps2.set_logging_level(ip_address.encode('utf-8')))

libaps2.get_ip_addr.argtypes = [c_char_p, c_char_p]
libaps2.get_ip_addr.restype  = c_int
def get_ip_addr():
	check(libaps2.get_ip_addr(ip_address.encode('utf-8')))

libaps2.set_ip_addr.argtypes = [c_char_p, c_char_p]
libaps2.set_ip_addr.restype  = c_int
def set_ip_addr():
	check(libaps2.set_ip_addr(ip_address.encode('utf-8')))

libaps2.get_dhcp_enable.argtypes = [c_char_p, POINTER(c_int)]
libaps2.get_dhcp_enable.restype  = c_int
def get_dhcp_enable():
	check(libaps2.get_dhcp_enable(ip_address.encode('utf-8')))

libaps2.set_dhcp_enable.argtypes = [c_char_p, c_int]
libaps2.set_dhcp_enable.restype  = c_int
def set_dhcp_enable():
	check(libaps2.set_dhcp_enable(ip_address.encode('utf-8')))

