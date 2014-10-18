#!/usr/bin/env sed -f
# run as create_matlab_header.sed ../lib/libaps2.h > libaps2.matlab.h
s/uint8_t/char/g
s/int16_t/short/g
s/uint32_t/unsigned int/g
s/uint64_t/unsigned __int64/g
