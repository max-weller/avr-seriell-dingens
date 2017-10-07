#!/usr/bin/python3
import broadlink
d=broadlink.sp2(host=(sys.argv[1], 80), mac=binascii.unhexlify(sys.argv[2]))
d.auth()
d.set_power(sys.argv[3]=='true')
