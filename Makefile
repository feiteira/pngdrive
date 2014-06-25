LIBS=`pkg-config fuse --cflags --libs` -lpng -lcrypto -lssl
SOURCES=pngbits.c filesystem.c bitmasks.c 
OBJECTS=$(SOURCES:.c=.o)
#CFLAGS=-g -Wall
CFLAGS=-g
EXE=pngdrive
CC=gcc


all: objects $(EXE)
#	gcc ${OBJECTS} pngdrive.c ${CFLAGS} ${LIBS} -o ${EXE}
	rm ${OBJECTS}

install:
	cp ${EXE} /usr/bin

$(EXE): $(OBJECTS) 
	$(CC) $(OBJECTS) pngdrive.c ${CFLAGS} ${LIBS}  -o $@

objects: ${OBJECTS}

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean: clean-objects
	rm -f ${EXE}
	
clean-objects:
	rm -f ${OBJECTS}
