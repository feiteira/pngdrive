//ZLFF 2014
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>

//#include "include/pngdrive.h"
//#include "include/pngbits.h"
//#include "include/bitmasks.h"
#ifndef PNGFILESYSTEM
#define PNGFILESYSTEM

//static const char *pngdrive_str = "Hello World!\n";
//static const char *pngdrive_path = "/h1llo";
static const char *proc_path = "/pngdrive.info";

typedef enum {DELETED,REGULAR_FILE, DIRECTORY} filetype;


typedef struct filereference{
	filetype type;
	int size;
	int data_offset;
	char name[FILENAME_MAX_LENGTH + 1];
} filereference;

typedef struct header{
	int version;
	int totalspace;
	int freespace;
	int filecount;
	filereference *files;
} header;

typedef unsigned char byte;

#define last_file_reference drive->files[drive->filecount-1]




#define availableSpace (drive->freespace - headerSize())
unsigned char *mem;
header *drive; 
char *info;
int mask = DEFAULT_MASK;

int headerSize();
void startMem(int size);

/*
	The references of the files grow from the begining of memory
	While the files are actually stored starting at the end of memory
	If a file changes in size or is deleted then the system becomes fragmented

	The center block (the space between the last reference and the first file data) is 
	the largest available space and will contain all available space when the disk is
	defragmented.
*/

char * getDataPointerFromReference(int reference_id);

filereference *getReferenceByPath(char *path);

void defrag();

void deleteFile(const char *path);

int addFile(char * name,int size, byte *content);

void updateInfo();
#endif //PNGFILESYSTEM
