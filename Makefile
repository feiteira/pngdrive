LIBS=`pkg-config fuse --cflags --libs` -lpng 
SOURCES=pngbits.c filesystem.c bitmasks.c 
OBJECTS=$(SOURCES:.c=.o)
#CFLAGS=-g -Wall
CFLAGS=-g
EXE=pngdrive


all: objects $(EXECUTABLE)
	gcc ${OBJECTS} pngdrive.c ${CFLAGS} ${LIBS} -o ${EXE}
	rm ${OBJECTS}

$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(CFLAGS) $(OBJECTS) -o $@

objects: ${OBJECTS}

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean: clean-objects
	rm -f ${EXE}
	
clean-objects:
	rm -f ${OBJECTS}
