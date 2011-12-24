CFLAGS = -Wall -Wextra -pedantic -Wno-unused-parameter
LDFLAGS = -lbluetooth -lcwiid

all: bal mpcal mplog wibble wiispkr

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
