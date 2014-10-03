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

run_DAC_BIST(aps::APS2, dac, testVec::Vector{Int16}) = 
	ccall((:run_DAC_BIST, "libaps2"), Cint, (Ptr{Cchar}, Cint, Ptr{Int16}, Cuint), aps.serial, dac, testVec, length(testVec))