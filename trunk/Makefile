all: tools
	gcc objs/pngbits.o objs/bitmasks.o -Wall -g pngdrive.c `pkg-config fuse --cflags --libs` -o pngdrive

tools:
	gcc pngbits.c -g -c -o objs/pngbits.o 
	gcc bitmasks.c -g -c -o objs/bitmasks.o

clean:
	rm -f objs/*.o pngdrive	
