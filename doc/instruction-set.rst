APS2 Instruction Set
====================

Abstract Instructions
---------------------

Arbitrary control flow requires three concepts: sequences, loops (repetition)
and conditional execution. We add to this set the concept of subroutines
because of their value in structured programming and memory re-use.

The BBN APS2 has two memories: a *waveform* memory and an
*instruction* memory. These memories are accessed via an intermediate
caching mechanism to provide low-latency access to nearby sections of memory.
In addition, the APS has four other resources available for managing control
flow: a repeat counter, an instruction counter, a stack, and a comparison
register. The instruction counter points to the current address in instruction
memory. The APS2 sequence controller reads and executes operations in
instruction memory at the instruction pointer. Unless the instruction
specifies otherwise, by default the controller increments the instruction
pointer upon executing each instruction. The available *abstract*
instructions are:

| SYNC
| WAIT
| WAVEFORM *address* *N*
| MARKER *channel* *state* *N*
| LOAD_REPEAT *count*
| REPEAT *address*
| CMP *operator* *N*
| GOTO *address*
| CALL *address*
| RETURN
| MODULATOR
| PREFETCH *address*
| NOOP

We explain each of these instructions below.

SYNC---Halts instruction dispatch until waveform and marker engines have
executed all queued instructions. The write flag should be set to broadcast
this instruction.

WAIT---Indicates that waveform and marker engines should wait for a trigger
before continuing. The write flag should be set to broadcast
this instruction.

WAVEFORM---Indicates that the APS should play back length *N* data points
starting at the given waveform memory address. An additional flag marks the
waveform as a time-amplitude variant, which outputs data at a *fixed*
address for *N* counts.

MARKER---Indicates the APS should hold marker *channel* in *state*
(0 or 1) for *N* samples.

LOAD_REPEAT---Loads the given value into the repeat counter.

REPEAT---Decrements the repeat counter. If the resulting value is greater than
zero, jumps to the given instruction address by updating the instruction
counter.

CMP---Compares the value of the comparison register to the mask *N* with any
of these operators: :math:`=, \neq, >, <`. So, (CMP :math:`\neq` 0) would be
true if the comparison register contains any value other than zero.

GOTO---Jumps to the given address by updating the instruction counter.

CALL---Pushes the current instruction and repeat counters onto the stack, then
jumps to the given address by updating the instruction counter.

MODULATOR---A modulator instruction.

RETURN---Moves (pops) the top values on the stack to the instruction and
repeat counters, jumping back to the most recent CALL instruction.

PREFETCH---Prefetches a cache line of instructions starting at address

NOOP---Null or No Operation

These instructions easily facilitate two kinds of looping: iteration and while
loops. The former is achieved through use of LOAD_REPEAT to set the value of the
repeat counter, followed by the loop body, and terminated by REPEAT to jump back
to the beginning of the loop. The latter is achieved by a conditional GOTO
followed by the loop body, where the address of the GOTO is set to the end of
the loop body.

Subroutines are implemented with the CALL and RETURN instructions. The address
of a CALL instruction can indicate the first instruction in instruction memory
of a subroutine. The subroutine may have multiple exit points, all of which
are marked by a RETURN instruction.

Conditional execution is directly supported by the GOTO, CALL, and RETURN
instructions. When these instructions are preceeded by a CMP instruction,
their execution depends on the comparison resulted. Consequently, the stated
instruction set is sufficient for arbitrary control flow.

Finally, filling the waveform cache is time consuming, requiring several hundred
microseconds. Therefore, the PREFETCH instruction allows one to schedule this
costly operation during "dead time" in an experiment, e.g. immediately prior
to instructions that wait for a trigger.

.. _instruction-spec:

Concrete Instructions
---------------------

The APS2 uses a 64-bit instruction format, divided into header (bits 63-56),
and payload (bits 55-0). The format of the payload depends on instruction op
code.

Instruction header (8-bits)
^^^^^^^^^^^^^^^^^^^^^^^^^^^

======  ===========
Bit(s)  Description
======  ===========
7-4     op code
3-2     engine select (0-3)
1       *reserved*
0       write flag
======  ===========

The op code determines the instruction type. For MARKER instructions, the
'engine select' field chooses the output channel of the instruction. The write
flag is used to indicate the final instruction in a group of WAVEFORM and
MARKER instructions to be sent simultaneously to their respective execution
engines.

Instruction op codes
^^^^^^^^^^^^^^^^^^^^

====  ===========
Code  instruction
====  ===========
0x0    WAVEFORM
0x1    MARKER
0x2    WAIT
0x3    LOAD_REPEAT
0x4    REPEAT
0x5    CMP
0x6    GOTO
0x7    CALL
0x8    RETURN
0x9    SYNC
0xA    MODULATOR
0xB    LOAD_CMP
0xC    PREFETCH
====  ===========

Instruction payload (56-bits)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The 56-bit payload formats for the various instruction op codes are described
below.

WAVEFORM
^^^^^^^^

======  ===========
Bit(s)  Description
======  ===========
47-46   op code (0 = play waveform, 1 = wait for trig, 2 = wait for sync, 3 = prefetch)
45      T/A pair flag
44-24   count
23-0    address
======  ===========

The top two bits of the WAVEFORM payload are an op code for the waveform engine.
A PLAY_WAVEFORM op code causes the waveform engine to play the waveform starting
at *address* for *count* quad-samples. When the time/amplitude pair flag is set,
the waveform engine will create a constant- amplitude waveform by holding the
analog output at the value given at *address* for *count* quad-samples. The
WAIT_FOR_TRIG and WAIT_FOR_SYNC op codes direct the waveform engine to pause
until receipt of an input trigger or a sequence SYNC input, respectively. The
PREFETCH op code causes the waveform cache to prefetch 64k samples from
*addresss* into the pending waveform cache bank.

MARKER
^^^^^^

======  ===========
Bit(s)  Description
======  ===========
47-46   op code (0 = play marker, 1 = wait for trig, 2 = wait for sync)
45-37   *reserved*
36-33   transition word
32      state
31-0    count (firmwave versions 2.5-2.33 support only 20 bit count)
======  ===========

The top two bits of the MARKER payload are an op code for the marker engine. A
PLAY_MARKER op code causes the marker engine to hold the marker output in
value *state* for *count* quad-samples. When the count reaches zero,
the marker engine will output the 4-bit transition word. One use of this
transition word is to achieve single- sample resolution on a low-to-high or
high-to-low transition of the marker output. The WAIT_FOR_TRIG and
WAIT_FOR_SYNC op codes function identically to the WAVEFORM op codes.

CMP
^^^

======  ===========
Bit(s)  Description
======  ===========
9-8     cmp code (0 = equal, 1 = not equal, 2 = greater than, 3 = less than)
7-0     mask
======  ===========

The CMP operation compares the current value of the 8-bit comparison register
to *mask* using the operator given by the *cmp code*. The result of this
comparison effects conditional execution of following GOTO, CALL, and RETURN
instructions.

GOTO, CALL, and REPEAT
^^^^^^^^^^^^^^^^^^^^^^

======  ===========
Bit(s)  Description
======  ===========
25-0    address
======  ===========

Jumps to *address*. For GOTO and CALL, the jump may be conditional if proceeded
by a CMP instruction. For REPEAT, the jump is conditioned on the repeat counter.

LOAD_REPEAT
^^^^^^^^^^^

======  ============
Bit(s)  Description
======  ============
15-0    repeat count
======  ============

The *repeat count* gives the number of times a section of code should be
repeated, i.e. to execute a sequence *N* times, one uses a repeat count of *N-1*.

PREFETCH
^^^^^^^^

======  ===========
Bit(s)  Description
======  ===========
25-0    address
======  ===========

Prefetches a cache-line (128 instructions) starting at *address* into the
subroutine cache.

WAIT and SYNC
^^^^^^^^^^^^^

======  ===========
Bit(s)  Description
======  ===========
47-46   op code (0 = play waveform/marker, 1 = wait for trig, 2 = wait for sync)
======  ===========

The payloads for the WAIT and SYNC instructions must also be valid WAVEFORM
and MARKER payloads. Therefore, in addition to indicating WAIT or SYNC in the
instruction header, the instruction type must also appear in the payload. The
write flag should be set to immediately dispatch this instruction.

RETURN
^^^^^^

This instruction ignores all payload data.

MODULATOR
^^^^^^^^^

====== =============
Bit(s)  Description
====== =============
47-45  op code
44     reserved
43-40  nco select
39-32  reserved
31-0   payload
====== =============

The modulator op codes are enumerate as follows:

0x0
	modulate using selected nco for count (payload)
0x1
	reset selected nco phase accumulator
0x2
	wait for trigger
0x3
	set selected nco phase increment (payload)
0x4
	wait for sync
0x5
	set selected nco phase offset (payload)
0x6
	reserved
0x7
	update selected nco frame (payload)

The nco select bit field gives one bit to each NCO.  In the current firmware
there are two NCO's. For example, to set the frequency of the second NCO the bit
field would read ``0010`` or to reset the phase of both NCOs it would read ``0011``.

All phase payloads are fixed point integers UQ2.28 representing portions of a
circle.  The frequency is determined with respect to the 300MHz system clock.
For example, setting a phase increment of 1/3 * 2^28 = 0x02aaaaab gives a
modulation frequency of 50MHz.  Integers greater than 2 give frequencies greater
than the Nyquist frequency of 600MHz and will be folded back in as negative
frequencies.


Example Sequences
-----------------

Ramsey
^^^^^^

To give a concrete example of construction of a standard QIP experiment in the
APS2 format, consider a Ramsey experiment consisting of two π/2-pulses
separated by a variable delay. If the waveform memory has a null-pulse at
offset 0x00 and a 16-sample π/2-pulse at offset 0x01, then the Ramsey
sequence might in abstract format would look like::

	SYNC
	WAIT
	WAVEFORM 0x01 4
	WAVEFORM T/A 0x00 10
	WAVEFORM 0x01 4
	SYNC
	WAIT
	WAVEFORM 0x01 4
	WAVEFORM T/A 0x00 20
	WAVEFORM 0x01 4
	SYNC
	WAIT
	WAVEFORM 0x01 4
	WAVEFORM T/A 0x00 30
	WAVEFORM 0x01 4
	    .
	    .
	    .
	GOTO 0x00

The {SYNC, WAIT} sequences demarcate separate Ramsey delay
experiments, where the SYNC command ensures that there is no residual
data in any execution engine before continuing, and the WAIT command
indicates to wait for a trigger. The GOTO command at the end of the
sequence is crucial to ensure that the instruction decoder doesn't "fall
off" into garbage data at the end of instruction memory.

CPMG
^^^^

The Carr-Purcell-Meiboom-Gill pulse sequence uses a repeated delay-π-delay
sequence to refocus spins in a fluctuating environment. The π pulse is offset by
90 degrees to the intial π/2 pulse that creates the coherence and even numbers
of π pulses are prefered for robustness. We can pull in many elements of
arbitrary flow control to compactly describe this sequence. We will use a
waveform library with three entries: a null pulse at offset 0x00, a 16-sample
π/2-pulse at offset 0x01, and a 16-sample π-pulse at offset 0x05. Note that
offsets are also written in terms of quad-samples, so the memory address range
of the first π/2 pulse is [0x01,0x04]. The Hahn echo delay-π-delay is considered
a subroutine. To ensure even multiples a CPMG subroutine then loops over the
Hahn echo twice. The two subroutines are placed at the cache-line aligned
address 1024 Then a CPMG sequence with 2, 4, 8, 16 ... loops is::

	SYNC
	WAIT
	WAVEFORM 0x01 4 # first 90
	LOAD_REPEAT 0
	CALL 1024 # call the CPMG subroutine
	REPEAT 3
	LOAD_REPEAT 1
	CALL 1024 # call the CPMG subroutine
	REPEAT 6
	LOAD_REPEAT 3
	CALL 1024 # call the CPMG subroutine
	REPEAT 6
	LOAD_REPEAT 7
	CALL 1024 # call the CPMG subroutine
	REPEAT 9
			.
			.
			.
	WAVEFORM 0x01 4 # final 90
	GOTO 0x00
  NOOP
	NOOP
	NOOP
			.
			.
			.
	# pad with NOOP's to address 1024
	# start CPMG subroutine
	LOAD_REPEAT 1
	CALL 1028
	REPEAT 1024
	RETURN
	# start Hahn echo subroutine
	WAVEFORM T/A 0x00 25 # delay
	WAVEFORM 0x05 4 # π pulse
	WAVEFORM T/A 0x00 25 #delay
	RETURN

Active Qubit Reset
^^^^^^^^^^^^^^^^^^

Here we dynamically steer the sequence in response to a qubit measurment in
order to actively drive the qubit to the ground state::

	GOTO 0x06 # jump over 'Reset' method definition
	# start of 'Reset' method
	WAIT # wait for qubit measurement data to arrive
	CMP = 0 # if the qubit is in the ground state, return
	RETURN
	# otherwise, do a pi pulse
	WAVEFORM 0x05 4
	GOTO 0x01 # go back to the beginning of 'Reset'
	# end of 'Reset' method
	SYNC
	CALL 0x01 # call 'Reset'
	# qubit is reset, do something...
	    .
	    .
	    .
	GOTO 0x00

In this example, we define a 'Reset' method for flipping the qubit state if it
is not currently in the ground state. The method is defined in instructions
1-5 of the instruction table. We preceed the method definition with a GOTO
command to unconditionally jump over the method definition. The structure of
the 'Reset' method is a while loop: it only exits when the comparison register
is equal to zero. We assume that this register's value is updated to the
current qubit state on every input trigger.
