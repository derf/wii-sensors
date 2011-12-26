CFLAGS = -Wall -Wextra -pedantic -Wno-unused-parameter -ggdb
LDFLAGS = -lbluetooth -lcwiid

all: acclog bal mpcal mplog wibble wiispkr

acclog: acclog.c
	${CC} ${CFLAGS} ${LDFLAGS} -lm -o $@ $<

bal: bal.c
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ $<

mpcal: mpcal.c
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ $<

mplog: mplog.c
	${CC} ${CFLAGS} ${LDFLAGS} -lm -o $@ $<

wibble: wibble.c
	${CC} ${CFLAGS} ${LDFLAGS} -lm -o $@ $<

wiispkr: wiispkr.c
	${CC} ${CFLAGS} -lm -lbluetooth -lcwiimote -o $@ $<

clean:
	rm -f bal mplog wibble

.PHONY: all clean
