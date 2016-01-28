#Simple testing of new TCP comms
using Base.Test

function write_memory(sock::TCPSocket, addr::UInt32, data::Vector{UInt32})
    #Have 16 bits for count in datagram so can write up to (1 << 16) - 1 words
    #but need 128bit alignment for SDRAM so 0xfffc
    idx = 1
    max_ct = 0xfffc
    datagrams_written = 0
    init_addr = addr
    while (idx < length(data))
        ct_left = length(data) - idx + 1
        ct = (ct_left < max_ct) ? ct_left : max_ct
        cmd = 0x80000000
        datagram = UInt32[cmd + ct; addr; data[idx:idx+ct-1]]
        write(sock, map(hton, datagram))
        datagrams_written += 1
        idx += ct
        addr += ct*4
    end

    #Check the results
    results = map(ntoh,read(tcp_sock, UInt32, 2*datagrams_written))
    addr = init_addr
    for ct = 1:datagrams_written
        words_written = (ct == datagrams_written) ? length(data)-((datagrams_written-1)*0xfffc) : 0xfffc
        @test results[2*ct-1] == 0x80800000 + UInt32(words_written)
        @test results[2*ct] == UInt32(addr)
        addr += 4*words_written
    end

end

ip_addr = ip"192.168.2.2"

#UDP responder
udp_sock = UDPSocket();
bind(udp_sock, ip"0.0.0.0", 0xbb4f)
send(udp_sock, ip"192.168.2.2", 0xbb4f, [0x01])
resp = bytestring(recv(udp_sock))
@test resp == "I am an APS2"

#TCP reset
# send(udp_sock, ip"192.168.2.2", 0xbb4f, [0x02])

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
# first BRAMs with short message
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

idx = 0
while ( (idx+1000) < length(write_vals))  #Send a read request
  datagram = UInt32[0x100003e8, 0x20000000+(idx*4)]
  write(tcp_sock, map(hton, datagram))
  #check response datagram header
  resp_header = map(ntoh, read(tcp_sock, UInt32, 2))
  @test resp_header == UInt32[0x100003e8; 0x20000000+(idx*4)]
  read_vals = map(ntoh, read(tcp_sock, UInt32, 1000))
  @test read_vals == write_vals[idx+1:idx+1000]
  idx += 1000
end

#Test status register request to wrapped ApsMsgProc
read_req = UInt32[0x37000010, 0x00000000]
write(tcp_sock, map(hton, read_req))
result = map(ntoh, read(tcp_sock, UInt32, 17))
if result[4] == 0xdddddddd
  @test result[1:5] == UInt32[0x97000010; 0x00000a20 ; 0xbadda555; 0xdddddddd; 0x0ddba111]
else
  @test result[1:5] == UInt32[0x97000010; 0x00000a20 ; 0xbadda555; 0xeeeeeeee; 0x0ddba111]
end

#Test configuration DRAM read/write
num_words = 1 << 8
write_vals = rand(UInt32, num_words)
cmd = 0x25000000
datagram = UInt32[cmd+num_words; 0; write_vals]
write(tcp_sock, map(hton, datagram))
write_resp = ntoh(read(tcp_sock, UInt32))
@test write_resp == 0x85000000
read_req = UInt32[0x35000000 + num_words, 0x0]
write(tcp_sock, map(hton, read_req))
resp_header = ntoh(read(tcp_sock, UInt32))
@test resp_header == UInt32(0x95000000 + num_words)
read_vals = map(ntoh, read(tcp_sock, UInt32, num_words))
@test read_vals == write_vals

#
# #Test flash read
# read_req = UInt32[0x32000004, 0x00ff0000]
# write(tcp_sock, map(hton, read_req))
# result = map(ntoh, read(tcp_sock, UInt32, 5))
# @test result == UInt32[0x92000004; 0x4651db00; 0x002e0000; 0xc0a80202; 0x00000000]

close(tcp_sock)
