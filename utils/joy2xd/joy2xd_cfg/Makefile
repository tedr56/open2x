CFLAGS=-c -O2 -g -Wall -DPLATFORM_GP2X -mcpu=arm920t -mtune=arm920 -ffast-math -W -Wall -pthread -msoft-float

#LDFLAGS=-L/opt/open2x/gcc-4.1.1-glibc-2.3.6/lib -Wl,-rpath,/opt/open2x/gcc-4.1.1-glibc-2.3.6/lib -static -lSDL_gfx -lSDL_image -ljpeg -lpng12 -lz -lSDL -lpthread -lgcc -lm -lc -ldl -msoft-float -O2
LDFLAGS=-L/opt/open2x/gcc-4.1.1-glibc-2.3.6/lib -Wl,-rpath,/opt/open2x/gcc-4.1.1-glibc-2.3.6/lib -lSDL_gfx -lSDL_image -ljpeg -lpng12 -lz -lSDL -lpthread -lgcc -lm -lc -ldl -msoft-float -O2

SOURCES=SFont.c joy2xd_cfg.c


OBJECTS=$(SOURCES:.c=.o)
EXECUTABLEGP=joy2xd_cfg

CC=arm-open2x-linux-gcc
STRIP=arm-open2x-linux-strip
CFLAGS+=-I/opt/open2x/gcc-4.1.1-glibc-2.3.6/include/ \
		-I/opt/open2x/gcc-4.1.1-glibc-2.3.6/arm-open2x-linux/include/ \
		-I/opt/open2x/gcc-4.1.1-glibc-2.3.6/include/SDL -D_REENTRANT \
		-msoft-float -DPLATFORM_GP2X

all: 
	make gpversion

gpversion: $(SOURCES) $(EXECUTABLEGP)


$(EXECUTABLEGP): $(OBJECTS) 
#	$(CC) $(LDFLAGS) -static $(OBJECTS) -o $@ $(GPLDFLAGS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS) 
	$(STRIP) $@


.c.o:
	$(CC) $(CFLAGS) $< -o $@

cleanobjs:
	rm -f $(OBJECTS)

clean:
	-rm -f joy2xd_cfg *~ *.o *.bak 

