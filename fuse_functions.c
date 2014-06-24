static int pngdrive_getattr(const char *path, struct stat *stbuf)
{
        int res = 0;

        printf("getattr: %s \n", path);

        memset(stbuf, 0, sizeof(struct stat));
        if (strcmp(path, "/") == 0) {
                stbuf->st_mode = S_IFDIR | 0755;
                stbuf->st_nlink = 2;
        } else if (strcmp(path, proc_path) == 0) {
                stbuf->st_mode = S_IFREG | 0444;
                stbuf->st_nlink = 1;
                stbuf->st_size = 1024;//strlen(pngdrive_str);-- It's a dynamic file, so never mind.
        } 
        else{
                int cnt;
                // for each file in drive
                for(cnt = 0; cnt < drive->filecount; cnt++){
                        printf("\t[%d]\t[%s]\n",cnt,drive->files[cnt].name);
                        filereference *ref = drive->files + cnt;
                        if(strcmp(path,ref->name) == 0){// found match
                                printf("found match, type: %d\n", ref->type);
                                stbuf->st_uid = getuid();
                                stbuf->st_gid = getgid();
                                if(ref->type == REGULAR_FILE){
                                        stbuf->st_mode = S_IFREG | 0644;
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

        // exception the procedure file, can also be opened
        if (strcmp(path, proc_path) == 0)
                return 0;

        printf("opening\n");

        if ((fi->flags & 3) != O_RDONLY)
                return -EACCES;

        return -ENOENT;
}
int pngdrive_create(const char *path, mode_t mode, struct fuse_file_info *info){
        printf("creating: %s\n", path);
        addFile((char *)path,0,NULL);
        return 0;
}

int pngdrive_write (const char *path, const char *data, size_t size, off_t offset, struct fuse_file_info *info){
        printf("writing %d bytes to %s\n", (int)size,path);
        filereference *ref = getReferenceByPath((char*)path);
        if(ref == NULL){
                return -ENOENT;
        }
        filereference refval = *ref;
        
        int totalsize = refval.size;
        if(totalsize < size + offset) totalsize = size + offset;
        byte *updated_file_data = (byte *) malloc(totalsize);   

        byte *ptr = ((byte*) drive) + refval.data_offset;
        memcpy(updated_file_data, ptr, refval.size);
        memcpy(updated_file_data+offset, data, size);

        deleteFile(path);
        int err = addFile((char *)path,totalsize,(byte *)updated_file_data);

        free(updated_file_data);        
        if(err < 0) 
                return err;
        
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
                                //rsize = datasize - offset;
                                unsigned char * ptr = mem + (ref->data_offset + offset); 
                                memcpy(buf,ptr, rsize);// puts the contents of the file in "buf"
                        }else 
                            rsize = 0;
                        return rsize;
                }
        }
        // for the info file
        if (strcmp(path, proc_path) == 0) {
                len = updateInfo();
                if (offset < len) {
                        if (offset + rsize > len)
                                rsize = len - offset;
                                memcpy(buf,info + offset, rsize);// puts the contents of the file in "buf"
                } else
                        rsize = 0;
                return rsize;
        }

        //if(strcmp(path, pngdrive_path) != 0)
        //      return -ENOENT;
        //len = strlen(pngdrive_str);
        //if (offset < len) {
        //      if (offset + rsize > len)
        //              rsize = len - offset;
        //              memcpy(buf,pngdrive_str + offset, rsize);// puts the contents of the file in "buf"
        //} else
        //      rsize = 0;

        return -ENOENT;
}

static void pngdrive_destroy(void *v){
	on_drive_exit();
}

static int pngdrive_rename (const char *srcpath, const char *destpath){
        printf("renaming %s to %s\n",srcpath,destpath);
        filereference *dest = getReferenceByPath((char *)destpath);
        filereference *src = getReferenceByPath((char *)srcpath);

        if(src == NULL){
                return -ENOENT; // no such file or directory
        }

        byte *data = (byte*)malloc(src->size);
        unsigned char *ptr = mem + src->data_offset;
        memcpy(data, ptr,src->size);

        if(dest != NULL){
                        deleteFile(destpath);
        }       
        deleteFile(srcpath);
        addFile((char*)destpath,src->size,data);        
        free(data);
        return 0;
}

static int pngdrive_unlink (const char *path){
        filereference *ref = getReferenceByPath((char *)path);
        if(ref == NULL){
                return -ENOENT; // no such file or directory
        }

        printf("Unlinking file:%s\n",path);
        deleteFile((char *) path);
        return 0;
}

static struct fuse_operations pngdrive_oper = {
        .getattr        = pngdrive_getattr,
        .readdir        = pngdrive_readdir,
        .open           = pngdrive_open,
        .write  = pngdrive_write,
        .read           = pngdrive_read,
        .create = pngdrive_create,
        .destroy        = pngdrive_destroy,
        .rename = pngdrive_rename,
        .unlink = pngdrive_unlink,
};
