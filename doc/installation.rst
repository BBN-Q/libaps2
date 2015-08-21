Installation Guide
==================

Hardware
--------

The BBN APS2 system contains one or more analog output modules and an advanced
trigger module in an enclosure that supplies power to each module. Up to 9
analog modules may be installed in a single 19" 8U enclosure, providing 18
analog output channels. Installing a new module only requires plugging it into
a free slot of a powered-off system, then connecting a SATA cable from the new
APS module to the trigger module.

Each module in an APS2 system acts as an independent network endpoint. The
modules communicate with a host computer via a UDP interface over 1GigE.  The
APS2 will not negotiate down to 100Mb or 10Mb so ensure you have an appropriate
switch and patch cable. To ensure high-bandwidth throughput, it is important
that the APS2 and the host computer not be separated by too many network hops.
If possible, locate the host and APS2 on a common switch or router [#f1]_.

While the APS can run in a standalone configuration, we recommend running with
a 10 MHz (+7 dBm) external reference (square wave or sine wave). This
reference must be supplied at the corresponding front panel inputs before
powering on the system. Multiple devices can be synchronized by supplying an
external trigger that is phase locked to this same reference.

.. _software-installation:

Software
--------

In order to control the APS2, BBN provides a Windows shared library. You may
download this driver from our APS2 source code repository
(http://github.com/BBN-Q/libaps2). Click on 'releases' to find the latest
binaries. We provide MATLAB wrappers to this library, but the APS2 may be used
with any software that can call a C-API DLL. To use the MATLAB driver, simply
add the path of the unzipped driver to your MATLAB path. The driver depends on
other shared libraries, for example HDF5 and the gcc libstdc++ from MinGW-w64.
We include these DLL's with the Windows releases and they need be in the same
folder as the driver to ensure they can by dynamically loaded with the libaps2
driver [#f2]_.

File List
~~~~~~~~~~~~~

The releases follow a directory structure that corresponds to the git
repository.

* `examples` - Example sequence and waveform files
	- `wfA.dat` and `wfB.dat` - test waveform patterns for play_waveform.cpp as signed integers one sample per line:
		+ a full scale ramp;
		+ gaussian pulses from 256 samples down to 8 samples with 10ns gaps;
		+ square wave from 256 down to 8 samples with 10ns gaps;
		+ wfB.dat is negative wfA.dat.
	- `ramsey_unslipped.h5` - an 10 sequence Ramsey pattern with 40ns gausssian shaped pulses in a ``40ns delay - X90 - variable delay - X90m pattern`` with the delay stepping from 44 to 104 samples.  All four markers are mirrored and act as blanking pulses around the analog pulses.
	- `ramsey_slipped.h5`` - a similar Ramsey pattern but with the markers slipped by one sample to show the marker resolution and jitter.
* `src` - the source code
	- `src/lib` - the shared library. ``libaps2.h`` contains the public API definitions.
	- `src/matlab` - Matlab bindings to libaps2
	- `src/julia` - Julia bindings to libaps2
	- `src/util` - test and utility command line programs. See below for description.
	- `src/C++` - C++ command line programs to play waveforms and sequences.
	- `src/wireshark` - lua dissector for sniffing APS2 packets.
* `build` - compiled shared library and executable programs
	- Shared library
		+ `libaps2.dll` - the main shared library
		+ load time dependencies for libaps2: `libgcc_s_seh-1.dll, libhdf5-0.dll, libhdf5_cpp-0.dll, libstdc++-6.dll, libwinpthread-1.dll, libszip-0.dll, zlib1.dll`
	- Command line programs
		+ `play_waveform.exe` - command line program to play a single waveform on the analog channels.
		+ `play_sequence.exe` - command line program to play a HDF5 sequence file.
	- Command line utilities
		+ `program.exe` - update the firmwave.  See `Firmware Updates`_.
		+ `flash.exe` - update IP and MAC addresses and the boot chip configuration sequence.
		+ `reset.exe` - reset an APS2.
	- Self-test programs
		+ `test_comms.exe` - tests the ethernet communications writing and reading
		+ `test_DACs.exe` - tests the analog output data integrity with a checksum at three points: leaving the FPGA; arriving at the DAC; leaving the DAC.

Writing Sequences
~~~~~~~~~~~~~~~~~~

The BBN APS2 has advanced sequencing capabilities. Fully taking advantage of
these capabilities may require use of higher-level languages which can be
'compiled down' into sequence instructions. BBN has produced one such
language, called Quantum Gate Language (QGL), as part of the PyQLab suite
(http://github.com/BBN-Q/PyQLab).  We encourage end-users to explore using
QGL for creating pulse sequences. You may also find the sequence file export
code to be a useful template when developing your own libraries. A detailed
instruction format specification can be found in the :ref:`instruction-spec`
section.

Networking Setup
----------------

Once the APS2 has been powered on, the user must assign static IP addresses to
each module. By default, the APS2 modules will have addresses on the
192.168.2.X subnet (e.g. the leftmost module in the system will have the
address 192.168.2.2, and increase sequentially left-to-right). The
``enumerate()`` method in libaps2 may be used to find APS2 modules on your
current subnet. Another method, ``set_ip_addr()`` may be used to program new
IP addresses. Since the APS2 modules will respond to any valid packet on its
port, we recommend placing the APS2 system on a private network, or behind a
firewall.

The control computer must be on the same subnet as the APS2 to respond to
returning packets. Most operating systems allow multiple IP addresses to coexist
on the same network card so the control computer must add a virtual IP on the
subnet.

Windows
~~~~~~~~~~~~~~

Under the Control Panel - Network and Internet - Network Connections click on
the "Local Area Connection" and then properties to change the adapter
settings. Then set the properties of the TCP/IPv4 interface.

.. figure:: images/WindowsDualHome-1.png
	:scale: 100%

	**Step 1** accessing the IPv4 settings for the network interface.

Then under the Advanced tab it will be possible to add additional IP
addresses. Unfortunately, Windows does not support multiple IP addresses with
DHCP so a static address is required for the main network.

.. figure:: images/WindowsDualHome-2.png
	:scale: 100%

	**Step 2** Adding addition IP addresses for the network interface.

Linux
~~~~~~~~~~~~~~~

Temporary IP addresses can be obtained by adding additional ethernet
interfaces::

	sudo ifconfig eth0:0 192.168.2.1 netmask 255.255.255.0 up

A more permanent solution would involve editing the network interfaces file,
e.g. ``/etc/network/interfaces``.

OS X
~~~~~~~~~~~~

In the System Preferences pane under Networking use the "Plus" button to add
an interface.


Firmware Updates
-------------------------

BBN releases periodic firmware updates with bug-fixes and enhancements.  These
can be loaded onto the APS2 modules using the ``program`` executable::

	./program
	BBN AP2 Firmware Programming Executable
	USAGE: program [options]

	Options:
	  --help      Print usage and exit.
	  --bitFile   Path to firmware bitfile.
	  --ipAddr    IP address of unit to program (optional).
	  --progMode  (optional) Where to program firmware DRAM/EPROM/BACKUP (optional).
	  --logLevel  (optional) Logging level level to print (optional; default=2/INFO).

	Examples:
	  program --bitFile=/path/to/bitfile (all other options will be prompted for)
	  program --bitFile=/path/to/bitfile --ipAddr=192.168.2.2 --progMode=DRAM

The executable will prompt the user for ip address and programming
mode. The APS2 can boot from multiple locations: volatile DRAM;
non-volatile flash or if all else fails a master backup in flash. The
DRAM storage takes only a few seconds to program and is used for
temporary booting for testing purposes. It will be lost on a power
cycle. Once you are happy there are no issues with the new bitfile you
can program it to the flash memory so the module will boot from the
new firmware on a power cycle. This process involves erasing, writing
and verifying and takes several minutes. The backup firmware should
only be programmed in the rare case BBN releases an update to the
backup image.  Should something catastrophic happen during programming
(unplugging the ethernet cable) the module may drop to an extremely
primitive firmware that will flash L1 and L2 in an alternating
fashion.  Should this happen contact BBN for assistance.

.. rubric:: Footnotes

.. [#f1] The APS2 use static self-assigned IP addresses and should ideally be
   behind the same router as the control computer.

.. [#f2] There is the potential for conflicts with previously loaded DLL's
   that are incompatible versions.  For example, if you have loaded another
   driver into Matlab that was built with a different version of MinGW-w64
   or trying to load libaps2 into Julia which was built with a different
   version of MinGW-w64. There is no easy solution to this problem on the
   Windows platform. Please contact BBN if you run into this situation.
