API Reference
=============

High-level methods
------------------

`int init()`

`int enumerate_devices()`

`int get_numDevices()`

`void get_deviceSerials(const char **)`

`int connect_APS(const char * deviceIP)`

`int disconnect_APS(const char * deviceIP)`

`int reset(const char * deviceIP, int resetMode)`

`int initAPS(const char * deviceIP, int force)`

`int get_bitfile_version(const char * deviceIP)`

`double get_uptime(const char * deviceIP)`

`uint64_t get_mac_addr(const char * deviceIP)`

`int set_mac_addr(const char * deviceIP, uint64_t mac_addr)`

`int set_ip_addr(const char * deviceIP, const char * ip_addr)`

`int set_sampleRate(const char * deviceIP, int rate)`

`int get_sampleRate(const char * deviceIP)`

`int set_channel_offset(const char * deviceIP, int channel, float offset)`

`float get_channel_offset(const char * deviceIP, int channel)`

`int set_channel_scale(const char * deviceIP, int channel, float scale)`

`float get_channel_scale(const char * deviceIP, int channel)`

`int set_channel_enabled(const char * deviceIP, int channel, int enabled)`

`int get_channel_enabled(const char * deviceIP, int channel)`

`int set_trigger_source(const char * deviceIP, int triggerSource)`

`int get_trigger_source(const char * deviceIP)`

`int set_trigger_interval(const char * deviceIP, double interval)`

`double get_trigger_interval(const char * deviceIP)`

`int set_waveform_float(const char * deviceIP, int channel, float* data, int numPts)`

`int set_waveform_int(const char * deviceIP, int channel, int16_t* data, int numPts)`

`int set_markers(const char * deviceIP, int channel, uint8_t* data, int numPts)`

`int write_sequence(const char * deviceIP, uint32_t* data, uint32_t numWords)`

TODO: update this method to take uint64_t's

`int load_sequence_file(const char * deviceIP, const char* seqFile)`

`int set_run_mode(const char * deviceIP, int mode)`

`int run(const char * deviceIP)`

`int stop(const char * deviceIP)`

`int get_running(const char * deviceIP)`


Low-level methods
-----------------

`int set_logging_level(int level)`

`int write_memory(const char * deviceIP, uint32_t addr, uint32_t* data, uint32_t numWords)`

`int read_memory(const char * deviceIP, uint32_t addr, uint32_t* data, uint32_t numWords)`

`int read_register(const char * deviceIP, uint32_t addr)`

`int program_FPGA(const char * deviceIP, const char * bitFile)`
