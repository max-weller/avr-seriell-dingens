#!/usr/bin/env python3

f=open("stripe_out.lst","r")
lines=[l for l in f]

info=[]

for idx,l in enumerate(lines):
	if "BANKSEL" in l:
		p = 0x000
		varname = l[52:].strip()

		str = lines[idx] + lines[idx+1]
		if "bsf     0x03, 0x5" in str: p |= 0b010000000
		elif "bcf     0x03, 0x5" in str: pass
		else: print("missing info for 0x5")

		if "bsf     0x03, 0x6" in str: p |= 0b100000000
		elif "bcf     0x03, 0x6" in str: pass
		else: print("missing info for 0x6")

		adr = lines[idx+2][26:30]
		p |= int(adr,16)
		#print("%03x  %s  \t\t%s"%(p,varname,lines[idx-1]))
		print("%03x  % 3d    %s"%(p,p,varname))




	

#0000ca   1683     bsf     0x03, 0x5        	BANKSEL	_recvcksum
#0000cb   1303     bcf     0x03, 0x6        
#0000cc   066c     xorwf   0x6c, 0x0        	XORWF	_recvcksum,W
