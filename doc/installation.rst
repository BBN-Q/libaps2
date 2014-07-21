Installation Guide
==================

Hardware
--------

The BBN APS2 system contains one or more analog output modules and an advanced
trigger module in an enclosure that supplies power to each module. Up to 9
analog modules may be installed in a single 19" 8U enclosure, providing 18
analog output channels. Installing a new module only requires plugging it into
a free slot on a powered-off system, then connecting SATA cables from the new
APS module to the trigger module.

Each module in an APS2 system acts as an independent network endpoint. The
modules communicate with a host computer via a UDP interface over 1GigE. To
ensure high-bandwidth throughput, it is important that the APS2 and the host
computer not be separated by too many network hops. If possible, locate the
host and APS2 on a common switch or router.

While the APS can run in a standalone configuration, we recommend running with
a 10~MHz (+7 dBm) external reference. This reference must be supplied at the
corresponding front panel inputs before powering on the system. Multiple
devices can be synchronized by supplying an external trigger that is phase
locked to this same reference.

Software
--------

In order to control the APS2, BBN provides a Windows shared library. You may
download this driver from our APS2 source code repository
(http://github.com/BBN-Q/libaps2). Click on 'releases' to find the latest
binaries. We provide MATLAB wrappers to this library, but the APS2 may be used
with any software that can call a C-API DLL. To use the MATLAB driver, simply
add the path of the unzipped driver to your MATLAB path.

Once the APS2 has been powered on, the user must assign static IP addresses to
each module. By default, the APS2 modules will have addresses on the
192.168.5.X subnet. The ``enumerate()`` method in libaps2 may be used to
find APS2 modules on your current subnet. Another method, ``set_ip_addr()``
may be used to program new IP addresses. Since the APS2 modules will respond
to any valid packet on its port, we recommend placing the APS2 system on a
private network, or behind a firewall.

The BBN APS2 has advanced sequencing capabilities. Fully taking advantage of
these capabilities may require use of higher-level languages which can be
'compiled down' into sequence instructions. BBN has produced one such
language, called Quantum Gate Language (QGL), as part of the PyQLab suite
(http://github.com/BBN-Q/PyQLab) [#f1]_.  We encourage end-users to explore using
QGL for creating pulse sequences. You may also find the sequence file export
code to be a useful template when developing your own libraries. A detailed
instruction format specification can be found in the :ref:`instruction-spec`
section.

.. rubric:: Footnotes

.. [#f1] APS2 compatibility is currently only available on the experimental
         'control-flow' branch.
