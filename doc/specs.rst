Hardware Specifications
=======================

The BBN Arbitrary Pulse Sequencer 2 (APS2) is a modular system providing up to
18 channels of analog waveform generation with a maximum output rate of 1.2
GS/s and 14-bits of vertical resolution. Each module in an APS2 system
provides two analog outputs, DC coupled into a fixed +/- 1V range, and four
digital outputs (1.5 V) for triggering other equipment. Each APS2 module has 1
GB of DDR3 SDRAM for waveform and sequence storage, which is enough for over
64 million sequence instructions. A low-latency cache allows for fast access
to 128K waveform samples. Each module can be independently triggered for
sophisticated waveform scenarios.

The digital and analog circuits have been carefully engineered to provide
extremely low-noise analog performance, resulting in a noise spectral density
that is orders of magnitude lower than competing products, as shown in
Fig NNN.

Detailed Specifications
-----------------------

========================  ==============================================================
Analog channels           two 14-bit 1.2 GS/s outputs per module
Digital channels          four 1.5V outputs per module
Jitter                    28ps RMS (71ps peak-to-peak)
Rise/fall time            2ns
Settling time             2ns to 10%, 10ns to 1%
Trigger input             1 V minimum into 50 Ω, 5 V maximum; triggered on *rising* edge
Waveform cache            128K samples
Sequence memory           64M instructions
Min instruction duration  8 samples
Max instruction duration  1M samples
Max loop repeats          65,536
========================  ==============================================================
