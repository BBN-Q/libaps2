/*
 * libaps2.h
 *
 *  Created on: Jun 25, 2012
 *      Author: qlab
 */

#ifndef LIBAPS_H_
#define LIBAPS_H_

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

typedef enum {EXTERNAL=0, INTERNAL, SOFTWARE} TRIGGER_SOURCE;

typedef enum {RUN_SEQUENCE=0, TRIG_WAVEFORM, CW_WAVEFORM} RUN_MODE;

typedef enum {STOPPED=0, PLAYING} RUN_STATE;

typedef enum {logERROR, logWARNING, logINFO, logDEBUG, logDEBUG1, logDEBUG2, logDEBUG3, logDEBUG4} TLogLevel;

typedef enum {
	APS2_OK,
	APS2_UNKNOWN_ERROR = -1,
	APS2_NO_DEVICE_FOUND = -2,
	APS2_UNCONNECTED = -3,
	APS2_RESET_TIMEOUT =-4,
	APS2_FILELOG_ERROR = -5,
	APS2_SEQFILE_FAIL = -6,
	APS2_PLL_LOST_LOCK = -7,
	APS2_MMCM_LOST_LOCK = -8,
} APS2_STATUS;

#ifdef __cplusplus
extern "C" {
#endif

EXPORT const char* get_error_msg(APS2_STATUS);

EXPORT APS2_STATUS get_numDevices(unsigned int*);
EXPORT APS2_STATUS get_deviceSerials(const char**);

EXPORT APS2_STATUS connect_APS(const char*);
EXPORT APS2_STATUS disconnect_APS(const char*);

EXPORT APS2_STATUS reset(const char*, int);
EXPORT APS2_STATUS init_APS(const char*, int);

EXPORT APS2_STATUS get_firmware_version(const char*, unsigned int*);
EXPORT APS2_STATUS get_uptime(const char*, double*);
EXPORT APS2_STATUS get_fpga_temperature(const char*, double*);

EXPORT APS2_STATUS set_sampleRate(const char*, unsigned int);
EXPORT APS2_STATUS get_sampleRate(const char*, unsigned int*);

EXPORT APS2_STATUS set_channel_offset(const char*, int, float);
EXPORT APS2_STATUS get_channel_offset(const char*, int, float*);
EXPORT APS2_STATUS set_channel_scale(const char*, int, float);
EXPORT APS2_STATUS get_channel_scale(const char*, int, float*);
EXPORT APS2_STATUS set_channel_enabled(const char*, int, int);
EXPORT APS2_STATUS get_channel_enabled(const char*, int, int*);

EXPORT APS2_STATUS set_trigger_source(const char*, TRIGGER_SOURCE);
EXPORT APS2_STATUS get_trigger_source(const char*, TRIGGER_SOURCE*);
EXPORT APS2_STATUS set_trigger_interval(const char*, double);
EXPORT APS2_STATUS get_trigger_interval(const char*, double*);
EXPORT APS2_STATUS trigger(const char*);

EXPORT APS2_STATUS set_waveform_float(const char*, int, float*, int);
EXPORT APS2_STATUS set_waveform_int(const char*, int, short*, int);
EXPORT APS2_STATUS set_markers(const char*, int, char*, int);

EXPORT APS2_STATUS write_sequence(const char*, unsigned __int64*, unsigned int);

EXPORT APS2_STATUS set_run_mode(const char*, RUN_MODE);

EXPORT APS2_STATUS load_sequence_file(const char*, const char*);

EXPORT APS2_STATUS clear_channel_data(const char*);

EXPORT APS2_STATUS run(const char*);
EXPORT APS2_STATUS stop(const char*);
EXPORT APS2_STATUS get_runState(const char*, RUN_STATE*);

EXPORT APS2_STATUS set_log(const char*);
EXPORT APS2_STATUS set_logging_level(TLogLevel);

EXPORT APS2_STATUS get_ip_addr(const char*, char*);
EXPORT APS2_STATUS set_ip_addr(const char*, const char*);

/* private API methods */

EXPORT int write_memory(const char*, unsigned int, unsigned int*, unsigned int);
EXPORT int read_memory(const char*, unsigned int, unsigned int*, unsigned int);
EXPORT int read_register(const char*, unsigned int);
EXPORT int program_FPGA(const char*, const char*);

EXPORT int write_flash(const char*, unsigned int, unsigned int*, unsigned int);
EXPORT int read_flash(const char*, unsigned int, unsigned int, unsigned int*);

EXPORT unsigned __int64 get_mac_addr(const char*);
EXPORT int set_mac_addr(const char*, unsigned __int64);

EXPORT int write_SPI_setup(const char*);

EXPORT int run_DAC_BIST(const char*, const int, short*, unsigned int, unsigned int*);
EXPORT int set_DAC_SD(const char*, const int, const char);

#ifdef __cplusplus
}
#endif

#endif /* LIBAPS_H_ */
