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
:ref:`Noise Comparison <noise-figure>`.

.. _noise-figure:

.. figure:: images/aps-ii-tek-noise-comparison.*
	:figwidth: 60%

	**Comparison of AWG output noise** Output noise power versus frequency for
	the Tektronix AWG5014, Innovative Integration X6-1000M, and BBN APS. The
	APS's linear power supplies and low-noise output amplifier lead to signficant
	improvements in the noise performance. The II X6 is significantly better
	than the Tek5014, but suffers from resonances in the noise spectrum because
	it is in a host PC environment.

Detailed Specifications
-----------------------

.. figure:: images/APS2-front.jpg
	:scale: 50%
	:figwidth: 60%

	**BBN APS2 front panel** The front panel of the APS has two analog outputs,
	4 marker outputs, a trigger input, two SATA ports, a 1 GigE port, a
	10 MHz reference input and two status LEDs.

========================  ==============================================================
Analog channels           two 14-bit 1.2 GS/s outputs per module
Digital channels          four 1.5V outputs per module
Analog Jitter             7.5ps RMS
Digital Jitter            5ps RMS
Rise/fall time            2ns
Settling time             2ns to 10%, 10ns to 1%
Trigger modes             Internal, external, system, or software triggering
Ext. trigger input        1 V minimum into 50 Ω, 5 V maximum; triggered on *rising* edge
Reference input           10 MHz sine or square, 1V to 3.3V peak to peak (+4 to +14 dBm)
Waveform cache            128K samples
Sequence memory           64M instructions
Min instruction duration  8 samples
Max instruction duration  8M samples (~7ms at 1.2GS/s)
Max loop repeats          65,536
========================  ==============================================================

Triggering
------------------------

The APS2 supports four different types of triggers. The *internal* mode
generates triggers on a programmable interval between 6.66ns and 14s. The
*external* mode listens for triggers on the front-panel SMA "trigger input"
port. In this mode, the APS2 is triggered on the rising edge of a 1-5V signal.
The *system* trigger accepts triggers on the SATA input port from the APS2
Trigger Distribution Module (TDM). Finally, the *software* mode allows the user
to trigger the APS2 via the host computer with the `trigger()` API method.

Communications Interface
------------------------

The APS2 communicates with a host PC via UDP/TCP over 1GigE. 1GigE is required
so ensure all switches between the host computer and APS2 support 1GigE. The
APS2 supports static and DHCP assigned IP addresses. Instructions for setting
the APS2 IP addresses are contained in the :ref:`software-installation` section.

Status LED's
------------------------

The L1 and L2 LEDs provide status indicators for the communication (L1)
and sequencing (L2) firmware components.

L1:

* off - SFP port failure
* green breathing - no ethernet connection;
* solid green - link established (but not necessarily connected to host);
* green blinks - receiving or transmitting data;
* red - fatal communication error. Power cycle the module to restore connectivity.

L2:

* dark - idle;
* solid green - playback enabled and outputing sequences;
* green breathing - playback enabled but no trigger received in the past 100ms;
* solid red - fatal cache controller error. Power cycle the module to restore playback
  functionality.
* blinking red - cache stall in playback.  See :ref:`cache` for details.
