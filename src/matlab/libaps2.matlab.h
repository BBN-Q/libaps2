/*
 * libaps2.h
 *
 *  Created on: Jun 25, 2012
 *      Author: qlab
 */

#ifndef LIBAPS_H_
#define LIBAPS_H_

#include "../lib/APS2_errno.h"
#include "../lib/APS2_enums.h"
enum TLogLevel {logERROR, logWARNING, logINFO, logDEBUG, logDEBUG1, logDEBUG2, logDEBUG3, logDEBUG4};

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h> //fixed-width integer types

//Typedef the enums for C compatibility
typedef enum APS2_STATUS APS2_STATUS;
typedef enum TRIGGER_SOURCE TRIGGER_SOURCE;
typedef enum RUN_MODE RUN_MODE;
typedef enum RUN_STATE RUN_STATE;
typedef enum TLogLevel TLogLevel;

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

EXPORT APS2_STATUS write_sequence(const char*, uint64_t*, unsigned int);

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

EXPORT APS2_STATUS get_dhcp_enable(const char*, int *);
EXPORT APS2_STATUS set_dhcp_enable(const char*, const int*);


/* private API methods */

EXPORT APS2_STATUS write_memory(const char*, unsigned int, unsigned int*, unsigned int);
EXPORT APS2_STATUS read_memory(const char*, unsigned int, unsigned int*, unsigned int);
EXPORT APS2_STATUS read_register(const char*, unsigned int, unsigned int*);
EXPORT int program_FPGA(const char*, const char*);

EXPORT int write_flash(const char*, unsigned int, unsigned int*, unsigned int);
EXPORT int read_flash(const char*, unsigned int, unsigned int, unsigned int*);

EXPORT uint64_t get_mac_addr(const char*);
EXPORT APS2_STATUS set_mac_addr(const char*, uint64_t);

EXPORT APS2_STATUS write_SPI_setup(const char*);

EXPORT int run_DAC_BIST(const char*, const int, short*, unsigned int, unsigned int*);
EXPORT APS2_STATUS set_DAC_SD(const char*, const int, const char);

#ifdef __cplusplus
}
#endif

#endif /* LIBAPS_H_ */
