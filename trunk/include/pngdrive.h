#define FUSE_USE_VERSION 26


#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>


#define FILENAME_MAX_LENGTH 64

static const char *pngdrive_str = "Hello World!\n";
static const char *pngdrive_path = "/h1llo";
static const char *proc_path = "/proc";

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


