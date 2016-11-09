# Version 1.2

## Features:
* Compatibility with firmware v4.4
* Faster queued interface between cache and main memory, giving more consistent
performance
* Revised channel alignment strategy that does not require calibration for each
bit file

## Fixes:
* Removes spurious waveform cache miss indicator.

# Version 1.1

## Features:
* consistent and stable aligned DAC outputs with firmware v4.3
* bitslipping for single sample DAC delays

## Fixes:
* only program first slice once when using "all" (#83)
* graceful exit when no APS2 slices found
* Julia channel setter/getters fixed (#60) and removed unnecessary argument typing
* asio updates to allow compiling on gcc 5.x and 6.x (#75)

# Version 1.0

## Features:
* TCP communications for speed and reliability
* Support for register-based channel offset and scaling
* Firmware v4.0+ support including:
    * Add mixer correction methods to API
    * Add modulation (DDS) methods to API
    * Modified instruction cache size
* New python libaps2 wrapper
* Demo scripts in matlab and python
* Option to program all connected APS slices
* Progress bars when programming
* Driver version logging

## Fixes:
* Fix waveform sample order
* Enforce memory alignment for SDRAM and EPROM writes
* Add reset and retry to TCP connect
* Catch errors in TCP reads
* Better error handling when failing to open UDP ports
