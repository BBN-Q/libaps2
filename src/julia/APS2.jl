type APS2
	serial::ASCIIString
end

push!(DL_LOAD_PATH, joinpath(dirname(@__FILE__), "../../build/"))

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
	results = Array(Uint32, 8)
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
		FPGAPhase1 = bistVals[3] == bistVals[1] ? "pass" : "fail"
		FPGAPhase2 = bistVals[4] == bistVals[2] ? "pass" : "fail"
		LVDSPhase1 = bistVals[5] == bistVals[1] ? "pass" : "fail"
		LVDSPhase2 = bistVals[6] == bistVals[2] ? "pass" : "fail"
		SYNCPhase1 = bistVals[7] == bistVals[7] ? "pass" : "fail"
		SYNCPhase2 = bistVals[8] == bistVals[8] ? "pass" : "fail"
		println("FPGA: $FPGAPhase1 / $FPGAPhase2; LVDS: $LVDSPhase1 / $LVDSPhase2; SYNC: $SYNCPhase1 / $SYNCPhase2")
	end

	return passedVec
end

set_DAC_SD(aps::APS2, dac, sd) = ccall((:set_DAC_SD, "libaps2"), Cint, (Ptr{Cchar}, Cint, Cchar), aps.serial, dac, sd)

function create_test_waveform()
	#Create a test pattern with the follow pattens separated by 10ns of zero:
	# * a full scale ramp
	# * gaussians with 256/128/64/32/16/8 points
	# * +/- square pulses with 256/128/64/32/16/8 points

	wf = int16([-8192:8191])
	for ex = 8:-1:3
		xpts = linspace(-2,2, 2^ex)
		vertShift = exp(-(2 + xpts[2] - xpts[1])^2/2)
		gaussWF = int16((8191/(1-vertShift)) * (exp(-xpts.^2/2) - vertShift))
		wf = cat(1, wf, zeros(Int16, 12), gaussWF)
	end

	for ex = 8:-1:3
		wf = cat(1, wf, zeros(Int16, 12), fill(int16(8191), int(2^ex/2)), fill(int16(-8192), int(2^ex/2)))
	end
	return wf
end
