//ZLFF 2014
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>

#include "include/pngdrive.h"
#include "include/pngbits.h"
#include "include/bitmasks.h"
#include "include/filesystem.h"

int mask = DEFAULT_MASK;

int headerSize(){
		return sizeof(header) + drive->filecount * (sizeof(filereference));
}

void loadMem(unsigned char * data){
	mem = data;	
	drive = (header *)mem;
	drive->files = (filereference*)(((char *)drive) + sizeof(header));// points immediately after the header
	printf("Loaded PNG drivo to memory:\n");
	printf("\tVersion:%d\n", drive->version);
	printf("\tTotal space:%d\n", drive->totalspace);
	printf("\tFile count:%d\n", drive->filecount);
}

bool validateMem(int size, unsigned char * data){
	header * h = (header *)data;
	return (h->totalspace == size);
	
}

void formatMem(int size, unsigned char * data){
	mem = data;	
	drive = (header *)mem;
	drive->version = PNG_DRIVE_VERSION;
	drive->totalspace = size;
	drive->freespace = size;
	drive->filecount = 0;
	drive->files = (filereference*)(((char *)drive) + sizeof(header));// points immediately after the header
}

void startMem(int size){
	unsigned char *m = (unsigned char *) malloc(size);
	printf("%d bytes alocated at: %p\n", size,mem);
	formatMem(size,m);
}

/*
	The references of the files grow from the begining of memory
	While the files are actually stored starting at the end of memory
	If a file changes in size or is deleted then the system becomes fragmented

	The center block (the space between the last reference and the first file data) is 
	the largest available space and will contain all available space when the disk is
	defragmented.
*/

char * getDataPointerFromReference(int reference_id){
	filereference * ref = drive->files+reference_id; 
	char *ret = (char *)drive;
	ret += ref->data_offset;
	return ret;
}

filereference *getReferenceByPath(char *path){
	int cnt;
	for(cnt = 0; cnt < drive->filecount; cnt++){
		filereference *ref = drive->files + cnt;
		if(strcmp(path,ref->name) == 0){// found match
			return ref;
		}
	}
	return NULL;
}

void defrag(){
	int cnt;
	int last_offset = drive->totalspace;
	// iterate over all files
	for(cnt = 0; cnt < drive->filecount; cnt++){
		filereference *curr = &(drive->files[cnt]);
		if(curr->data_offset != (last_offset - curr->size)){
			// original pointer, still contains the data
			byte *prev = ((byte*) drive) + curr->data_offset;
			// target pointer (updates the data_offset to the new address)
			curr->data_offset = last_offset - curr->size;
			byte *target = ((byte*) drive) + curr->data_offset;
			memmove(target,prev,curr->size);
		}
		last_offset = curr->data_offset;
		/*
			// if it's already the last file, then no need to move anything.
			if(cnt == drive->filecount - 1) {
				drive->filecount--;
				return;
			}else{
			//must move memory from here onwards
				// first move the references
				filereference *target_pointer = &(drive->files[cnt]);
				filereference *source_pointer  = &(drive->files[cnt+1]);
				int size = sizeof(filereference) * (drive->filecount - cnt);
				memmove(target_pointer,source_pointer,size);

				// then move the data - the last pointer is the one closeset 
				filereference *last_pointer = &(drive->files[drive->filecount-1]);

				
				// decrement the file counter because file has been deleted
				drive->filecount--;
			}
		}
		*/
	}
}

void deleteFile(const char *path){
	filereference *ref = getReferenceByPath((char *)path);	
	if(ref == NULL){
		printf("Warning: Trying to delete unexistent file: $%s\n", path);
		return;
	}

	drive->filecount--;
	drive->freespace += ref->size;
	// if the file to be deleted is already the last file in the list
	if(strcmp(drive->files[drive->filecount].name,path) == 0){
		// there is nothing else to be done, decrementing the filecount is enough
		return;
	} 

	// the file is somewhere in the middle or begining of the filelist
	int cnt;
	bool copyahead = false;
	for(cnt = 0; cnt < drive->filecount; cnt++){
		ref = drive->files + cnt;
		if(strcmp(path,ref->name) == 0){// found match
			copyahead = true;
		}
		if(copyahead){
			*ref = drive->files[cnt+1];
		}
	}

	defrag();
}

int addFile(char * name,int size, byte *content){
	int size_in_disk = size + sizeof(filereference);
	if(size_in_disk > availableSpace)
		return -ENOSPC;

	filereference newref;
	newref.type = REGULAR_FILE; 
	strncpy(&(newref.name[0]),name,FILENAME_MAX_LENGTH); 
	newref.name[FILENAME_MAX_LENGTH] = (char)0;
	newref.size = size;
	
	if(drive->filecount == 0){
		drive->filecount = 1;
		newref.data_offset = (drive->totalspace - size);
		unsigned char *ptr = mem + newref.data_offset;
		memcpy(ptr,content,size);
		drive->files[0] = newref;
		drive->freespace -= size;
	}else{// address of the last (or first in memory) file reference
		int last_address = drive->files[drive->filecount-1].data_offset; 
		drive->filecount++;
		newref.data_offset = (last_address - size);
		unsigned char *ptr = mem + newref.data_offset;
		memcpy(ptr,content,size);
		drive->files[drive->filecount-1] = newref;
		drive->freespace -= size;
	}

	return 0;	
}

int updateInfo(){
	sprintf(info,"PNG Drive version: %d\n", drive->version);
	sprintf(info,"%sTotal size: %d bytes.\n", info,drive->totalspace);
	sprintf(info,"%sAvailable space: %d bytes (File System header is %d bytes).\n", info,availableSpace,headerSize());
	sprintf(info,"%sNumber of files:%d\n",info,drive->filecount);
	return strlen(info);
}

