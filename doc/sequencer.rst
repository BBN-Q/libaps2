Pulse Sequencing
================

Background
----------

Sequencing typically requires construction of a sequence table which defines the
order in which waveforms are played along with control-flow instructions. In
existing commercial AWGs, these control-flow instructions are limited to
repeated waveforms (basic looping) and non-conditional goto statements to jump
to other sections of the waveform table. More recently, equipment manufacturers
have added rudimentary conditional elements through *event triggers* to
conditionally jump to an address in the waveform table upon receipt of an
external trigger. This capability introduces *branches* into the sequence table.
Equipment manufacturers have also expanded the memory re-use concept by
*subsequences* which allow for jumping to sections of the waveform table and
then returning to the jump point in a manner similar to a subroutine or function
call in a standard programming language.

These recent additions expand the number of sequence flow graphs that can be
built with these primitives. However, they are still limited in several ways.
First, previous implementations have not allowed arbitrary combinations of
control-flow constructs. For instance, it may be desirable to have *all*
control-flow instructions be conditional, so that, for example, subsequence
execution could depend on external input. Or it may be desirable to construct
recursive control-flow structures, i.e. nested subsequences should be possible.
Second, *event triggers* are not sufficiently expressive to choose between
branches of more than two paths. With wider, multi-bit input interfaces, one can
construct higher-order branches (e.g. with a 2-bit input you could have four
choices).

In short, rather than having an instruction set that allows for a limited number
of control-flow graphs, we wish to expand the instruction set of AWGs to allow
for fully arbitrary control flow structures.

Superscalar Sequencer Design
----------------------------

.. figure:: images/superscalar_architecture.*
	:figwidth: 80%

	**Sequencer block diagram** The APS has a single instruction decoder that
	dispatches instructions to multiple waveform and marker engines.

To achieve arbitrary control flow in an AWG, we adopt modern CPU design practice
and separate the functions of control flow from instruction execution. An
instruction decoder and scheduler dispatches waveform and marker instructions to
independent waveform, marker and modulation engines, while using control-flow
instructions to decide which instruction to read next. This asynchronous design
allows for efficient representation of common AWG sequences. However, it also
requires reintroducing some sense of synchronization across the independent
waveform and marker engines. This is achieved in two ways: SYNC instructions and
write flags. The SYNC instruction ensures that all execution engines have
finished any queued instructions before allowing sequencer execution to
continue. The write flag allows a sequence of waveform and marker instructions
to be written to their respective execution engines simultaneously. A waveform
or marker instruction with its write flag low will be queued for its
corresponding execution engine, but instruction delivery is delayed until the
decoder receives an instruction with the write flag high.

Cache Design
---------------

The deep DDR3 memory comes with a latency penalty, particularly when jumping to
random addresses. Rather than flowing these constraints down to the sequencer,
the APS2 attempts to hide the latency by caching instruction and waveform data
on board the sequencing FPGA. The cache follows some simple heuristics and needs
some hints in the forms of explicit *PREFETCH* commands for more sophisticated
sequencing.

Instruction Cache
~~~~~~~~~~~~~~~~~

The APS2 instruction cache is split into two parts to support two different
heuristics about how the sequence will move through the instruction stream. Both
caches operate with cache lines of 128 instructions. The first cache is a
circular buffer centered around the current instruction address that supports
the notion that the most likely direction is forward motion through the
instruction stream with potentially jumps to recently played addresses when
looping. The controller greedily prefetches additional cache lines ahead of the
current address but leaves a buffer of previously played cache lines. Function
calls do not fit into these heuristics so the second part of the cache is a
fully associative to support jumps anywhere to subroutines. The subroutine
caches are filled in round-robin fashion with explicit *PREFETCH* instructions.
The controller will ignore *8PREFETCH* instructions where the line is already in
the cache. If the sequencer asks for an address not in either cache the cache
will flush the circular buffer and fetch the aligned line into the start of the
circular buffer. The sequencer LED will blink 1 per second to indicate the cache
stall.

.. figure:: images/instruction_cache_architecture.*
	:figwidth: 80%

	**Instruction cache architecture** The instruction cache has two part: (a) a circular buffer centered around the currently playing address and (b) a fully
	associative cache for subroutine calls.

Waveform Cache
~~~~~~~~~~~~~~~~

The waveform cache can hold a maximum of 131072 (128k) samples. When the cache
is enabled, the cache preloads the first 128k samples from waveform memory. If
the waveform library is smaller than this then nothing further is needed.  To
support sequences that need deeper waveform memory, the cache is split into two
to enable bank bouncing between a playing and a loading section. Loading is
triggered by explicit waveform engine *PREFETCH* commands which will load 64k
samples at an address aligned to a 64k sample boundary. Due to the vagaries of
SDRAM accesses the time for this prefetch varies but should be approximately
200μs.  Should the waveform engines ask for an address not in the waveform cache
the cache controller will flush fetch the aligned 64k sample segment into the
first half. The sequencer LED will blink twice per second to indicated the cache
miss.

.. figure:: images/waveform_cache_architecture.*
	:figwidth: 80%

	**Waveform cache architecture** The waveform cache can be used to simply play waveforms from the first 128k samples (i) or with explicit waveform *PREFETCH* commands can be used in bank-bouncer mode (ii) and (iii).


Waveform Modulation
-----------------------

When an APS2 slice is used to drive the I and Q ports of an I/Q mixer to
amplitude and phase modulate a microwave carrier it is convenient to
bring some features typically baked into the waveforms back into the hardware. For
example, if the I/Q mixer is used to single-side-band modulate the carrier the
hardware can track the phase evolution through non deterministic delays and the
phase modulation can be updated as part of the sequence. This can then even
occur conditionally as part of the sequence control flow.

To support both the SSB modulation and dynamic frame updates, the APS2 can
dispatch instructions to the modulation engine. The modulation engine controls
the modulator which has up to four (current firmware has two) numerically
controlled oscillators (NCO).  When selected, the NCO with a phase Θ rotates the
(a,b) output pair to (a cosΘ + b sinΘ, b cosΘ - a sinΘ). Multiple NCOs are
supported to enable merging multiple logical channels at different frequencies
onto the same physical channel. The modulation engine supports the following
instructions

WAIT
	stall until a trigger is received
SYNC
	stall until a sync signal is received
RESET PHASE
	reset the phase of the selected NCO(s)
SET PHASE OFFSET
	set the phase offset of the selected NCO(s)
SET PHASE INCREMENT
	set the phase increment of the selected NCO(s) which sets an effective frequency
UPDATE FRAME
	update the frame of the selected NCO(s) by adding to the current frame
MODULATE
	apply modulation using the selected NCO for a given number of samples

All NCO phase commands are held until the the next boundary which is the end of
the currently playing `MODULATE` command or a trigger/sync signal being
received. The commands are held to allow them to occur at specific instances.
For example, we want the phase to be reset at the trigger or the Z rotation
implemented as a frame change to occur at the end of a pulse.

In addition, to account for mixer imperfections that can be inverted by
appropriate adjustments of the waveforms the APS2 applies a 2x2 correction
matrix applied to the I/Q pairs followed by a DC shift.

.. figure:: images/modulator.*
	:figwidth: 80%

	**Modulator block diagram** Block diagram of the on-board modulation
	capabilities. The NCOs phase accumulators are controlled by the modulation
	engine which can also choose which NCO to select on a pulse by pulse basis.
	The selected phase is used for a sin/cos look up table (LUT) which provides
	values for the rotation matrix.  The waveform pairs are subsequently
	processed through of arbitrary 2x2 matrix for amplitude and phase imbalance,
	channel scaling and offset.
