API Reference
=============

High-level methods
------------------

`int init()`

`int enumerate_devices()`

`int get_numDevices()`

`void get_deviceSerials(const char **)`

`int connect_APS(const char *)`

`int disconnect_APS(const char *)`

`int reset(const char *, int)`

`int initAPS(const char *, int)`

`int get_bitfile_version(const char *)`

`double get_uptime(const char *)`

`uint64_t get_mac_addr(const char *)`

`int set_mac_addr(const char *, uint64_t)`

`const char * get_ip_addr(const char *)`

`int set_ip_addr(const char *, const char *)`

`int set_sampleRate(const char *, int)`

`int get_sampleRate(const char *)`

`int set_channel_offset(const char *, int, float)`

`float get_channel_offset(const char *, int)`

`int set_channel_scale(const char *, int, float)`

`float get_channel_scale(const char *, int)`

`int set_channel_enabled(const char *, int, int)`

`int get_channel_enabled(const char *, int)`

`int set_trigger_source(const char *, int)`

`int get_trigger_source(const char *)`

`int set_trigger_interval(const char *, double)`

`double get_trigger_interval(const char *)`

`int set_waveform_float(const char *, int, float*, int)`

`int set_waveform_int(const char *, int, int16_t*, int)`

`int set_markers(const char *, int, uint8_t*, int)`

`int write_sequence(const char *, uint32_t*, uint32_t)`

`int load_sequence_file(const char *, const char*)`

`int set_run_mode(const char *, int)`

`int run(const char *)`

`int stop(const char *)`

`int get_running(const char *)`


Low-level methods
-----------------

`int set_log(const char *)`
`int set_logging_level(int)`

`int write_memory(const char *, uint32_t, uint32_t*, uint32_t)`

`int read_memory(const char *, uint32_t, uint32_t*, uint32_t)`

`int read_register(const char *, uint32_t)`

`int program_FPGA(const char *, const char *)`
