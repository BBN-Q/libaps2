Pulse Sequencing
================

Background
----------

Sequencing typically requires construction of a sequence table which defines
the order in which waveforms are played along with control-flow instructions.
In existing commercial AWGs, these control-flow instructions are limited to
repeated waveforms (basic looping) and non-conditional goto statements to jump
to other sections of the waveform table. More recently, equipment
manufacturers have added rudimentary conditional elements through *event
triggers* to conditionally jump to an address in the waveform table upon
receipt of an external trigger. This capability introduces *branches*
into the sequence table. Equipment manufacturers have also expanded the memory
re-use concept by *subsequences* which allow for jumping to sections of
the waveform table and then returning to the jump point in a manner similar to
a subroutine or function call in a standard programming language.

These recent additions expand the number of sequence flow graphs that can be
built with these primitives. However, they are still limited in several ways.
First, previous implementations have not allowed arbitrary combinations of
control-flow constructs. For instance, it may be diserable to have *all*
control-flow instructions be conditional, so that, for example, subsequence
execution could depend on external input. Or it may be desireable to construct
recursive control-flow structures, i.e. nested subsequences should be
possible. Second, *event triggers* are not sufficiently expressive to
choose between branches of more than two paths. With wider, multi-bit input
interfaces, one can construct higher-order branches (e.g. with a 2-bit input
you could have four choices).

In short, rather than having an instruction set that allows for a limited
number of control-flow graphs, we wish to expand the instruction set of AWGs
to allow for fully arbitrary control flow structures.

Sequencer Design
----------------

.. figure:: images/sequencer-block-diagram.*
	:figwidth: 60%

	**Sequencer block diagram** The APS has a single instruction decoder that
	dispatches instructions to multiple waveform and marker engines.

To achieve arbitrary control flow in an AWG, we adopt modern CPU design
practice and separate the functions of control flow from instruction
execution. An instruction decoder and scheduler broadcasts waveform and marker
instructions to independent waveform and marker engines, while using control-flow
instructions to decide which instruction to read next. This asynchronous design
allows for efficient representation of common AWG sequences. However, it also
requires reintroducing some sense of synchronization across the indepent
waveform and marker engines. This is achieved in two ways: SYNC instructions
and write flags. The SYNC instruction ensures that all execution engines have 
finished any queued instructions before allowing sequencer execution to 
continue. The write flag allows a sequence of waveform and marker instructions
to be written to their respective execution engines simultaneously. A waveform
or marker instruction with its write flag low will be queued for its corresponding
execution engine, but instruction delivery is delayed until the decoder receives
an instruction with the write flag high.

