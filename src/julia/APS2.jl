type APS2
	serial::ASCIIString
end

push!(DL_LOAD_PATH, joinpath(dirname(@__FILE__), "../../build/"))

typealias APS2_STATUS Cint

const APS2_OK = 0
const APS2_ERROR_MSG_LEN = 256

function check_status(status::APS2_STATUS)
	if status != APS2_OK
		msg = Array(Uint8, APS2_ERROR_MSG_LEN)
		ccall((:get_error_msg, "libaps2"), APS2_STATUS, (APS2_STATUS, Ptr{Uint8}), status, msg) 
		error("APS2 library call failed with error: $(bytestring(msg))")
	end
end

macro aps2_getter(funcName, dataType)
	funcProto = :($funcName(aps::APS2))
	funcNameBis = string(funcName)
	funcBody = quote
				val = Array($dataType, 1)
				status = ccall(($funcNameBis, "libaps2"), APS2_STATUS, (Ptr{Uint8}, Ptr{$dataType}), aps.serial, val)
				check_status(status)
				return val[1]
			end
	Expr(:function, esc(funcProto), esc(funcBody))
end

macro aps2_setter(funcName, dataType)
	funcProto = :($funcName(aps::APS2, val::$dataType))
	funcNameBis = string(funcName)
	funcBody = quote
				status = ccall(($funcNameBis, "libaps2"), APS2_STATUS, (Ptr{Uint8}, $dataType), aps.serial, val)
				check_status(status)
			end
	Expr(:function, esc(funcProto), esc(funcBody))
end

macro aps2_call(funcName, args...)
	funcProto = :($funcName(aps::APS2))
	funcNameBis = string(funcName)
	funcBody = quote
				status = ccall(($funcNameBis, "libaps2"), APS2_STATUS, (Ptr{Uint8},), aps.serial)
				check_status(status)
			end
	for ct in 1:length(args)
		argName = symbol("arg$ct")
		push!(funcProto.args, :($argName::$(args[ct]) ) )
		push!(funcBody.args[2].args[2].args, :($argName))
	end
	Expr(:function, esc(funcProto), esc(funcBody))
end

function get_numDevices()
	numDevices = Array(Cuint,1)
	status = ccall((:get_numDevices, "libaps2"), APS2_STATUS, (Ptr{Cuint},), numDevices)
	return numDevices[1]
end

function enumerate()
	numDevices = get_numDevices()
	Cserials = Array(Ptr{Uint8}, numDevices)
	status = ccall((:get_deviceSerials, "libaps2"), APS2_STATUS, (Ptr{Ptr{Uint8}},), Cserials)
	serials = Array(ASCIIString, 0)
	for ct = 1:numDevices
		push!(serials, bytestring(Cserials[ct]))
	end
	return (numDevices, serials)
end
	
function connect!(aps::APS2, serial)
	status = ccall((:connect_APS, "libaps2"), APS2_STATUS, (Ptr{Uint8},), serial)
	check_status(status)
	aps.serial = serial
end

function disconnect!(aps::APS2) 
	status = ccall((:disconnect_APS, "libaps2"), APS2_STATUS, (Ptr{Uint8},), aps.serial)
	check_status(status)
	aps.serial = ""
end

@aps2_call init Cint
init(aps::APS2, force) = init(aps::APS2, int32(force))

@aps2_call set_logging_level Cint

@aps2_getter get_firmware_version Uint32

@aps2_call run
@aps2_call stop

@aps2_setter set_channel_offset Float32
@aps2_getter get_channel_offset Float32




# function read_memory(aps::APS2, addr, numWords)
# 	buf = Array(Uint32, numWords)
# 	ccall((:read_memory, "libaps2"), Cint, (Ptr{Cchar}, Uint32, Ptr{Uint32}, Uint32), aps.serial, addr, buf, numWords)
# 	buf
# end

# write_memory(aps::APS2, addr, data::Vector{Uint32}) =
# 	ccall((:write_memory, "libaps2"), Cint, (Ptr{Cchar}, Uint32, Ptr{Uint32}, Uint32), aps.serial, addr, data, length(data))

# set_channel_offset(aps::APS2, chan, offset) =
# 	ccall((:set_channel_offset, "libaps2"), Cint, (Ptr{Cchar}, Cint, Float32), aps.serial, chan, offset)


# function run_DAC_BIST(aps::APS2, dac, testVec::Vector{Int16})
# 	results = Array(Uint32, 8)
# 	passed = ccall((:run_DAC_BIST, "libaps2"),
# 						Cint, (Ptr{Cchar}, Cint, Ptr{Int16}, Cuint, Ptr{Uint32}),
# 						aps.serial, dac, testVec, length(testVec), results)
# 	return passed, results
# end

# function test_BIST_bits(aps::APS2, dac)

# 	passedVec = Array(Int, 14)
# 	for bit = 0:13
# 		print("Testing bit $bit: ")
# 		testVec = (int16(rand(Bool, 100)) .<< bit) * (bit == 13 ? -1 : 1)
# 		passedVec[bit+1], bistVals = run_DAC_BIST(aps, dac, testVec)
# 		FPGAPhase1 = bistVals[3] == bistVals[1] ? "pass" : "fail"
# 		FPGAPhase2 = bistVals[4] == bistVals[2] ? "pass" : "fail"
# 		LVDSPhase1 = bistVals[5] == bistVals[1] ? "pass" : "fail"
# 		LVDSPhase2 = bistVals[6] == bistVals[2] ? "pass" : "fail"
# 		SYNCPhase1 = bistVals[7] == bistVals[7] ? "pass" : "fail"
# 		SYNCPhase2 = bistVals[8] == bistVals[8] ? "pass" : "fail"
# 		println("FPGA: $FPGAPhase1 / $FPGAPhase2; LVDS: $LVDSPhase1 / $LVDSPhase2; SYNC: $SYNCPhase1 / $SYNCPhase2")
# 	end

# 	return passedVec
# end

# set_DAC_SD(aps::APS2, dac, sd) = ccall((:set_DAC_SD, "libaps2"), Cint, (Ptr{Cchar}, Cint, Cchar), aps.serial, dac, sd)

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
