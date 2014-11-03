API Reference
=============

BBN provides a C-API shared library (libaps2) for communicating with the APS2,
as well as MATLAB and Julia wrappers for the driver.  We follow language
conventions for index arguments, so channel arguments in the C-API are zero-
indexed, while in MATLAB and Julia they are one-indexed. Most of the C-API
methods require a device serial (an IP address) as the first argument. In
MATLAB and Julia, the serial is stored in a device object and helper functions
inject it as necessary.

Enums
------------------

Nearly all the library calls return an ``APS2_STATUS`` enum.  If there are no
errors then this will be ``APS2_OK``. Otherwise a more detailed description of
the error an be obtained from ``get_error_msg``.  See the Matlab and Julia
drivers for examples of how to wrap each library call with error checking. The
enum and descriptions can be found ``APS2_errno.h``.  

There are also enums for the trigger mode, run mode, running status and
logging level.  These can be found in ``APS2_enums.h`` or ``logger.h``.

High-level methods
------------------

Getter calls return the value in the appropriate pointer. 

`const char* get_error_msg(APS2_STATUS)`

	Returns the null-terminated error message string associated with the
	``APS2_STATUS`` code.

`APS2_STATUS get_numDevices(unsigned int* numDevices)`

	This method sends out a broadcast packet to find all APS2's on the local
	subnet and returns the number of devices found.

`APS2_STATUS get_deviceSerials(const char** deviceIPs)`

	Populates `deviceIPs[]` with C strings of APS2 IP addresses. The caller is
	responsible for sizing deviceIPs appropriately. For example, in C++::

		int numDevices = get_numDevices();
		const char** serialBuffer = new const char*[numDevices];
		get_deviceSerials(serialBuffer);

`APS2_STATUS connect_APS(const char* deviceIP)`

	Connects to the APS2 at the given IP address.

`APS2_STATUS disconnect_APS(const char* deviceIP)`

	Disconnects the APS2 at the given IP address.

`APS2_STATUS reset(const char* deviceIP, int resetMode)`

	Resets the APS2 at the given IP address. The `resetMode` parameter can be
	used to do a hard reset from non-volatile flash memory (`resetMode` = 0)
	or a soft reset from volatile DRAM (`resetMode` = 1) reset.

`APS2_STATUS initAPS(const char* deviceIP, int force)`

	This method initializes the APS2 at the given IP address, which synchronizes
	and calibrates the DAC clock timing. If `force` = 0, the driver will attempt
	to determine if this procedure has already been run and return immediately. To
	force the driver to run the initialization procedure, call with `force` = 1.

`APS2_STATUS get_firmware_version(const char* deviceIP, uint32_t* version)`

	Returns the version number of the currently loaded firmware. The major version
	number is contained in bits 15-8, while the minor version number is in bits
	7-0. So, a returned value of 513 indicates version 2.1.

`APS2_STATUS get_uptime(const char* deviceIP, double* upTime)`

	Returns the APS2 uptime in seconds.

`APS2_STATUS get_mac_addr(const char* deviceIP, uint64_t* MAC)`

	Returns the MAC address of the APS2 at the given IP address.

`APS2_STATUS set_ip_addr(const char* deviceIP, const char* ip_addr)`

	Sets the IP address of the APS2 currently at `deviceIP` to `ip_addr`. The
	IP address does not actually update until `reset()` is called, or the
	device is power cycled.  Note that if you change the IP and reset you will
	have to disconnect and re-enumerate for the driver to pick up the new IP
	address.

`APS2_STATUS set_sampleRate(const char* deviceIP, unsigned int rate)`

	Sets the output sampling rate of the APS2 to `rate` (in MHz). By default the
	APS2 initializes with a rate of 1200 MHz. The allow values for rate are: 1200,
	600, 300, and 200. **WARNING**: the APS2 firmware has not been tested with
	sampling rates other than the default of 1200. In particular, it is expected
	that DAC synchronization will fail at other update rates.

`APS2_STATUS get_sampleRate(const char* deviceIP, unsigned int* rate)`

	Returns the current APS2 sampling rate in MHz.

`APS2_STATUS set_channel_offset(const char* deviceIP, int channel, float offset)`

	Sets the offset of `channel` to `offset`. Note that the APS2 offsets the
	channels by digitally shifting the waveform values, so non-zero values of
	offset may cause clipping to occur.

`APS2_STATUS get_channel_offset(const char* deviceIP, int channel, float* offset)`

	Returns the current offset value of `channel`.

`APS2_STATUS set_channel_scale(const char* deviceIP, int channel, float scale)`

	Sets the scale parameter for `channel` to `scale`. This method will cause the
	currently loaded waveforms (and all subsequently loaded ones) to be multiplied
	by `scale`. Values greater than 1 may cause clipping.

`APS2_STATUS get_channel_scale(const char* deviceIP, int channel, float* scale)`

	Returns the scale parameter for `channel`.

`APS2_STATUS set_channel_enabled(const char* deviceIP, int channel, int enabled)`

	Enables (`enabled` = 1) or disables (`enabled` = 0) `channel`.

`APS2_STATUS get_channel_enabled(const char* deviceIP, int channel, int* enabled)`

	Returns the enabled state of `channel`.

`APS2_STATUS set_trigger_source(const char* deviceIP, TRIGGER_SOURCE source)`

	Sets the trigger source to EXTERNAL (), INTERNAL or SOFTWARE.

`APS2_STATUS get_trigger_source(const char* deviceIP, TRIGGER_SOURCE* source)`

	Returns the current trigger source.

`APS2_STATUS set_trigger_interval(const char* deviceIP, double interval)`

	Set the internal trigger interval to `interval` (in seconds).  The
	internal trigger has a resolution of 3.333 ns and a minimum interval of
	6.67ns and maximum interval of ``2^32+1 * 3.333 ns ~ 14.17s``.

`APS2_STATUS get_trigger_interval(const char* deviceIP, double* interval)`

	Returns the current internal trigger interval.

`APS2_STATUS trigger(const char* deviceIP)`

	Sends a software trigger to the APS2.

`APS2_STATUS set_waveform_float(const char* deviceIP, int channel, float* data, int numPts)`

	Uploads `data` to `channel`'s waveform memory. `numPts` indicates the
	length of the `data` array. :math:`\pm 1` indicate full-scale output.

`APS2_STATUS set_waveform_int(const char* deviceIP, int channel, int16_t* data, int numPts)`
	
	Uploads `data` to `channel`'s waveform memory. `numPts` indicates the
	length of the `data` array. Data should contain 14-bit waveform data sign-
	extended int16's. Bits 14-13 in each array element will be ignored.

`APS2_STATUS set_markers(const char* deviceIP, int channel, uint8_t* data, int numPts)`

	**FOR FUTURE USE ONLY** Will add marker data in `data` to the currently
	loaded waveform on `channel`.

`APS2_STATUS write_sequence(const char* deviceIP, uint64_t* data, uint32_t numWords)`

	Writes instruction sequence in `data` of length `numWords`.

`APS2_STATUS load_sequence_file(const char* deviceIP, const char* seqFile)`

	Loads the APS2-structured HDF5 file given by the path `seqFile`. Be aware
	the backslash character must be escaped (doubled) in C strings.

`APS2_STATUS set_run_mode(const char* deviceIP, RUN_MODE mode)`

	Changes the APS2 run mode to sequence (RUN_SEQUENCE, the default),
	triggered  waveform (TRIG_WAVEFORM) or continuous loop waveform
	(CW_WAVEFORM) **IMPORTANT NOTE** The run mode is not a state and the APS2
	does not "remember" its current playback mode.  The waveform modes simply
	load a simple sequence to play a single waveform. In particular, uploading
	new sequence or waveform data will cause the APS2 to return to 'sequence'
	mode. To use 'waveform' mode, call `set_run_mode` only after calling
	`set_waveform_float` or `set_waveform_int`.

`APS2_STATUS run(const char* deviceIP)`

	Enables the pulse sequencer.

`APS2_STATUS stop(const char* deviceIP)`

	Disables the pulse sequencer.

`APS2_STATUS get_runState(const char* deviceIP, RUN_STATE* state)`

	Returns the running state of the APS2.

Low-level methods
-----------------

`int set_log(char* logfile)`

	Directs logging information to `logfile`, which can be either a full file
	path, or one of the special strings "stdout" or "stderr".

`int set_logging_level(TLogLevel level)`

	Sets the logging level to `level` (values between 0-8 logINFO to logDEBUG4). Determines the
	amount of information written to the APS2 log file. The default logging
	level is 2 or logINFO.

`int write_memory(const char* deviceIP, uint32_t addr, uint32_t* data, uint32_t numWords)`

	Write `numWords` of `data` to the APS2 memory starting at `addr`.

`int read_memory(const char* deviceIP, uint32_t addr, uint32_t* data, uint32_t numWords)`

	Read `numWords` into `data` from the APS2 memory starting at `addr`.

`int read_register(const char* deviceIP, uint32_t addr)`

	Returns the value of the APS2 register at `addr`.
