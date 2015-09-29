module APSInstructions

using HDF5, Compat

export APSInstr, wf_entry, flow_call, flow_return, flow_repeat, load_repeat, sync, wait_trig, flow_loadcmp, flow_cmp, flow_goto
# write_file, write_hdf5_file

import Base.show

type APSInstr
	data::Vector{UInt16}
	name::Symbol
	target::Symbol
end

function __init__()
	global PRINTSHORT = false
	global const emptySymbol = symbol("")
	global const OPCODES = @compat Dict(
		0x0 => :WFM,
		0x1 => :MARKER,
		0x2 => :WAIT,
		0x3 => :LOAD_REPEAT,
		0x4 => :REPEAT,
		0x5 => :CMP,
		0x6 => :GOTO,
		0x7 => :CALL,
		0x8 => :RETURN,
		0x9 => :SYNC,
		0xA => :PREFETCH,
		0xB => :LOAD_CMP)
end

APSInstr(data::Vector{UInt16}) = APSInstr(data, emptySymbol, emptySymbol)
APSInstr(data::Vector{UInt16}, name::Symbol) = APSInstr(data, name, emptySymbol)
header(instr::APSInstr) = (instr.data[1] >> 8) % UInt8
opcode(instr::APSInstr) = header(instr) >> 4

function show(io::IO, e::APSInstr)
	PRINTSHORT || print(io, string(OPCODES[opcode(e)], ": "))
	for b in e.data
		print(io, hex(b, 4))
	end
end

function write_file(filename, seq)
	global PRINTSHORT = true
	open(filename, "w") do f
		for s = seq
			println(f, s)
		end
	end
	PRINTSHORT = false
end

function write_hdf5_file(filename, seq, wfA, wfB)

	if isfile(filename)
		rm(filename)
	end

	#Stack the instruction data
	instrs = UInt64[]
	for s in seq
		r = UInt64(0)
		for ct in 1:4
			r += UInt64(s.data[ct]) << 16*(4-ct)
		end
		push!(instrs, r)
	end

	#write it to the h5 file
	h5write(filename, "chan_1/instructions", instrs)

	#write the waveform data
	h5write(filename, "chan_1/waveforms", wfA);
	h5write(filename, "chan_2/waveforms", wfB);
end

function read_hdf5_file(filename)
	raw_instrs = h5read(filename, "chan_1/instructions")
	instrs = Array(APSInstr, length(raw_instrs))
	for ct in 1:length(raw_instrs)
		data = raw_instrs[ct]
		# unstack the data
		instrs[ct] = APSInstr(UInt16[(data >> 16*(4-i)) & 0xffff for i in 1:4])
	end
	return instrs
end

# instruction encodings
WFM = 0x0000
MARKER = 0x0001
WAIT_TRIG = 0x0002
LOAD_REPEAT = 0x0003
DEC_REPEAT = 0x0004
CMP = 0x0005
GOTO = 0x0006
CALL = 0x0007
RET = 0x0008
SYNC = 0x0009
# PREFETCH = 0x000A
LOAD_CMP = 0x000B

#The top bits of the 48 bit instruction payload set the sync or wait for trigger in the playback engines
WAIT_TRIG_INSTR_WORD = 1 << 14;
SYNC_INSTR_WORD = 1 << 15;
PREFETCH_INSTR_WORD = 3 << 14;

# CMP op encodings
EQUAL = 0x0000
NOTEQUAL = 0x0001
GREATERTHAN = 0x0002
LESSTHAN = 0x0003

#Waveform payload slv is 47 downto 0
# Bits 47-46 are op-code (play wf / wait for trig / wait for sync )
# Bit 45 is the TAPair flag
# Bits 44 downto 24 is count
# Bits 23 downto 0 is addr
function wf_entry(addr, count; TAPair=false, writeData=true, label=emptySymbol)
	@assert (addr & 0xffffff) == addr "Oops! We can only handle 24 bit addresses"
	@assert (count & 0xfffff) == count "Oops! We can only handle 20 bit counts"

	APSInstr(UInt16[WFM << 12 | (writeData ? 1 : 0) << 8, (TAPair ? 1 : 0) << 13  | count >> 8 , (count & 0x00ff) << 8 | addr >> 16, addr & 0xffff], label)
end

wf_prefetch(addr) = APSInstr(UInt16[WFM << 12 | 1 << 8, PREFETCH_INSTR_WORD, addr >> 16, addr & 0xffff])

function marker_entry(target, state, count, transitionWord; writeData=true, label=emptySymbol)
	@assert (count & 0xffffffff) == count "Oops! We can only handle 32 bit trigger counts"

	APSInstr(UInt16[MARKER << 12 | (writeData ? 1 : 0) << 8 | (target & 0x3) << 10, (transitionWord & 0xf) << 1 | (state & 0x1), count >> 16, count & 0xffff], label)
end

flow_entry(cmd, jumpAddr::Integer, payload; label=emptySymbol) =
	APSInstr(UInt16[(cmd << 12), payload, jumpAddr >> 16, jumpAddr & 0xffff], label)

flow_entry(cmd, jumpAddr::Symbol, payload; label=emptySymbol) =
	APSInstr(UInt16[(cmd << 12), payload, 0x0000, 0x0000], label, jumpAddr)

function wait_trig(; label=emptySymbol)
	instr = flow_entry(WAIT_TRIG, 0, WAIT_TRIG_INSTR_WORD; label=label)
	instr.data[1] |= 1 << 8; # set broadcast bit
	return instr
end

function sync(; label=emptySymbol)
	instr = flow_entry(SYNC, 0, SYNC_INSTR_WORD; label=label)
	instr.data[1] |= 1 << 8; # set broadcast bit
	return instr
end

function load_repeat(loadValue; label=emptySymbol)
	@assert loadValue < 2^16 "Oops! We can only handle 16 bit repeat counts."
	flow_entry(LOAD_REPEAT, loadValue, 0; label=label)
end

flow_loadcmp(; label=emptySymbol) = flow_entry(LOAD_CMP, 0, 0; label=label)
flow_cmp(cmpOp, mask; label=emptySymbol) = APSInstr(UInt16[CMP << 12, 0, 0, ((cmpOp & 0x03) << 8) | (mask & 0xff)], label)
flow_repeat(target; label=emptySymbol) = flow_entry(DEC_REPEAT, target, 0; label=label)
flow_goto(target; label=emptySymbol) = flow_entry(GOTO, target, 0; label=label)
flow_call(target; label=emptySymbol) = flow_entry(CALL, target, 0; label=label)
flow_return(; label=emptySymbol) = flow_entry(RET, 0, 0; label=label)

# updates the address in a command instruction
function updateAddr!(entry, addr)
	entry.data[3] = addr >> 16
	entry.data[4] = addr & 0xffff
	entry
end

function resolve_symbols!(seq)
	labeledEntries = filter((x) -> x[2].name != emptySymbol, collect(enumerate(seq)))
	symbolDict = [entry.name => idx-1 for (idx, entry) in labeledEntries]
	println(symbolDict)
	for entry in seq
		if entry.target != emptySymbol && haskey(symbolDict, entry.target)
			#println("Updating target of $(entry.target) to $(symbolDict[entry.target]). Before:")
			#println(entry)
			updateAddr!(entry, symbolDict[entry.target])
			#println("After:")
			#println(entry)
		end
	end
	seq
end

# # test sequences

# function simpleSeq()
# 	seq = APSInstr[]
# 	push!(seq, wf_entry(0, 63; label=:A))
# 	push!(seq, wf_entry(0, 7; label=:B))
# 	push!(seq, wf_entry(8, 7; label=:C))
# 	push!(seq, wf_entry(16, 7))
# 	push!(seq, wf_entry(24, 7))
# 	push!(seq, wf_entry(32, 7))
# 	push!(seq, flow_entry(GOTO, 0, 0, 0))
# 	resolve_symbols!(seq)
# end
function simpleSeq()
	seq = APSInstr[
		sync(),
		load_repeat(1),
		wf_entry(0x1A, 3; label=:A),
		flow_loadcmp(),
		flow_cmp(EQUAL, 0x01),
		flow_call(:C),
		wf_entry(0x2A, 3),
		flow_repeat(:A),
		load_repeat(2),
		wf_entry(0x3A, 3; label=:B),
		wf_entry(0x4A, 4; label=:C),
		wf_entry(0x5A, 4),
		wf_entry(0x6A, 3),
		flow_repeat(:B),
		wf_entry(0x00, 3; label=:D),
		flow_loadcmp(),
		flow_cmp(EQUAL, 0x02),
		flow_return(),
		flow_goto(:D)
	]
	resolve_symbols!(seq)
end

function ramsey()
	seq = APSInstr[]
	for delay in 10:10:100
		push!(seq, sync())
		push!(seq, wait_trig())
		push!(seq, wf_entry(0, 9, TAPair=true, writeData=false))
		push!(seq, marker_entry(0, 0, 8, 0xC))
		push!(seq, wf_entry(13, 11))
		push!(seq, marker_entry(0, 1, 11, 0x3))
		push!(seq, wf_entry(0, delay, TAPair=true))
		push!(seq, marker_entry(0, 0, delay, 0xC))
		push!(seq, wf_entry(1, 11))
		push!(seq, marker_entry(0, 1, 11, 0x3))
		push!(seq, marker_entry(0, 0, 1, 0x0))
	end
	push!(seq, flow_goto(0x00))
	resolve_symbols!(seq)
end

function cpmg()
	seq = APSInstr[]
	for (ct,rep) in enumerate(2.^[1:5])
		push!(seq, sync())
		push!(seq, wait_trig())

		#First 90
		push!(seq, wf_entry(0, 9, TAPair=true, writeData=false))
		push!(seq, marker_entry(0, 0, 8, 0xC))
		push!(seq, wf_entry(1, 11))
		push!(seq, marker_entry(0, 1, 11, 0x3))

		#CPMG block repeats
		push!(seq, load_repeat(rep-1))

		push!(seq, wf_entry(0, 0xA, TAPair=true; label=symbol("C$ct")))
		push!(seq, marker_entry(0, 0, 9, 0xC))
		push!(seq, wf_entry(25, 11))
		push!(seq, marker_entry(0, 1, 11, 0x3))
		push!(seq, wf_entry(0, 0x14, TAPair=true))
		push!(seq, marker_entry(0, 0, 0x13, 0xC))
		push!(seq, wf_entry(25, 11))
		push!(seq, marker_entry(0, 1, 11, 0x3))
		push!(seq, wf_entry(0, 0xA, TAPair=true))
		push!(seq, marker_entry(0, 0, 9, 0x0))
		push!(seq, flow_repeat(symbol("C$ct")))

		#Final 90
		push!(seq, wf_entry(1, 11))
		push!(seq, marker_entry(0, 1, 11, 0x3))
	end
	push!(seq, flow_goto(0x00))
	resolve_symbols!(seq)
end

function rabi_amp()
	seq = APSInstr[]
	for ct in 1:100
		push!(seq, sync())
		push!(seq, wait_trig())
		push!(seq, wf_entry(0, 9, TAPair=true, writeData=false))
		push!(seq, marker_entry(0, 0, 8, 0xC))
		push!(seq, wf_entry((ct-1)*256 + 1, 255))
		push!(seq, marker_entry(0, 1, 255, 0x3))
		push!(seq, wf_entry(0, 9, TAPair=true))
		push!(seq, marker_entry(0, 0, 9, 0x0))
	end
	push!(seq, flow_goto(0x00))
end	

function test_prefetch()
	seq = APSInstr[]
	offset = 0
	for offset = 0:2^15:(3*2^15)
		push!(seq, wf_prefetch(offset))
		for ct in 1:100
			push!(seq, sync())
			push!(seq, wait_trig())
			push!(seq, wf_entry(offset+0, 9, TAPair=true, writeData=false))
			push!(seq, marker_entry(0, 0, 8, 0xC))
			push!(seq, wf_entry(offset+ (ct-1)*256 + 1, 255))
			push!(seq, marker_entry(0, 1, 255, 0x3))
			push!(seq, wf_entry(offset+0, 9, TAPair=true))
			push!(seq, marker_entry(0, 0, 9, 0x0))
		end
	end
	push!(seq, flow_goto(0x00))
end

function writeTestFiles()
	seq = simpleSeq();
	write_file("simpleSeq.dat", seq)
end

end
