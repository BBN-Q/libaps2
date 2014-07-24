Formats
=======

Waveforms
---------

Stored as arrays of signed 16-bit integers.


Instructions
------------

Stored as arrays of unsigned 64-bit integers, with the instruction header in
the 8 most signficant bits.


Sequence Files
--------------

HDF5 container for waveform and instruction data. The structure of this HDF5 file is as follows:

| /version - attribute containing version information for the container structure
| /chan_1/instructions - uint64 vector of instruction data
| /chan_1/waveforms - int16 vector of waveform data
| /chan_2/waveforms - int16 vector of waveform data
