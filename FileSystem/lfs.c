/*
    Filename:lfs.c
    Author:ShanruiZhang
    Purpose:Implement FUSE layer
*/

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include "flash.h"
#include "log.h"

//extern Inodep *inodes;
//extern Dir dir;

//static const char *hello_str = "Hello World!\n";
static const char *hello_path = "/hello";
static const char *link_path = "/link";
int debug2=1;


static void printFiles(){
    printf("\n-----------PRINTFILES------------\n");
    int i;
    for(i=0;i<10;i++){
        if(dir.entry[i].inum==-10)
            break;
        printf("FILENAME: %s  INUM: %d\n",dir.entry[i].name,dir.entry[i].inum);
        
   
    }
     printf("\n-----------ENDPRINTING------------\n");
}
static int hello_getattr(const char *path, struct stat *stbuf)
{
    //int res = 0;
    
    
    
        
    memset(stbuf, 0, sizeof(struct stat));
    
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0777;//protection mode
        stbuf->st_nlink = 2;//numbers of hard link
        stbuf->st_ino = 3;// inum
        return 0;
    }
    /*else if (strcmp(path, hello_path) == 0) {
        stbuf->st_mode = S_IFREG | 0777;
        stbuf->st_nlink = 1;
        stbuf->st_size = strlen(hello_str);
        stbuf->st_ino = 17;
        return 0;
    } else if (strcmp(path, link_path) == 0) {
        stbuf->st_mode = S_IFLNK | 0777;
        stbuf->st_nlink = 1;
        stbuf->st_size = strlen(hello_path);
        stbuf->st_ino = 17;
        return 0;
    }*/
    
    //----------------------------->FIND INUM
    Dirp root=Dic_Path_Search((char*)path+1,&dir);
    
    char * ptr=(char *)path+1;
    int size1=0;
    while(*ptr!='\0'){
        ptr++;
        size1++;
    }
    int count=0;
    while(*ptr!='/'){
        if(count==size1)
            break;
        ptr--;
        count++;
        
    }
    if(*ptr=='/')
        ptr++;
    
    int inum=-100;
    inum=Dic_File_Search(ptr,root);
    //--------------------------------------------
    if(debug2){
        printf("DEBUG FROM GETATTR: INUM=%d\n",inum);
        if(inum!=-100){
            printf("DEBUG FROM GETATTR: SIZE OF INUM %d is =%d\n",inum,inodes[inum]->size);
            
           
        }
    }
    
    if(inum==-100){
        printf("\n\nGETATTR: THE FILE NOT FOUND IS :%s whole path:%s\n\n",ptr,path+1);
        return -ENOENT;
    }
    if(inodes[inum]->type==2){
         printf("DEBUG FROM GETATTR: FILE TYPE IS DIR\n\n");
         stbuf->st_mode = S_IFDIR | 0777;
    }
     else{
            printf("DEBUG FROM GETATTR: FILE TYPE IS REGULAR\n\n");
            stbuf->st_mode = S_IFREG | 0777;
     }
    stbuf->st_nlink = 1;
    stbuf->st_size = inodes[inum]->size;
    stbuf->st_ino = inum;
    return 0;
    
    
    
    
    
    
     
   
}

static int hello_fgetattr(const char* path, struct stat* stbuf, struct fuse_file_info *fi){
    hello_getattr(path,stbuf);
    return 0;
}

static int hello_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi)
{
    (void) offset;
    (void) fi;

    //if (strcmp(path, "/") != 0)
        //return -ENOENT;
    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
   // printf("\n\nREADDIR: THE WHOLE PATH IS :%s\n\n",(char *)path);
    //------------------>Find INOD
    Dirp root=Dic_Path_Search2((char*)path+1,&dir);
    //printf("\n\nREADDIR: THE CURRENT ROOT DIR IS :%s\n\n",root->name);
    //--------------------
    int i;
    for(i=0;i<10;i++){
        if(root->entry[i].inum!=-10){
            char * ptr=root->entry[i].name;
            int size1=0;
            while(*ptr!='\0'){
                ptr++;
                size1++;
            }
            int count=0;
            while(*ptr!='/'){
                if(count==size1)
                    break;
                ptr--;
                count++;
            }
            if(*ptr=='/')
                ptr++;
            //printf("\n\nREADDIR: THE FILE OF THE CURRENT DIR IS %s\n\n",ptr);
            filler(buf,ptr,NULL,0);
        }
    }
   

    return 0;
}

static int hello_open(const char *path, struct fuse_file_info *fi)
{
    /*if (strcmp(path, hello_path) != 0)
        return -ENOENT;

    if ((fi->flags & 3) != O_RDONLY)
        return -EACCES;*/

    return 0;
}

static int hello_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi)
{
    printf("\n\nHELLO_READ\n\n");
    Dic_File_Read((char *)path+1,(int)offset,size,buf);
   // Dic_File_Read((char *)path,(int)offset,size,(char*) buf);
   
    return size;
    /*size_t len;
    (void) fi;
    if(strcmp(path, hello_path) != 0)
        return -ENOENT;

    len = strlen(hello_str);
    if (offset < len) {
        if (offset + size > len)
            size = len - offset;
        memcpy(buf, hello_str + offset, size);
    } else
        size = 0;
    
    //Dic_File_Read((char *)path,(int)offset,(char*) buf);
    return size;*/
}
static int hello_mkdir(const char* path, mode_t mode){
    printf("\n\nMKDIR.....\n\n");
    FileCreateByName((char *)path+1,2);
    return 0;
}
static int hello_write(const char* path, const char *buf, size_t size, off_t offset, struct fuse_file_info* fi){
    printf("\n\nHELLO_WRITE!!!!!\n\n");
    Dic_File_Write((char *)path +1, (int)offset,size,(char*)buf);
    return size;
   
}


static int hello_truncate(const char* path, off_t size){
 
    Dirp root=Dic_Path_Search((char*)path+1,&dir);
  
    int inum=-100;
   char * ptr=(char *)path+1;
    int size1=0;
    while(*ptr!='\0'){
        ptr++;
        size1++;
    }
    int count=0;
    while(*ptr!='/'){
        if(count==size1)
            break;
        ptr--;
        count++;
        
    }
    if(*ptr=='/')
        ptr++;
    inum=Dic_File_Search(ptr,root);
   
    if(inum==-100){
        return -ENOENT;
    }
    int size2=inodes[inum]->size;
    void* buffer=malloc(1024);
    Dic_File_Read((char*)path+1,0,size2,buffer);
   
    void* buffer2=malloc(1024);
    //Append
    if(size>size2){
        memcpy(buffer2,buffer,size);
        Dic_File_Write((char*)path+1,0,size,buffer2);
    }
    //Truncate
    else{
       
        memcpy(buffer2,buffer,size);
        File_Free((char*)path+1);
       
        FileCreateByName((char *)path+1,0);
        Dic_File_Write((char*)path+1,0,size,buffer2);
        
    }
   
    return 0;
}

static int hello_mknod(const char* path, mode_t mode, dev_t rdev){
     printf("MAKE A PLAIN FILE A \n\n\n\n\n\n");
     return 0;
}

static int hello_create(const char *path, mode_t mode, struct fuse_file_info *fi){
    printf("CREATE ::MAKE A PLAIN FILE A \n\n\n\n\n\n");
    FileCreateByName((char *)path+1,0);
    
    return 0;
}
static int hello_opendir(const char* path, struct fuse_file_info* fi){
    
    printf("\n\nOPENDIR......\n\n");
    if(strcmp(path,"/")==0)
        return 0;
    Dirp root=Dic_Path_Search((char*)path+1,&dir);
    char * ptr=(char *)path+1;
    int size1=0;
    while(*ptr!='\0'){
        ptr++;
        size1++;
    }
    int count=0;
    while(*ptr!='/'){
        if(count==size1)
            break;
        ptr--;
        count++;
        
    }
    if(*ptr=='/')
        ptr++;
    
    int inum=-100;
    inum=Dic_File_Search(ptr,root);
    if(inum==-100)
        return -ENOENT;
    if(inodes[inum]->type!=2){
         return -ENOENT;
    }
    return 0;
}

static int hello_readlink(const char *path, char *buf, size_t size)
{
    if(strcmp(path, link_path) != 0)
        return -ENOENT;
    buf[size - 1] = '\0'; 
    strncpy(buf, ".", size-1);
    strncat(buf, hello_path, size-2);

    return 0;
}
static int hello_release(const char* path, struct fuse_file_info *fi){
    printf("RELEASE!!!\n");
    printFiles();
    return 0;
}
static int hello_unlink(const char* path){
    Dirp root=Dic_Path_Search((char*)path+1,&dir);
    int inum=-100;
   char * ptr=(char*)path+1;
    int size=0;
    while(*ptr!='\0'){
        ptr++;
        size++;
    }
    int count=0;
    while(*ptr!='/'){
        if(count==size)
            break;
        ptr--;
        count++;
        
    }
    if(*ptr=='/')
        ptr++;
    inum=Dic_File_Search(ptr,root);
    if(inum==-100)
        return -ENOENT;
    File_Free((char*)path+1);
    return 0;
}
static void hello_destroy(void* private_data){
    
}
static int  hello_rmdir(const char* path){
     Dirp root=Dic_Path_Search((char*)path+1,&dir);
    int inum=-100;
   char * ptr=(char*)path+1;
    int size=0;
    while(*ptr!='\0'){
        ptr++;
        size++;
    }
    int count=0;
    while(*ptr!='/'){
        if(count==size)
            break;
        ptr--;
        count++;
        
    }
    if(*ptr=='/')
        ptr++;
    inum=Dic_File_Search(ptr,root);
    if(inum==-100)
        return -ENOENT;
    void *buffer=malloc(1024);
    File_Read(inum,0,sizeof(Dir),buffer);
    Dirp temp=(Dirp)buffer;
    int i;
    int flag=0;
    for(i=0;i<10;i++){
        if(temp->entry[i].inum!=-10)
            flag=1;
    }
    if(flag)
        return ENOTEMPTY;
    File_Free((char*)path+1);
    return 0;
}

static struct fuse_operations hello_oper = {
    .getattr	   = hello_getattr,
    .fgetattr    = hello_fgetattr,
    .readdir	= hello_readdir,
    .open	      = hello_open,
    .write      = hello_write,
    .create     = hello_create,
    .mknod      = hello_mknod,
    .truncate    = hello_truncate,
    .read	      = hello_read,
    .release        = hello_release,
    .readlink   = hello_readlink,
    .unlink          =hello_unlink,
    .destroy     = hello_destroy,
    .mkdir       = hello_mkdir,
    .opendir    =hello_opendir,
    .rmdir      =hello_rmdir
};

int main(int argc, char *argv[])
{
    char 	**nargv = NULL;
    int		i;
    int         nargc;

#define NARGS 3

    nargc = argc -1 + NARGS;

    nargv = (char **) malloc(nargc * sizeof(char*));
    nargv[0] = argv[0];
    nargv[1] = "-f";
    nargv[2] = "-s";
    nargv[3] = "-d";
    int j=1;
    for (i = 1; i < argc; i++) {
        if(i==argc-2)
            continue;
	    nargv[j+NARGS] = argv[i];
        j++;
    }
    
    Lfs_start(argv[argc-2]);
   
    return fuse_main(nargc, nargv, &hello_oper, NULL);
}
