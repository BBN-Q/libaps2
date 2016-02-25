module LIBAPS2

export APS2,
	connect!,
	disconnect!,
	enumerate_APS2s,
	init,
	set_logging_level,
	get_firmware_version,
	get_fpga_temperature,
	get_uptime,
	run,
	stop,
	trigger,
	set_run_mode,
	set_trigger_source,
	get_trigger_source,
	set_trigger_interval,
	get_trigger_interval,
	set_channel_offset,
	get_channel_offset,
	set_channel_scale,
	get_channel_scale,
	set_channel_enabled,
	get_channel_enabled,
	load_waveform,
	load_sequence,
	read_memory,
	write_memory

using Compat

type APS2
	ip_addr::IPv4
end
APS2() = APS2(IPv4(0))

__init__() = push!(Libdl.DL_LOAD_PATH, joinpath(dirname(@__FILE__), "../../build/"))

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
		msg = ccall((:get_error_msg, "libaps2"), Ptr{UInt8}, (APS2_STATUS,), status)
		error("APS2 library call failed with error: $(bytestring(msg))")
	end
end

macro aps2_getter(funcName, dataType)
	funcProto = :($funcName(aps::APS2))
	funcNameBis = string(funcName)
	funcBody = quote
				val = Array($dataType, 1)
				status = ccall(($funcNameBis, "libaps2"), APS2_STATUS, (Ptr{UInt8}, Ptr{$dataType}), string(aps.ip_addr), val)
				check_status(status)
				return val[1]
			end
	Expr(:function, esc(funcProto), esc(funcBody))
end

macro aps2_setter(funcName, dataType)
	funcProto = :($funcName(aps::APS2, val::$dataType))
	funcNameBis = string(funcName)
	funcBody = quote
				status = ccall(($funcNameBis, "libaps2"), APS2_STATUS, (Ptr{UInt8}, $dataType), string(aps.ip_addr), val)
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
				status = ccall(($funcNameBis, "libaps2"), APS2_STATUS, (Ptr{UInt8},), string(aps.ip_addr), $(argNames...))
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
	Cip_addrs = Array(Ptr{UInt8}, numDevices)
	status = ccall((:get_device_IPs, "libaps2"), APS2_STATUS, (Ptr{Ptr{UInt8}},), Cip_addrs)
	ip_addrs = Array(IPv4, 0)
	for ct = 1:numDevices
		push!(ip_addrs, IPv4(bytestring(Cip_addrs[ct])))
	end
	return (numDevices, ip_addrs)
end

function connect!(aps::APS2, ip_addr::IPv4)
	if aps.ip_addr !== IPv4(0)
		warn("Disconnecting from $(aps.ip_addr) before connection to $(ip_addr)")
		disconnect!(aps)
	end
	status = ccall((:connect_APS, "libaps2"), APS2_STATUS, (Ptr{UInt8},), string(ip_addr))
	check_status(status)
	aps.ip_addr = ip_addr
end

function disconnect!(aps::APS2)
	status = ccall((:disconnect_APS, "libaps2"), APS2_STATUS, (Ptr{UInt8},), string(aps.ip_addr))
	check_status(status)
	aps.ip_addr = IPv4(0)
end

function init(aps::APS2, force=1)
	status = ccall((:init_APS, "libaps2"), APS2_STATUS, (Ptr{UInt8}, Cint), string(aps.ip_addr), Cint(force))
	check_status(status)
end

set_logging_level(level) = ccall((:set_logging_level, "libaps2"), APS2_STATUS, (Cint,), level)

@aps2_getter get_firmware_version UInt32
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

function load_waveform{T<:Integer}(aps::APS2, chan, wf::Vector{T})
	status = ccall((:set_waveform_int, "libaps2"), APS2_STATUS, (Ptr{UInt8}, Cint, Ptr{Int16}, UInt32), string(aps.ip_addr), Cint(chan-1), int16(wf), length(wf))
	check_status(status)
end

function load_waveform{T<:Real}(aps::APS2, chan, wf::Vector{T})
	status = ccall((:set_waveform_float, "libaps2"), APS2_STATUS, (Ptr{UInt8}, Cint, Ptr{Float32}, UInt32), string(aps.ip_addr), Cint(chan-1), float32(wf), length(wf))
	check_status(status)
end

function load_sequence(aps::APS2, seqfile)
	status = ccall((:load_sequence_file, "libaps2"), APS2_STATUS, (Ptr{UInt8}, Ptr{UInt8}), string(aps.ip_addr), seqfile)
	check_status(status)
end

function read_memory(aps::APS2, addr, numWords)
	buf = Array(UInt32, numWords)
	ccall((:read_memory, "libaps2"), Cint, (Ptr{Cchar}, UInt32, Ptr{UInt32}, UInt32), string(aps.ip_addr), addr, buf, numWords)
	buf
end

write_memory(aps::APS2, addr, data::Vector{UInt32}) =
	ccall((:write_memory, "libaps2"), Cint, (Ptr{Cchar}, UInt32, Ptr{UInt32}, UInt32), string(aps.ip_addr), addr, data, length(data))


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

end
