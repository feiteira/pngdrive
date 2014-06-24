#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>

#define PNG_DRIVE_VERSION 0x1

#define FILENAME_MAX_LENGTH 64

typedef unsigned char byte;

#define DRIVE_SUFIX ".drive"

#define FORMAT_OPTION "-format" 


void on_drive_exit();

