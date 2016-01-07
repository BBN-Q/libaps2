#Simple testing of new TCP comms
using Base.Test

ip_addr = ip"192.168.2.3"

#UDP responder
udp_sock = UDPSocket();
bind(udp_sock, ip"0.0.0.0", 0xbb4f)
send(udp_sock, ip"192.168.2.3", 0xbb4f, [0x01])
resp = bytestring(recv(udp_sock))
@test resp == "I am an APS2"
close(udp_sock)

tcp_sock = connect(ip_addr, 0xbb4e)

#memory write
write_vals = rand(UInt32, 1000)
#first half as no-ack
datagram = [0x000001f4; 0xc0000000; write_vals[1:500]]
write(tcp_sock, map(hton, datagram))
#second half with ack
datagram = [0x400001f4; 0xc00007d0; write_vals[501:end]]
write(tcp_sock, map(hton, datagram))

result = ntoh(read(tcp_sock, UInt32))

@test result == 0x408001f4

#Send a read request
datagram = UInt32[0x100003e8, 0xc0000000]
write(tcp_sock, map(hton, datagram))
#check response datagram header
resp_header = map(ntoh, read(tcp_sock, UInt32, 2))
@test resp_header == UInt32[0x100003e8; 0xc0000000]

read_vals = map(ntoh, read(tcp_sock, UInt32, 1000))
@test read_vals == write_vals

# flash read the IP and MAC address
read_req = UInt32[0x32000004, 0x00ff0000]
write(tcp_sock, map(hton, read_req))
result = map(ntoh, read(tcp_sock, UInt32, 5))
@test result == UInt32[0x92000004; 0x4651db00; 0x002e0000; 0xc0a80202; 0x00000000]


close(tcp_sock)
