CFLAGS=-c -O2 -g -Wall -DPLATFORM_GP2X -mcpu=arm920t -mtune=arm920 -ffast-math -W -Wall -pthread -msoft-float
LDFLAGS=-L/opt/open2x/gcc-4.1.1-glibc-2.3.6/lib -Wl,-rpath,/opt/open2x/gcc-4.1.1-glibc-2.3.6/lib -lSDL_gfx -lSDL_image -ljpeg -lpng12 -lz -lSDL -lpthread -lgcc -lm -lc -ldl -msoft-float -O2

SOURCES=SFont.c ts_calibrator.c

OBJECTS=$(SOURCES:.c=.o)
EXECUTABLEGP=ts_calibrator

CC=arm-open2x-linux-gcc
STRIP=arm-open2x-linux-strip
CFLAGS+=-I/opt/open2x/gcc-4.1.1-glibc-2.3.6/include/ \
		-I/opt/open2x/gcc-4.1.1-glibc-2.3.6/arm-open2x-linux/include/ \
		-I/opt/open2x/gcc-4.1.1-glibc-2.3.6/include/SDL -D_REENTRANT

all: 
	make gpversion

gpversion: $(SOURCES) $(EXECUTABLEGP)


$(EXECUTABLEGP): $(OBJECTS) 
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS) 
	$(STRIP) $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

cleanobjs:
	rm -f $(OBJECTS)

clean:
	-rm -f ts_calibrator *~ *.o *.bak 

