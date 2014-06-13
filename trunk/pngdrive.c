//ZLFF 2014
#define FUSE_USE_VERSION 26
#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>


#include "include/pngdrive.h"

unsigned char *mem;
header *drive; 


void startMem(int size){
	mem = (unsigned char *) malloc(size);
	printf("%d bytes alocated at: %p\n", size,mem);
	drive = (header *)mem;
	drive->version = 0;
	drive->totalspace = size;
	drive->freespace = size;
	drive->filecount = 0;
	drive->files = (filereference*)(((char *)drive) + sizeof(header));// points immediately after the header
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

void deleteFile(const char *path){
	filereference *ref = getReferenceByPath((char *)path);	
	if(ref == NULL){
		printf("Warning: Trying to delete unexistent file: $%s\n", path);
		return;
	}

	drive->filecount--;
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
}

int addFile(char * name,int size, char *content){
	int size_in_disk = size + strlen(name) + 1;
	if(size_in_disk > drive->freespace)
		return -ENOSPC;

	filereference newref;
	newref.type = REGULAR_FILE; 
	//strcpy((char *)&(name[0]), &(newref.name[0])); 
	strncpy(&(newref.name[0]),name,FILENAME_MAX_LENGTH); 
	newref.name[FILENAME_MAX_LENGTH] = (char)0;
	newref.size = size;
	
	if(drive->filecount == 0){
		drive->filecount = 1;
		newref.data_offset = (drive->totalspace - 1 - size);
		unsigned char *ptr = mem + newref.data_offset;
		memcpy(ptr,content,size);
		drive->files[0] = newref;
	}else{// address of the last (or first in memory) file reference
		int last_address = drive->files[drive->filecount-1].data_offset; 
		drive->filecount++;
		newref.data_offset = (drive->totalspace - 1 - last_address);
		unsigned char *ptr = mem + newref.data_offset;
		memcpy(ptr,content,size);
		drive->files[drive->filecount-1] = newref;
	}

	return 0;	
}

void defrag(){
	int cnt;
	// iterate over all files
	for(cnt = 0; cnt < drive->filecount; cnt++){
		//if the file has been deleted, then must defragment from here onwards
		if(drive->files[cnt].type == DELETED){
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
		
	}
}



static int pngdrive_getattr(const char *path, struct stat *stbuf)
{
	int res = 0;

	printf("getattr: %s \n", path);

	memset(stbuf, 0, sizeof(struct stat));
	if (strcmp(path, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} else if (strcmp(path, pngdrive_path) == 0) {
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = strlen(pngdrive_str);
	} else if (strcmp(path, "/folder") == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	} else if (strcmp(path, proc_path) == 0) {
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = strlen(pngdrive_str);
	} 
	else{
		int cnt;
		// for each file in drive
		for(cnt = 0; cnt < drive->filecount; cnt++){
			printf("\t[%d]\t[%s]\n",cnt,drive->files[cnt].name);
			filereference *ref = drive->files + cnt;
			if(strcmp(path,ref->name) == 0){// found match
				printf("found match, type: %d\n", ref->type);
				if(ref->type == REGULAR_FILE){
					stbuf->st_mode = S_IFREG | 0444;
					stbuf->st_nlink = 1;
					stbuf->st_size = ref->size;
					printf("size: %d\n", (int)stbuf->st_size);
					return res;
				}else if (ref->type == DIRECTORY){
				    	stbuf->st_mode = S_IFDIR | 0755;
				    	stbuf->st_nlink = 2;
					return res;
				}else{
					printf("Unknonw filetype %d for file named %s\n", ref->type, ref->name);
				}
			}
	  	}
		
		res = -ENOENT;
	}
	


	return res;
}

static int pngdrive_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi)
{

	printf("readdir: %s\n", path);
	(void) offset;
	(void) fi;

	if (strcmp(path, "/") != 0)
		return -ENOENT;

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	filler(buf, proc_path + 1, NULL, 0);
	filler(buf, pngdrive_path + 1, NULL, 0);

	int cnt = 0;
	printf("filecount:: %d\n", drive->filecount);
	for(cnt = 0; cnt < drive->filecount; cnt++){
		printf("\t[%d]\t%s\n",cnt,drive->files[cnt].name);
		char *nameptr = (char *)(drive->files[cnt].name);
		filler(buf, nameptr+1, NULL, 0);
	}



	return 0;
}

static int pngdrive_open(const char *path, struct fuse_file_info *fi)
{
	filereference *ref = getReferenceByPath((char *)path);
	// if the file exists then it can be opened (yeah as simple as that)
	if(ref != NULL)
		return 0;	

	printf("opening\n");
	if (strcmp(path, pngdrive_path) != 0)
		return -ENOENT;

	if ((fi->flags & 3) != O_RDONLY)
		return -EACCES;

	return 0;
}
int pngdrive_create(const char *path, mode_t mode, struct fuse_file_info *info){
	printf("creating: %s\n", path);
	addFile((char *)path,0,NULL);
	return 0;
}

int pngdrive_write (const char *path, const char *data, size_t size, off_t offset, struct fuse_file_info *info){
	printf("writing '%s' to %s\n", data,path);
	filereference *ref = getReferenceByPath((char*)path);
	filereference refval = *ref;
	if(ref == NULL){
		return -ENOENT;
	}
	
	int totalsize = refval.size;
	if(totalsize < size + offset) totalsize = size + offset;
	byte *updated_file_data = (byte *) malloc(totalsize);	

	byte *ptr = ((byte*) drive) + refval.data_offset;
	memcpy(updated_file_data, ptr, refval.size);
	memcpy(updated_file_data+offset, data, size);

	deleteFile(path);
	addFile((char *)path,totalsize,(char *)updated_file_data);

	free(updated_file_data);	
	
	return size;
}

static int pngdrive_read(const char *path, char *buf, size_t rsize, off_t offset,
		      struct fuse_file_info *fi)
{
	printf("reading: %s\n", path);
	size_t len;
	(void) fi;
	
	filereference *ref = getReferenceByPath((char *)path);
	if(ref != NULL){
		if(strcmp(path,ref->name) == 0){
			int datasize = ref->size;
			if(offset < datasize){
				rsize = datasize - offset;
				unsigned char * ptr = mem + (ref->data_offset + offset); 
				memcpy(buf,ptr, rsize);// puts the contents of the file in "buf"
			}else 
			    rsize = 0;
			return rsize;
		}
	}


	if(strcmp(path, pngdrive_path) != 0)
		return -ENOENT;

	len = strlen(pngdrive_str);
	if (offset < len) {
		if (offset + rsize > len)
			rsize = len - offset;
			memcpy(buf,pngdrive_str + offset, rsize);// puts the contents of the file in "buf"
	} else
		rsize = 0;

	return rsize;
}

static void pngdrive_destroy(void *v){
	printf("exiting....\n");
}

static struct fuse_operations pngdrive_oper = {
	.getattr	= pngdrive_getattr,
	.readdir	= pngdrive_readdir,
	.open		= pngdrive_open,
	.write	= pngdrive_write,
	.read		= pngdrive_read,
	.create	= pngdrive_create,
	.destroy	= pngdrive_destroy,
};

int main(int argc, char *argv[])
{
	startMem(1024*1024);
	return fuse_main(argc, argv, &pngdrive_oper, NULL);
}
