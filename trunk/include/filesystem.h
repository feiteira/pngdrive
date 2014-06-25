//ZLFF 2014
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>

#ifndef PNGFILESYSTEM
#define PNGFILESYSTEM

static const char *proc_path = "/pngdrive.info";

typedef enum {DELETED,REGULAR_FILE, DIRECTORY} filetype;


/*
	The references of the files grow from the begining of memory
	While the files are actually stored starting at the end of memory
	If a file changes in size or is deleted then the system becomes fragmented

	The center block (the space between the last reference and the first file data) is 
	the largest available space and will contain all available space when the disk is
	defragmented.
*/
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

// first declare them as external
extern unsigned char *mem;
extern header *drive; 
extern char *info;

unsigned char *mem;
header *drive; 
char *info;


// returns the size of the header in bytes, including the filereferences
int headerSize();

//brings up memory (formatted)
void startMem(int size);

void loadMem(unsigned char * data);

bool validateMem(int size, unsigned char * data);

void formatMem(int size, unsigned char * data);

char * getDataPointerFromReference(int reference_id);

filereference *getReferenceByPath(char *path);

void defrag();

void deleteFile(const char *path);

int addFile(char * name,int size, byte *content);

int updateInfo();
#endif //PNGFILESYSTEM
