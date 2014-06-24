//ZLFF 2014
#define FUSE_USE_VERSION 26
#include <fuse.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>

#include "include/pngdrive.h"
#include "include/pngbits.h"
#include "include/bitmasks.h"
#include "include/filesystem.h"

#include "fuse_functions.c"

png_store pngdata;
char *drivename;
char *filepath;


void help(){
	printf("This should be the help\n");
}

char * getFileNameFromPath(char * path){
	char * ptr = path;
	char * ret = path;

	for(;*ptr;ptr++)
		if(*ptr == '/')
			ret = ptr+1;
	return ret;
}

void prepareDrive(char * filepath,png_store * pngdata, bool format){
	pngdata->key=NULL;
	pngdata->mask = DEFAULT_MASK;
	pngdata->mask = 0x00010102;
	
	printf("Opening file: %s\n", filepath);

	FILE *f = readpng_or_exit(filepath,pngdata);	
	int size = getPNGDriveSize(pngdata);

	printf("This image can store %d bytes (%.2f MBs).\n",size,size/(1024*1024.0f));
	fclose(f);
	loadPNGDriveData(pngdata);// the malloc on pngdata->drivedata is done here

	if(format)
		formatMem(size, pngdata->drivedata);
	else
		loadMem(pngdata->drivedata);

	bool valid = validateMem(size, pngdata->drivedata);

	if(!valid){
		fprintf(stderr,"Exiting, found invalid drive. Expected to find %d size drive instead found %d.\n",size,((header *)pngdata->drivedata)->totalspace );
		fprintf(stderr,"Consider using the -format option\n");
		help();
		exit(1);
	}else{
		printf("PNG appears to contain a valid filesystem\n");
	}
}

bool checkFormatOption(int argc, char *argv[]){
	int cnt = 0;
	for(; cnt < argc-1;cnt++)// goes only up to argc - 1 because the last argument is always the filename
		if(strcmp(argv[cnt],FORMAT_OPTION) == 0){
			return true;
		}
	return false;
}

struct sigaction old_action;

void on_drive_exit() {
	printf("System interrupted, saving changes into image... exiting when finished...\n");
	savePNGDriveData(&pngdata);
	writepng(filepath,&pngdata);
//	printf("Removing %s\n",drivename);
	rmdir(drivename);
}



int main(int argc, char *argv[])
{
	//-------- exit if invalid number of arguments
	if(argc < 2){
		help();
		exit(1);
	}

	info = (char *) malloc(4096);
//	startMem(1024*1024);
//	startMem(50*1024*1024);

	//------- load file into memory
	struct stat st = {0};
	filepath = argv[argc-1];
	char *filename = getFileNameFromPath(filepath);
	drivename = (char*) malloc(strlen(filename)+strlen(DRIVE_SUFIX)+1);
	sprintf(drivename,"%s%s",filename,DRIVE_SUFIX);

	printf("File path is %s\n", filepath);
	printf("File name is %s\n", filename);

	bool format = checkFormatOption(argc, argv);
	prepareDrive(filepath,&pngdata,format);

	if (stat(drivename, &st) == -1) {
          printf("Creating folder %s...\n",drivename);	
	    mkdir(drivename, 0755);
	}else{
		fprintf(stderr, "Error, folder %s alreay exists\n",drivename);
		help();
		exit(1);
	}

		

	//------ fuse options "-f -o big_writes" are set by default
	int fargc = 5;
	const char *fargv[fargc];
	fargv[0] = argv[0];	
	fargv[1] = "-f";	
	fargv[2] = "-o";	
	fargv[3] = "big_writes";	
	fargv[4] = drivename;	
	int i;

	for(i = 0; i < fargc; i++)
		printf("\t%s\n",fargv[i]);


	return fuse_main(fargc, (char **)fargv, &pngdrive_oper, NULL);
}
