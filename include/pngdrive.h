#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>


#ifndef PNGDRIVE_H
#define PNGDRIVE_H

#define PNG_DRIVE_VERSION 0x1

#define FILENAME_MAX_LENGTH 64

typedef unsigned char byte;

#define DRIVE_SUFIX ".drive"

#define FORMAT_OPTION "-format" 
#define DEBUG_OPTION "-debug" 
#define KEY_OPTION "-key=" 
#define MASK_OPTION "-mask=" 

#define DEFAULT_MASK 0x0010102


#define DEBUG if(debug_pngdrive)
#define DEBUG_OFF debug_pngdrive = false;
#define DEBUG_ON debug_pngdrive = true;

extern bool debug_pngdrive;
bool debug_pngdrive;

void on_drive_exit();

#endif
