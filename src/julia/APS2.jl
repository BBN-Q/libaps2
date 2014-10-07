type APS2
	serial::ASCIIString
end

push!(DL_LOAD_PATH, "/home/cryan/Programming/Repos/libaps2/build/")

function connect!(aps::APS2, serial)
	ccall((:connect_APS, "libaps2"), Cint, (Ptr{Cchar},), serial)
	aps.serial = serial
end

disconnect(aps::APS2) = ccall((:disconnect_APS, "libaps2"), Cint, (Ptr{Cchar},), aps.serial)

init(aps::APS2) = ccall((:initAPS, "libaps2"), Cint, (Ptr{Cchar}, Cint), aps.serial, 1)

set_logging_level(level) = ccall(("set_logging_level", "libaps2"), Cint, (Cint,), level)

run(aps::APS2) = ccall((:run, "libaps2"), Cint, (Ptr{Cchar},), aps.serial)
stop(aps::APS2) = ccall((:stop, "libaps2"), Cint, (Ptr{Cchar},), aps.serial)

function read_memory(aps::APS2, addr, numWords)
	buf = Array(Uint32, numWords)
	ccall((:read_memory, "libaps2"), Cint, (Ptr{Cchar}, Uint32, Ptr{Uint32}, Uint32), aps.serial, addr, buf, numWords)
	buf
end

write_memory(aps::APS2, addr, data::Vector{Uint32}) =
	ccall((:write_memory, "libaps2"), Cint, (Ptr{Cchar}, Uint32, Ptr{Uint32}, Uint32), aps.serial, addr, data, length(data))

set_channel_offset(aps::APS2, chan, offset) =
	ccall((:set_channel_offset, "libaps2"), Cint, (Ptr{Cchar}, Cint, Float32), aps.serial, chan, offset)

get_firmware_version(aps::APS2) = ccall((:get_firmware_version, "libaps2"), Cint, (Ptr{Cchar},), aps.serial)

function run_DAC_BIST(aps::APS2, dac, testVec::Vector{Int16})
	results = Array(Uint32, 6)
	passed = ccall((:run_DAC_BIST, "libaps2"),
						Cint, (Ptr{Cchar}, Cint, Ptr{Int16}, Cuint, Ptr{Uint32}),
						aps.serial, dac, testVec, length(testVec), results)
	return passed, results
end

function test_BIST_bits(aps::APS2, dac)

	passedVec = Array(Int, 14)
	for bit = 0:13
		print("Testing bit $bit: ")
		testVec = (int16(rand(Bool, 100)) .<< bit) * (bit == 13 ? -1 : 1)
		passedVec[bit+1], bistVals = run_DAC_BIST(aps, dac, testVec)
		LVDSPhase1 = bistVals[1] == bistVals[5]
		LVDSPhase2 = bistVals[2] == bistVals[6]
		SYNCPhase1 = bistVals[3] == bistVals[1]
		SYNCPhase2 = bistVals[4] == bistVals[2]
		println("LVDS: Phase 1 = $LVDSPhase1 and Phase 2 = $LVDSPhase2; SYNC: Phase 1 = $SYNCPhase1 and Phase 2 = $SYNCPhase2")
	end

	return passedVec
end

set_DAC_SD(aps::APS2, dac, sd) = ccall((:set_DAC_SD, "libaps2"), Cint, (Ptr{Cchar}, Cint, Cchar), aps.serial, dac, sd)


