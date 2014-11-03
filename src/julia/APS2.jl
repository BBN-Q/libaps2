type APS2
	serial::ASCIIString
end

push!(DL_LOAD_PATH, joinpath(dirname(@__FILE__), "../../build/"))

typealias APS2_STATUS Cint

const APS2_OK = 0

const EXTERNAL = 0 
const INTERNAL = 1
const SOFTWARE = 2 

const RUN_SEQUENCE = 0
const TRIG_WAVEFORM = 1
const CW_WAVEFORM = 2

function check_status(status::APS2_STATUS)
	if status != APS2_OK
		msg = ccall((:get_error_msg, "libaps2"), Ptr{Uint8}, (APS2_STATUS,), status) 
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
	argNames = [symbol("arg$ct") for ct in 1:length(args)]
	for (ct,arg) in enumerate(argNames)
		push!(funcProto.args, :($arg::$(args[ct]) ) )
	end
	funcNameBis = string(funcName)
	funcBody = quote
				status = ccall(($funcNameBis, "libaps2"), APS2_STATUS, (Ptr{Uint8},), aps.serial, $(argNames...))
				check_status(status)
			end
	Expr(:function, esc(funcProto), esc(funcBody))
end

function get_numDevices()
	numDevices = Array(Cuint,1)
	status = ccall((:get_numDevices, "libaps2"), APS2_STATUS, (Ptr{Cuint},), numDevices)
	return numDevices[1]
end

function enumerate_APS2s()
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
@aps2_getter get_uptime Float64
@aps2_getter get_fpga_temperature Float64

@aps2_call run
@aps2_call stop

@aps2_setter set_run_mode Cint

@aps2_setter set_channel_offset Float32
@aps2_getter get_channel_offset Float32

@aps2_setter set_channel_scale Float32
@aps2_getter get_channel_scale Float32

@aps2_setter set_channel_enabled Cint
@aps2_getter get_channel_enabled Cint

@aps2_setter set_trigger_source Cint
@aps2_getter get_trigger_source Cint
@aps2_setter set_trigger_interval Float64
@aps2_getter get_trigger_interval Float64
@aps2_call trigger

function read_memory(aps::APS2, addr, numWords)
	buf = Array(Uint32, numWords)
	ccall((:read_memory, "libaps2"), Cint, (Ptr{Cchar}, Uint32, Ptr{Uint32}, Uint32), aps.serial, addr, buf, numWords)
	buf
end

write_memory(aps::APS2, addr, data::Vector{Uint32}) =
	ccall((:write_memory, "libaps2"), Cint, (Ptr{Cchar}, Uint32, Ptr{Uint32}, Uint32), aps.serial, addr, data, length(data))


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
