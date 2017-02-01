
PRJ = stripe433
OBJS = stripe433.o ws2812b.o

.c.o:
	sdcc -V --use-non-free -mpic14 -p16f877a -c $<

stripe_out.hex: $(OBJS)
	#gplink -m -s $(PRJ).lkr -o $(PRJ).hex $(OBJS) libsdcc.lib
	/usr/local/bin/gplink -s stripe433.lkr -I/usr/local/bin/../share/sdcc/lib/pic14 -I/usr/local/Cellar/sdcc/3.6.0/share/sdcc/lib/pic14 -I/usr/local/bin/../share/sdcc/non-free/lib/pic14 -I/usr/local/Cellar/sdcc/3.6.0/share/sdcc/non-free/lib/pic14  -I/usr/local/bin/../share/sdcc/lib/pic14 -I/usr/local/Cellar/sdcc/3.6.0/share/sdcc/lib/pic14 -I/usr/local/bin/../share/sdcc/non-free/lib/pic14 -I/usr/local/Cellar/sdcc/3.6.0/share/sdcc/non-free/lib/pic14   -w -r -o stripe_out   stripe433.o ws2812b.o  libsdcc.lib pic16f877a.lib -r

bootloader_pre_18f4520.asm: bootloader.c
	sdcc --use-non-free -mpic16 -p18f4520 -DMAX_FLASH_WRITE_ADDR=0x b bootloader.c -V -S

bootloader_pre_16f877a.asm: bootloader.c
	sdcc --use-non-free -mpic14 -p16f877a -DMAX_FLASH_WRITE_ADDR=0x1b bootloader.c -V -S

bootloader_2.hex: bootloader_2.asm
	gpasm -o bootloader_2.o -c bootloader_2.asm
	/usr/local/bin/gplink -s stripe433.lkr -I/usr/local/bin/../share/sdcc/lib/pic14 -I/usr/local/Cellar/sdcc/3.6.0/share/sdcc/lib/pic14 -I/usr/local/bin/../share/sdcc/non-free/lib/pic14 -I/usr/local/Cellar/sdcc/3.6.0/share/sdcc/non-free/lib/pic14  -I/usr/local/bin/../share/sdcc/lib/pic14 -I/usr/local/Cellar/sdcc/3.6.0/share/sdcc/lib/pic14 -I/usr/local/bin/../share/sdcc/non-free/lib/pic14 -I/usr/local/Cellar/sdcc/3.6.0/share/sdcc/non-free/lib/pic14   -w -r -o bootloader_2 bootloader_2.o  libsdcc.lib pic16f877a.lib -c 


clean:
	rm *.o

program: stripe_out.hex
	scp stripe_out.hex wellernas:/tmp/
	ssh wellernas "/opt/flaskdoor/rs485.py --bootloader --program /tmp/stripe_out.hex --program-run"
