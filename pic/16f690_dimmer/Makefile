
PRJ = dimmertest
OBJS = dimmertest.o

.c.o:
	sdcc -V --use-non-free -mpic14 -p16f690 -c $<

dimmertest_out.hex: $(OBJS)
	#gplink -m -s $(PRJ).lkr -o $(PRJ).hex $(OBJS) libsdcc.lib
	/usr/local/bin/gplink  -I/usr/local/bin/../share/sdcc/lib/pic14 \
		-I/usr/local/Cellar/sdcc/3.6.0/share/sdcc/lib/pic14 \
		-I/usr/local/bin/../share/sdcc/non-free/lib/pic14 \
		-I/usr/local/Cellar/sdcc/3.6.0/share/sdcc/non-free/lib/pic14  \
		-I/usr/local/bin/../share/sdcc/lib/pic14 \
		-I/usr/local/Cellar/sdcc/3.6.0/share/sdcc/lib/pic14 \
		-I/usr/local/bin/../share/sdcc/non-free/lib/pic14 \
		-I/usr/local/Cellar/sdcc/3.6.0/share/sdcc/non-free/lib/pic14   \
		-w -r \
		-o dimmertest_out \
	  dimmertest.o  \
		libsdcc.lib pic16f690.lib -r
