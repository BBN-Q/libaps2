import aps2

ips = aps2.enumerate()[1]

if ips == None:
    print("No APS2s found")
else:
    for ip in ips:
        aps = aps2.APS2()
        aps.connect(ip)
        print("FPGA temp of APS2 @ " + ip + " is: " + str(round(aps.get_fpga_temperature(), 2))+ " C")
        aps.disconnect()
