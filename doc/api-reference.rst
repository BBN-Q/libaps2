API Reference
=============

BBN provides a C-API DLL for communicating with the APS2, as well as MATLAB
and Julia wrappers for the driver. Below is the API documentation for the C
DLL. We follow language conventions for index arguments, so channel arguments
in the C-API are zero-indexed, while in MATLAB and Julia they are one-indexed.
Most of the C-API methods require a device serial (an IP address) as the first
argument. In MATLAB and Julia, the serial is stored in a device object, and so
is dropped from all methods except for `connect()`.

High-level methods
------------------

`int init()`

	Initializes the libaps2 driver. You must call this method after loading
	the libaps2 DLL into memory before the other methods will work.

`int enumerate_devices()`

	Called automatically by `init()`, this method sends out a broadcast packet
	to find all APS2's on the local subnet. You may want to manually call this
	method if an APS2 has been (dis)connected from the network. Follow this up
	with a call to `get_deviceSerials()` to get a list of APS2 IP addresses.
	Returns an APS_STATUS value.

`int get_numDevices()`

	This method will return the number of APS2's found on the local subnet in the
	last enumeration attempt.

`void get_deviceSerials(const char ** deviceIPs)`

	Populates `deviceIPs[]` with C strings of APS2 IP addresses. The caller is
	responsible for sizing deviceIPs appropriately. For example, in C++::

		int numDevices = get_numDevices();
		const char ** serialBuffer = new const char*[numDevices];
		get_deviceSerials(serialBuffer);

`int connect_APS(const char * deviceIP)`

	Connects to the APS2 at the given IP address.

`int disconnect_APS(const char * deviceIP)`

	Disconnects the APS2 at the given IP address.

`int reset(const char * deviceIP, int resetMode)`

	Resets the APS2 at the given IP address. The `resetMode` parameter can be used
	to do a HARD (`resetMode` = 0) or SOFT (`resetMode` = 1) reset.

`int initAPS(const char * deviceIP, int force)`

	This method initializes the APS2 at the given IP address, which synchronizes
	and calibrates the DAC clock timing. If `force` = 0, the driver will attempt
	to determine if this procedure has already been run and return immediately. To
	force the driver to run the initialization procedure, call with `force` = 1.

`int get_bitfile_version(const char * deviceIP)`

	Returns the version number of the currently loaded firmware. The major version
	number is contained in bits 15-8, while the minor version number is in bits
	7-0. So, a returned value of 513 indicates version 2.1.

`double get_uptime(const char * deviceIP)`

	Returns the APS2 uptime in seconds.

`uint64_t get_mac_addr(const char * deviceIP)`

	Returns the MAC address of the APS2 at the given IP address.

`int set_ip_addr(const char * deviceIP, const char * ip_addr)`

	Sets the IP address of the APS2 currently at `deviceIP` to `ip_addr`. The
	IP address does not actually update until `reset()` is called, or the
	device is power cycled.

`int set_sampleRate(const char * deviceIP, int rate)`

	Sets the output sampling rate of the APS2 to `rate` (in MHz). By default the
	APS2 initializes with a rate of 1200 MHz. The allow values for rate are: 1200,
	600, 300, and 200. **WARNING**: the APS2 firmware has not been tested with
	sampling rates other than the default of 1200. In particular, it is expected
	that DAC synchronization will fail at other update rates.

`int get_sampleRate(const char * deviceIP)`

	Returns the current APS2 sampling rate in MHz.

`int set_channel_offset(const char * deviceIP, int channel, float offset)`

	Sets the offset of `channel` to `offset`. Note that the APS2 offsets the
	channels by digitally shifting the waveform values, so non-zero values of
	offset may cause clipping to occur.

`float get_channel_offset(const char * deviceIP, int channel)`

	Returns the current offset value of `channel`.

`int set_channel_scale(const char * deviceIP, int channel, float scale)`

	Sets the scale parameter for `channel` to `scale`. This method will cause the
	currently loaded waveforms (and all subsequently loaded ones) to be multiplied
	by `scale`. Values greater than 1 may cause clipping.

`float get_channel_scale(const char * deviceIP, int channel)`

	Returns the scale parameter for `channel`.

`int set_channel_enabled(const char * deviceIP, int channel, int enabled)`

	Enables (`enabled` = 1) or disables (`enabled` = 0) `channel`.

`int get_channel_enabled(const char * deviceIP, int channel)`

	Returns the enabled state of `channel`.

`int set_trigger_source(const char * deviceIP, int triggerSource)`

	Sets the trigger source to external (`triggerSource` = 0) or internal (`triggerSource` = 1).

`int get_trigger_source(const char * deviceIP)`

	Returns the current trigger source.

`int set_trigger_interval(const char * deviceIP, double interval)`

	Set the internal trigger interval to `interval` (in seconds).

`double get_trigger_interval(const char * deviceIP)`

	Returns the current internal trigger interval.

`int set_waveform_float(const char * deviceIP, int channel, float* data, int numPts)`

	Uploads `data` to `channel`'s waveform memory. `numPts` indicates the
	length of the `data` array. :math:`\pm 1` indicate full-scale output.

`int set_waveform_int(const char * deviceIP, int channel, int16_t* data, int numPts)`
	
	Uploads `data` to `channel`'s waveform memory. `numPts` indicates the
	length of the `data` array. Data should contain 14-bit waveform data sign-
	extended int16's. Bits 14-13 in each array element will be ignored.

`int set_markers(const char * deviceIP, int channel, uint8_t* data, int numPts)`

	**FOR FUTURE USE ONLY** Will add marker data in `data` to the currently
	loaded waveform on `channel`.

`int write_sequence(const char * deviceIP, uint64_t* data, uint32_t numWords)`

	Writes instruction sequence in `data` of length `numWords`.

`int load_sequence_file(const char * deviceIP, const char* seqFile)`

	Loads the APS2-structured HDF5 file given by the path `seqFile`. Be aware
	the backslash character must be escaped (doubled) in C strings.

`int set_run_mode(const char * deviceIP, int mode)`

	**FOR FUTURE USE ONLY** Changes the APS2 run mode from sequence (`mode` = 0)
	to waveform (`mode` = 1)

`int run(const char * deviceIP)`

	Enables the pulse sequencer.

`int stop(const char * deviceIP)`

	Disables the pulse sequencer. The driver attempts to allow completion of
	the currently playing sequence by temporarily disabling the internal
	trigger and waiting for 1 second before stopping the sequencer. In many
	cases, this allows a sequence to be run once by immediately calling
	`stop()` after calling `run()`.

`int get_running(const char * deviceIP)`

	Returns the running state of the APS2.


Low-level methods
-----------------

`int set_logging_level(int level)`

	Sets the logging level to `level` (values between 0-8). Determines the
	amount of information written to the APS2 log file. The default logging
	level is 2.

`int write_memory(const char * deviceIP, uint32_t addr, uint32_t* data, uint32_t numWords)`

	Write `numWords` of `data` to the APS2 memory starting at `addr`.

`int read_memory(const char * deviceIP, uint32_t addr, uint32_t* data, uint32_t numWords)`

	Read `numWords` into `data` from the APS2 memory starting at `addr`.

`int read_register(const char * deviceIP, uint32_t addr)`

	Returns the value of the APS2 register at `addr`.
