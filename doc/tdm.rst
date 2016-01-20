Trigger Distribution Module
==========================

The trigger distribution module (TDM) provides a flexible mechanism for
distributing triggers and pulse sequence steering information across an APS2
crate. Since we expect our users to have diverse requirements for distributing
steering information, we have decided to deliver the TDM as a reconfigurable
device with a basic firmware that will satisfy certain needs.

Base Functionality
------------------

The base firmware delivered with the TDM will distribute signals captured on its
front-panel interface to all APS2 modules connected by SATA cables. Port T8 is
used as a 'valid' signal to indicate that data is ready to capture on T1-T7. On
the rising of a signal on T8, the signals on T1-T7 are captured into a 7-bit
trigger word which is immediately distributed across the APS2 crate. Each of the
trigger ports T1-T8 drives a comparator with a programmable threshold. The base
firmware fixes this threshold at 0.8V.


Building Custom Firmware
------------------------

The TDM firmware source may be found here:
https://github.com/BBN-Q/APS2-TDM

To get starting creating your own APS2 TDM firmware, you need a copy of Xilinx
Vivado 2015.1 (the free Webpack edition is sufficient). Note that there are more
recent versions of Vivado, but that the firmware source refers to specific
versions of Xilinx IP cores, and it may be necessary to convert these for use
with later Vivado versions. The APS2 TDM firmware relies upon one not-free
Xilinx IP Core, the Tri-Mode Ethernet Media Access Controller, or TEMAC:
http://www.xilinx.com/products/intellectual-property/temac.html

You can build the firmware without buying a TEMAC license, but the controller
will stop functioning ~8 hours after loading the image, and you will be forced
to power cycle the APS2 TDM to continue. We recommend purchasing a project
license for the Xilinx TEMAC if you plan to build your own TDM firmware.

Refer to the README in the APS2-TDM firmware source for instructions on creating
a Vivado project.
