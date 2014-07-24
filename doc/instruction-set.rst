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
| LOAD *count*
| REPEAT *address*
| CMP *operator* *N*
| GOTO *address*
| CALL *address*
| RETURN
| PREFETCH *address*

We explain each of these instructions below.

SYNC---Halts instruction dispatch until waveform and marker engines have
executed all queued instructions.

WAIT---Indicates that waveform and marker engines should wait for a trigger
before continuing.

WAVEFORM---Indicates that the APS should play back length *N* data points
starting at the given waveform memory address. An additional flag marks the
waveform as a time-amplitude variant, which outputs data at a *fixed*
address for *N* counts.

MARKER---Indicates the APS should hold marker *channel* in *state*
(0 or 1) for *N* samples.

LOAD---Loads the given value into the repeat counter.

REPEAT---Decrements the repeat counter. If the resulting value is greater than
zero, jumps to the given instruction address by updating the instruction
counter.

CMP---Compares the value of the comparison register to the mask *N* with any
of these operators: :math:`=, \neq, >, <`. So, (CMP :math:`\neq` 0) would be 
true if the comparison register contains any value other than zero.

GOTO---Jumps to the given address by updating the instruction counter.

CALL---Pushes the current instruction and repeat counters onto the stack, then
jumps to the given address by updating the instruction counter.

RETURN---Moves (pops) the top values on the stack to the instruction and
repeat counters, jumping back to the most recent CALL instruction.

PREFETCH---Flushes the waveform caches and refills starting at the currently
requested address.

These instructions easily facilitate two kinds of looping: iteration and while
loops. The former is achieved through use of LOAD to set the value of the
repeat counter, followed by the loop body, and terminated by REPEAT to jump
back to the beginning of the loop. The latter is achieved by a conditional
GOTO followed by the loop body, where the address of the GOTO is set to the
end of the loop body.

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
0000  WAVEFORM
0001  MARKER
0010  WAIT
0011  LOAD_REPEAT
0100  REPEAT
0101  CMP
0111  GOTO
1000  CALL
1001  RETURN
1010  SYNC
1011  PREFETCH
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
47-46   op code (0 = play waveform, 1 = wait for trig, 2 = wait for sync)
45      T/A pair flag
44-24   count
23-0    address
======  ===========

The top two bits of the WAVEFORM payload are an op code for the waveform
engine. A PLAY_WAVEFORM op code causes the waveform engine to play the
waveform starting at *address* for *count* quad-samples. When the
time/amplitude pair flag is set, the waveform engine will create a constant-
amplitude waveform by holding the analog output at the value given at
*address* for *count* quad-samples. The WAIT_FOR_TRIG and
WAIT_FOR_SYNC op codes direct the waveform engine to pause until receipt of
an input trigger or a sequence SYNC input, respectively.

MARKER
^^^^^^

======  ===========
Bit(s)  Description
======  ===========
47-46   op code (0 = play marker, 1 = wait for trig, 2 = wait for sync)
45-37   *reserved*
36-33   transition word
32      state
31-0    count
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
23-0    address
======  ===========

Refills the waveform cache starting at *address*. Sequencer execution halts 
until the cache is filled.

RETURN and SYNC
^^^^^^^^^^^^^^^^^^^^^^^^

These instructions ignore all payload data.

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
sequence to refocus spins in a fluctuating environment. For this sequence one
could use a waveform library with three entries: a null pulse at offset 0x00,
a 16-sample π/2-pulse at offset 0x01, and a 16-sample π-pulse at
offset 0x05. Note that offsets are also written in terms of quad-samples, so
the memory address range of the first π/2 pulse is [0x01,0x04]. Then a CPMG
sequence with 10 delay-π-delay blocks might be programmed as::

	SYNC
	WAIT
	WAVEFORM 0x01 4
	LOAD_REPEAT 9
	WAVEFORM T/A 0x00 25
	WAVEFORM 0x05 4
	WAVEFORM T/A 0x00 25
	REPEAT
	WAVEFORM 0x01 4
	GOTO 0x00

Note that we load a repeat count of 9 in order to loop the block 10 times.

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
