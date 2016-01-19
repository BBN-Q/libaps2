#Simple testing of new TCP comms
using Base.Test

function write_memory(sock::TCPSocket, addr::UInt32, data::Vector{UInt32})
    #Have 16 bits for count in datagram so can write up to (1 << 16) - 1 words
    idx = 1
    max_ct = (1 << 16) - 1
    while (idx < length(data))
        ct_left = length(data) - idx + 1
        last = (ct_left < max_ct) ? true : false
        ct = last ? ct_left : max_ct
        cmd = last ? 0x40000000 : 0x0
        datagram = UInt32[cmd + ct; addr; data[idx:idx+ct-1]]
        write(sock, map(hton, datagram))

        if last
            result = ntoh(read(tcp_sock, UInt32))
            @test result == cmd + 0x00800000 + UInt32(ct)
        end

        idx += ct
        addr += ct*4
    end

end

ip_addr = ip"192.168.2.2"

#UDP responder
udp_sock = UDPSocket();
bind(udp_sock, ip"0.0.0.0", 0xbb4f)
send(udp_sock, ip"192.168.2.2", 0xbb4f, [0x01])
resp = bytestring(recv(udp_sock))
@test resp == "I am an APS2"
close(udp_sock)

tcp_sock = connect(ip_addr, 0xbb4e)

#Read uptime
datagram = UInt32[0x10000002, 0x44a00050]
write(tcp_sock, map(hton, datagram))
#check response datagram header
resp_header = map(ntoh, read(tcp_sock, UInt32, 2))
@test resp_header == UInt32[0x10000002; 0x44a00050]

uptime_array = map(ntoh,read(tcp_sock, UInt32, 2))
uptime = uptime_array[1] + 1e-9*uptime_array[2]
println("Uptime $uptime")

#memory write
#first BRAMs with short message
write_vals = rand(UInt32, 1 << 10)
write_memory(tcp_sock, 0xc2000000, write_vals)

#Send a read request
datagram = UInt32[0x100003e8, 0xc2000000]
write(tcp_sock, map(hton, datagram))
#check response datagram header
resp_header = map(ntoh, read(tcp_sock, UInt32, 2))
@test resp_header == UInt32[0x100003e8; 0xc2000000]

read_vals = map(ntoh, read(tcp_sock, UInt32, 1000))
@test read_vals == write_vals[1:1000]


#Now SDRAM
write_vals = rand(UInt32, 1 << 18)
tic()
write_memory(tcp_sock, 0x20000000, write_vals)
write_speed = sizeof(eltype(write_vals)) * length(write_vals) / toq()
println("Write speed: $(write_speed/1e6) MB/s")

#Send a read request
datagram = UInt32[0x100003e8, 0x20000000]
write(tcp_sock, map(hton, datagram))
#check response datagram header
resp_header = map(ntoh, read(tcp_sock, UInt32, 2))
@test resp_header == UInt32[0x100003e8; 0x20000000]

read_vals = map(ntoh, read(tcp_sock, UInt32, 1000))
@test read_vals == write_vals[1:1000]

# flash read the IP and MAC address
read_req = UInt32[0x32000004, 0x00ff0000]
write(tcp_sock, map(hton, read_req))
result = map(ntoh, read(tcp_sock, UInt32, 5))
@test result == UInt32[0x92000004; 0x4651db00; 0x002e0000; 0xc0a80202; 0x00000000]


close(tcp_sock)
