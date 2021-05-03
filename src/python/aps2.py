import os
import platform
import warnings
import numpy as np
import numpy.ctypeslib as npct
from ctypes import c_int, c_uint, c_ulong, c_ulonglong, c_float, c_double, c_char, \
                   c_char_p, addressof, create_string_buffer, byref, POINTER, CDLL
from ctypes.util import find_library
from enum import IntEnum
import sys

#ctypes compatible Enum class to set logger severity according to plog values
#https://www.chriskrycho.com/2015/ctypes-structures-and-dll-exports.html

class PlogSeverity(IntEnum):
    none = 0
    fatal = 1
    error = 2
    warning = 3
    info = 4
    debug = 5
    verbose = 6

    def __init__(self, value):
        self._as_parameter = int(value)

    @classmethod
    def from_param(cls, obj):
        return int(obj)


np_double_1D = npct.ndpointer(dtype=np.double, ndim=1, flags='CONTIGUOUS')
np_float_1D  = npct.ndpointer(dtype=np.float32, ndim=1, flags='CONTIGUOUS')
np_int16_1D  = npct.ndpointer(dtype=np.int16, ndim=1, flags='CONTIGUOUS')
np_int8_1D   = npct.ndpointer(dtype=np.int8, ndim=1, flags='CONTIGUOUS')
np_uint64_1D = npct.ndpointer(dtype=np.uint64, ndim=1, flags='CONTIGUOUS')
np_uint32_1D = npct.ndpointer(dtype=np.uint32, ndim=1, flags='CONTIGUOUS')
np_uint16_1D = npct.ndpointer(dtype=np.uint16, ndim=1, flags='CONTIGUOUS')
np_uint8_1D  = npct.ndpointer(dtype=np.uint8, ndim=1, flags='CONTIGUOUS')

# load the library
libaps2 = None
try:
    # first, try the obvious place

    # determine OS
    if sys.platform == 'windows':
        suffix = '\\Library\\bin'
        libpath = sys.prefix + suffix
        libaps2 = CDLL(libpath)
    elif sys.platform == 'linux':
        libpath = '../../build/libaps2.so'
        libaps2 = CDLL(libpath)
        suffix = ''
    elif sys.platform == 'darwin':
        libpath = '../../build/libaps2.dylib'
        libaps2 = CDLL(libpath)
        suffix = ''
    else:
        suffix = ''

except:
    print("Could not find the library in the obvious place.")
    print("Looking in other places.")

    # load the shared library
    # try with and without "lib" prefix
    libpath = find_library("aps2")
    if libpath is None:
        libpath = find_library("libaps2")
        libaps2 = CDLL(libpath)
    # if we still can't find it, then look in python prefix (where conda stores binaries)
    if libpath is None:
        libpath = sys.prefix + suffix
        try:
            libaps2 = npct.load_library("libaps2", libpath)
        except:
            raise Exception("Could not find libaps2 shared library!")

    elif libaps2 is None:
        libpath = sys.prefix + suffix
        try:
            libaps2 = CDLL(libpath)
        except:
            raise Exception("Could not find libaps2 shared library!")

libaps2.get_device_IPs.argtypes              = [POINTER(c_char_p)]
libaps2.get_device_IPs.restype               = c_int
libaps2.get_firmware_version.argtypes        = [c_char_p, POINTER(c_ulong), POINTER(
    c_ulong), POINTER(c_ulong), c_char_p]
libaps2.get_firmware_version.restype         = c_int
libaps2.set_waveform_float.argtypes          = [c_char_p, c_int, np_float_1D, c_int]
libaps2.set_waveform_float.restype           = c_int
libaps2.set_waveform_int.argtypes            = [c_char_p, c_int, np_int16_1D, c_int]
libaps2.set_waveform_int.restype             = c_int
libaps2.set_markers.argtypes                 = [c_char_p, c_int, np_int8_1D, c_int]
libaps2.set_markers.restype                  = c_int
libaps2.write_sequence.argtypes              = [c_char_p, np_uint64_1D, c_ulong]
libaps2.write_sequence.restype               = c_int
libaps2.load_sequence_file.argtypes          = [c_char_p, c_char_p]
libaps2.load_sequence_file.restype           = c_int
libaps2.set_log.argtypes                     = [c_char_p]
libaps2.set_log.restype                      = c_int
libaps2.set_file_logging_level.argtypes      = [PlogSeverity]
libaps2.set_file_logging_level.restype       = c_int
libaps2.set_console_logging_level.argtypes   = [PlogSeverity]
libaps2.set_console_logging_level.restype    = c_int
libaps2.get_ip_addr.argtypes                 = [c_char_p, POINTER(c_char)]
libaps2.get_ip_addr.restype                  = c_int
libaps2.set_ip_addr.argtypes                 = [c_char_p, c_char_p]
libaps2.set_ip_addr.restype                  = c_int
libaps2.get_mixer_correction_matrix.argtypes = [c_char_p,
    npct.ndpointer(dtype=np.float32, ndim=2, flags='C_CONTIGUOUS')]
libaps2.get_mixer_correction_matrix.restype  = c_int
libaps2.set_mixer_correction_matrix.argtypes = [c_char_p,
    npct.ndpointer(dtype=np.float32, ndim=2, flags='C_CONTIGUOUS')]
libaps2.set_mixer_correction_matrix.restype  = c_int

# APS2_TRIGGER_SOURCE
EXTERNAL = 0
INTERNAL = 1
SOFTWARE = 2
SYSTEM   = 3

trigger_dict = {0: "EXTERNAL", 1: "INTERNAL", 2: "SOFTWARE", 3: "SYSTEM"}

# APS2_RUN_MODE
RUN_SEQUENCE  = 0
TRIG_WAVEFORM = 1
CW_WAVEFORM   = 2

run_mode_dict = {0: "RUN_SEQUENCE", 1: "TRIG_WAVEFORM", 2: "CW_WAVEFORM"}

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
    0: "APS2_OK",
    -1: "APS2_UNKNOWN_ERROR",
    -2: "APS2_NO_DEVICE_FOUND",
    -3: "APS2_UNCONNECTED",
    -4: "APS2_RESET_TIMEOUT",
    -5: "APS2_FILELOG_ERROR",
    -6: "APS2_SEQFILE_FAIL",
    -7: "APS2_PLL_LOST_LOCK",
    -8: "APS2_MMCM_LOST_LOCK",
    -9: "APS2_UNKNOWN_RUN_MODE",
    -10: "APS2_FAILED_TO_CONNECT",
    -11: "APS2_INVALID_DAC",
    -12: "APS2_NO_SUCH_BITFILE",
    -13: "APS2_MAC_ADDR_VALIDATION_FAILURE",
    -14: "APS2_IP_ADDR_VALIDATION_FAILURE",
    -15: "APS2_DHCP_VALIDATION_FAILURE",
    -16: "APS2_RECEIVE_TIMEOUT",
    -17: "APS2_SOCKET_FAILURE",
    -18: "APS2_INVALID_IP_ADDR",
    -19: "APS2_COMMS_ERROR",
    -20: "APS2_UNALIGNED_MEMORY_ACCESS",
    -21: "APS2_ERPOM_ERASE_FAILURE",
    -22: "APS2_BITFILE_VALIDATION_FAILURE",
    -23: "APS2_BAD_PLL_VALUE",
}

libaps2.get_error_msg.restype = c_char_p


def get_error_msg(status_code):
    return libaps2.get_error_msg(status_code).decode("utf-8")


def check(status_code):
    if status_code is 0:
        return
    else:
        raise Exception("APS2 Error: {} - {}".format(status_dict[
            status_code], get_error_msg(status_code)))


def get_numDevices():
    num_devices = c_uint()
    check(libaps2.get_numDevices(byref(num_devices)))
    return num_devices.value


def get_device_IPs():
    num_devices = get_numDevices()
    if num_devices > 0:
        results = (c_char_p * num_devices)(addressof(create_string_buffer(16)))
        check(libaps2.get_device_IPs(results))
        return [r.decode('ascii') for r in results]
    else:
        return None


def enumerate():
    device_IPs = get_device_IPs()
    if device_IPs:
        n = len(device_IPs)
    else:
        n = 0
    return (n, device_IPs)


def set_log(filename):
    check(libaps2.set_log(filename.encode('utf-8')))


def set_file_logging_level(level):
    check(libaps2.set_file_logging_level(level))

def set_console_logging_level(level):
    check(libaps2.set_console_logging_level(level))

class APS2_Getter():
    def __init__(self, arg_type, return_type=None):
        super(APS2_Getter, self).__init__()
        self.arg_type = arg_type
        self.return_type = return_type


class APS2_Setter():
    def __init__(self, arg_type):
        super(APS2_Setter, self).__init__()
        self.arg_type = arg_type


class APS2_Chan_Getter(APS2_Getter):
    def __init__(self, arg_type, **kwargs):
        super(APS2_Chan_Getter, self).__init__(arg_type, **kwargs)


class APS2_Chan_Setter(APS2_Setter):
    def __init__(self, arg_type):
        super(APS2_Chan_Setter, self).__init__(arg_type)


class APS2_Call(object):
    def __init__(self):
        super(APS2_Call, self).__init__()


def add_call(instr, name, cmd):
    getattr(libaps2, name).restype = c_int
    getattr(libaps2, name).argtypes = [c_char_p]

    def f(self):
        status_code = getattr(libaps2, name)(self.ip_address.encode('utf-8'))
        check(status_code)

    setattr(instr, name, f)


def add_setter(instr, name, cmd):
    getattr(libaps2, name).restype = c_int
    if isinstance(cmd, APS2_Chan_Setter):
        # There's an extra channel parameter here
        getattr(libaps2, name).argtypes = [c_char_p, c_int, cmd.arg_type]
    else:
        getattr(libaps2, name).argtypes = [c_char_p, cmd.arg_type]

    def f(self, *args):
        args = list(args)
        if isinstance(cmd, APS2_Chan_Setter):
            if len(args) != 2:
                raise Exception("Wrong number of arguments given to API call.")
            args = [self.ip_address.encode('utf-8'), args[0], args[1]]
        else:
            if len(args) != 1:
                raise Exception("Wrong number of arguments given to API call.")
            args = [self.ip_address.encode('utf-8'), args[0]]

        status_code = getattr(libaps2, name)(*args)
        check(status_code)

    setattr(instr, name, f)


def add_getter(instr, name, cmd):
    getattr(libaps2, name).restype = c_int
    if isinstance(cmd, APS2_Chan_Getter):
        # There's an extra channel parameter here
        getattr(libaps2,
                name).argtypes = [c_char_p, c_int, POINTER(cmd.arg_type)]
    else:
        getattr(libaps2, name).argtypes = [c_char_p, POINTER(cmd.arg_type)]

    def f(self, *args):
        if args is None:
            args = []
        if isinstance(cmd, APS2_Chan_Getter):
            if len(args) != 1:
                raise Exception("Wrong number of arguments given to API call.")
            args = [self.ip_address.encode('utf-8'), args[0]]
        else:
            if len(args) != 0:
                raise Exception("Wrong number of arguments given to API call.")
            args = [self.ip_address.encode('utf-8')]

        # Instantiate the values
        var = cmd.arg_type()
        args.append(byref(var))
        status_code = getattr(libaps2, name)(*args)
        if status_code is 0:
            if cmd.return_type is None:
                return var.value
            else:
                return cmd.return_type(var.value)
        else:
            raise Exception("APS Error: {}".format(status_dict[status_code]))

    setattr(instr, name, f)


class Parser(type):
    """Meta class to create instrument classes with controls turned into descriptors.
	"""

    def __init__(self, name, bases, dct):
        type.__init__(self, name, bases, dct)
        for k, v in dct.items():
            if isinstance(v, APS2_Call):
                add_call(self, k, v)
            elif isinstance(v, APS2_Getter):
                add_getter(self, k, v)
            elif isinstance(v, APS2_Setter):
                add_setter(self, k, v)


class APS2(metaclass=Parser):
    # Simple calls, take only IP address
    connect_APS = APS2_Call()
    disconnect_APS = APS2_Call()
    stop = APS2_Call()
    run = APS2_Call()
    trigger = APS2_Call()
    clear_channel_data = APS2_Call()

    enable_DAC_output  = APS2_Call()
    disable_DAC_output = APS2_Call()

    # Getters and Setters
    get_uptime = APS2_Getter(c_double)
    get_fpga_temperature = APS2_Getter(c_float)
    get_runState = APS2_Getter(c_int)
    set_run_mode = APS2_Setter(c_int)

    set_dhcp_enable = APS2_Setter(c_int)
    get_dhcp_enable = APS2_Getter(c_int, return_type=bool)

    set_sampleRate = APS2_Setter(c_uint)
    get_sampleRate = APS2_Getter(c_uint)

    reset = APS2_Setter(c_int)

    set_trigger_source = APS2_Setter(c_int)
    get_trigger_source = APS2_Getter(c_int)

    set_trigger_interval = APS2_Setter(c_double)
    get_trigger_interval = APS2_Getter(c_double)

    set_trigger_source = APS2_Setter(c_int)
    get_trigger_source = APS2_Getter(c_int)

    # These are per channel commands, take IP and channel number plus these args
    set_channel_offset = APS2_Chan_Setter(c_float)
    get_channel_offset = APS2_Chan_Getter(c_float)

    set_channel_scale = APS2_Chan_Setter(c_float)
    get_channel_scale = APS2_Chan_Getter(c_float)

    set_channel_enabled = APS2_Chan_Setter(c_int)
    get_channel_enabled = APS2_Chan_Getter(c_int)#, return_type=bool)

    set_channel_delay = APS2_Chan_Setter(c_uint)
    get_channel_delay = APS2_Chan_Getter(c_uint)

    set_waveform_frequency = APS2_Setter(c_float)
    get_waveform_frequency = APS2_Getter(c_float)

    set_mixer_amplitude_imbalance = APS2_Setter(c_float)
    get_mixer_amplitude_imbalance = APS2_Getter(c_float)

    set_mixer_phase_skew = APS2_Setter(c_float)
    get_mixer_phase_skew = APS2_Getter(c_float)

    def __init__(self):
        super(APS2, self).__init__()
        self.ip_address = ""

    def __del__(self):
        try:
            self.disconnect()
        except Exception as e:
            pass

    def connect(self, ip_address):
        if self.ip_address:
            #Disconnect before connecting to avoid dangling connections
            warnings.warn(
                "Disconnecting from {} before connection to {}".format(
                    self.ip_address, ip_address))
            self.disconnect()
        try:
            self.ip_address = ip_address
            self.connect_APS()
            self.init()
        except Exception as e:
            self.ip_address = ""
            raise e

    def disconnect(self):
        self.disconnect_APS()
        self.ip_address = ""

    def get_firmware_version(self):
        version = c_ulong()
        sha = c_ulong()
        timestamp = c_ulong()
        string = create_string_buffer(64)
        check(libaps2.get_firmware_version(
            self.ip_address.encode('utf-8'), byref(version), byref(sha), byref(
                timestamp), string))
        return version.value, sha.value, timestamp.value, string.value.decode(
            'ascii')

    def init(self, force=0):
        check(libaps2.init_APS(self.ip_address.encode('utf-8'), force))

    def set_waveform_float(self, channel, data):
        num_points = len(data)
        check(libaps2.set_waveform_float(
            self.ip_address.encode('utf-8'), channel, data, num_points))

    def set_waveform_int(self, channel, data):
        num_points = len(data)
        check(libaps2.set_waveform_int(
            self.ip_address.encode('utf-8'), channel, data, num_points))

    def set_markers(self, channel, data):
        num_points = len(data)
        check(libaps2.set_markers(
            self.ip_address.encode('utf-8'), channel, data, num_points))

    def write_sequence(self, data):
        num_points = len(data)
        check(libaps2.write_sequence(
            self.ip_address.encode('utf-8'), data, num_points))

    def load_sequence_file(self, filename):
        filename = filename.replace("\\", "\\\\")
        check(libaps2.load_sequence_file(
            self.ip_address.encode('utf-8'), filename.encode('utf-8')))

    def get_ip_addr(self):
        addr = create_string_buffer(64)
        check(libaps2.get_ip_addr(self.ip_address.encode('utf-8'), addr))
        return addr.value.decode('ascii')

    def set_ip_addr(self, new_ip_address):
        check(libaps2.set_ip_addr(
            self.ip_address.encode('utf-8'), new_ip_address.encode('utf-8')))

    def get_mixer_correction_matrix(self):
        matrix = np.zeros((2, 2), dtype=np.float32)
        check(libaps2.get_mixer_correction_matrix(
            self.ip_address.encode('utf-8'), matrix))
        return matrix

    def set_mixer_correction_matrix(self, matrix):
        check(libaps2.set_mixer_correction_matrix(
            self.ip_address.encode('utf-8'), matrix))
