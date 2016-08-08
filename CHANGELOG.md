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
