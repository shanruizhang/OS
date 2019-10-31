/*
 Filename:log.c
 Author:ShanruiZhang
 Purpose:Implement the log,file,and dictory layer
*/
#include "log.h"
#include<stdio.h>
#include<stdlib.h>
#include"flash.h"
#include <string.h>

/*---------------------------------
	In-memory data structures
----------------------------------*/

Segmentp tail=NULL;
Inodep ifile=NULL;
Flash flash=NULL;
Superp super=NULL;
Inodep *inodes;
Dir dir;//Dirctory entry


int debug=0;
int bSize=-1;
int sSize=-1;
int fSize=-1;
int wear=-1;
int offset=-1;
int debugRead=0;
int debugWrite=0;
int debugFile1=0;
int debugDic=1;


/*----------------------------
	Helper functions
-----------------------------*/
int Error_handler(char *);//Print error message
Segmentp InitSegment();//Initialize Segment
Inodep InitInode(int ,int);//Initialize inode
void InitDir(Dirp input,char * name,int inum);//Initialize directory
void printAll();


/*---------------------------------
	Function implementations
----------------------------------*/

/*-
	LFS_START
-*/
int Lfs_start(char *FlashDeviceName){

	//1--------------->read super block
	u_int FlashSize;
	flash=Flash_Open(FlashDeviceName,FLASH_SILENT,&FlashSize);
	if(flash==NULL)
		Error_handler("flash is NULL!\n");
	
	void *test=malloc(sSize*bSize*512);
	u_int readRes=-1;
	readRes=Flash_Read(flash,0,sSize*bSize,test);
	if(readRes==1)
		 Error_handler("Error from logStart()-->flash read error!!\n");
	
	super=((Superp)test);
	bSize=super->bSize;
	sSize=super->sSize;
	fSize=super->fSize;
	wear=super->wear;
	offset=super->offset;
	if(debug){
		printf("Debug from logStart()--->bbSize=%d,sSize=%d,fSize=%d,wear=%d,offset=%d\n",bSize,\
		sSize,fSize,wear,offset);
	}

	//2--------------->intialize tail segment in memory
	tail=InitSegment();

	//3--------------->initialize ifile in main memory
	File_Create(-1,-1);
	inodes=malloc(40*sizeof(Inodep));
	//4--------------->initilize directory in main memory
	InitDir(&dir,"root",-2);

	return 0;

}
int Lfs_Close(){
	return 0;
}

/*-
        LOG_READ
_*/
int Log_Read(logAddress *address,int length,void *buffer){
	if(debugRead){
		printf("debugRead: number of segment=%d\n",super->offset);
	}

	//1-------------------->The content to be read is not exist 
	if(address->sNum>(super->offset))
		Error_handler("Error from Log_Read: the content to be read is not exist..The segment number is too big!!!\n");

	//2-------------------->The content to be read is in memory
	if(address->sNum==(super->offset)){
		if(debugRead)
			printf("debugRead: Content is in memory\n");
		
		void **temp=tail->blocks;
		int i;
		for(i=0;i<(address->bNum);i++){
			temp++;
		}
		memcpy(buffer,*temp,length);
		return 0;
	}

	//3--------------------->The content to be read is in flash device	
	int sNum=address->sNum;	
	int bNum=address->bNum;
	int offset=(sNum*sSize+bNum+1)*bSize;
	int numOfSector=-1;
	if(length%512==0)
		numOfSector=length/512;
	else
		numOfSector=length/512+1;
		
	int res=Flash_Read(flash,offset,numOfSector,buffer);	
	if(res==1)
		Error_handler("Error from Log_Read: error when calling flash read!!!\n");
	
	
	return 0;
}

/*-
	LOG_WRITE
_*/
int Log_Write(int inum,int block,int length,void *buffer,logAddress *address){

	//1------->Handle invalid 
	int sNum=address->sNum;
        int bNum=address->bNum;
	if(sNum!=super->offset)
		Error_handler("Error from Log_Write: the segment num is not invalid!!\n");
	if(bNum!=tail->super->offset-1)
		Error_handler("Error from Log_Write: the block num is not invalid!!\n");
	if(tail==NULL)
		Error_handler("Error from Log_Write: the tail segment is null!\n");



	//2------>Write to the tail segment, which is in memory
	void **temp=tail->blocks;
	int i;
	for(i=0;i<(address->bNum);i++){
		temp++;
	}
	memcpy(*temp,buffer,length);
	tail->super->offset++;
	


	inodes[inum]->blocks[block].bNum=address->bNum;
	inodes[inum]->blocks[block].sNum=address->sNum;



	//3------->If the tail segment is full, write to flash
	if(tail->super->offset==tail->super->sSize){
		int offset=(super->offset)*sSize*bSize;
		//Write super block
		int res=-1;
		void *superTemp=malloc(bSize*512);
		memcpy(superTemp,tail->super,sizeof(Superblock));
		res=Flash_Write(flash,offset,bSize,superTemp);
		if(res==1)
			Error_handler("Error form Log_Write: error when write superblock into flash!\n");
		//Write blocks
		int i;
		void **temp=tail->blocks;
		for(i=0;i<sSize-1;i++){
			offset=offset+bSize;
			int res2=-1;
			res2=Flash_Write(flash,offset,bSize,*temp);
			if(res2==1)
				Error_handler("Error form Log_Write: error when write superblock into flash!\n");
			temp++;
		}	
		super->offset++;
		//Initialize a new tail segment
		tail=InitSegment();
	}
	
	return 0;
}
int Log_Free(logAddress address,int length){
	return 0;
}



/*-
	FILE_WRITE
_*/
int File_Write(int inum,int offset,int length,void *buffer){
	if(offset>4*(bSize*512))
		Error_handler("Error from FILE_WRITE: the file have only 4 blocks in phase1\n");
	int bNum=offset/(bSize*512);
	int bOffset=offset%(bSize*512);

	//1--------------->write a new block
	if(inodes[inum]->blocks[bNum].sNum==-1){

		//a------------>Write from the begining
		if(bOffset==0){
			if(debugDic)
				printf("write a new block from the begining\n");
			int bIndex=offset/(bSize*512);
			logAddressp logTempAddress=malloc(sizeof(logAddress));
			logTempAddress->bNum=(tail->super->offset)-1;
			logTempAddress->sNum=super->offset;
			if(debugDic){
				printf("This is from FILE_WRITE(new)sNum=%d,bNum=%d\n",logTempAddress->sNum,logTempAddress->bNum);
			}	
			Log_Write(inum,bIndex,length,buffer,logTempAddress);
			inodes[inum]->size+=length;
			return 0;
		}
		//b-------------->Write from middle
		if(debugDic)
			printf("write a new block from middle\n");
		void *bufferWhole=malloc(bSize*512);
		char *temp=malloc(bOffset);
		int i;
		for(i=0;i<bOffset;i++)
			temp[i]='\0';
		memcpy(bufferWhole,((void *)temp),bOffset);
		memcpy(bufferWhole+bOffset,buffer,length);
       
        int bIndex=offset/(bSize*512);
        logAddressp logTempAddress=malloc(sizeof(logAddress));
        logTempAddress->bNum=(tail->super->offset)-1;
        logTempAddress->sNum=super->offset;
        Log_Write(inum,bIndex,length+bOffset,bufferWhole,logTempAddress);
        inodes[inum]->size+=length+bOffset;
		return 0;	
	}

	//2--------------->modifiy a block
	if(debugDic)
		printf("modify or append a block\n");
	void *bufferWhole=malloc(bSize*512);
	void *bufferOld=malloc(bOffset);
    void *bufferOld2=malloc(1024-bOffset-length);
    
    
	File_Read(inum,bNum,bOffset,bufferOld);
    File_Read(inum,bNum+bOffset+length,1024-bOffset-length,bufferOld2);
    if(debugDic){
        printf("length=%d\n",length);
        printf("bufferOld2 length= %d\n",1024-bOffset-length);
		printf("bufferOld2 is ");
		int k;
		for(k=0;k<1024-bOffset-length;k++)
			printf("%c",((char *)bufferOld2)[k]);
		printf("\n");
	}
	memcpy(bufferWhole,bufferOld,bOffset);
	memcpy(bufferWhole+bOffset,buffer,length);
    memcpy(bufferWhole+bOffset+length,bufferOld2,1024-bOffset-length);
    
	if(debugDic){
		printf("the buffer after modify is ");
		int k;
		for(k=0;k<1024;k++)
			printf("%c",((char *)bufferWhole)[k]);
		printf("\n");
	}
	int bIndex=offset/(bSize*512);
	logAddressp logTempAddress=malloc(sizeof(logAddress));
	logTempAddress->bNum=(tail->super->offset)-1;
	logTempAddress->sNum=super->offset;
	if(debugDic){
		printf("Thi is from FILE_WRITE(modify): sNum=%d,bNum=%d\n",logTempAddress->sNum,logTempAddress->bNum);
	}	
	Log_Write(inum,bIndex,bSize*512,bufferWhole,logTempAddress);
    
    //If modify from middle and not append
    if(bOffset+length<=inodes[inum]->size){
        if(debugDic){
            printf("FILE_WRITE: THE FILE SIZE IS NOT CHANGED\n");
        }
    }
        
    else{
	   inodes[inum]->size+=bOffset+length-inodes[inum]->size;
       if(debugDic){
            printf("FILE_WRITE: SIZE CHANGED!!!!");
        }
    }

	
	return 0;
}

/*-
	FILE_READ
_*/
int File_Read(int inum,int offset,int length,void *buffer){
	if(inodes[inum]==NULL)
		Error_handler("Error from FILE_READ: the inum is not exist!\n");
	
	int bIndex=offset/(bSize*512);
	int reminder=offset%(bSize*512);
	logAddressp temp=&(inodes[inum]->blocks[bIndex]);
	if(debugDic){
		printf("This is from FILE_READ:sNum=%d,bNum=%d\n",temp->sNum,temp->bNum);
        }

	void *bufferTemp=malloc(bSize*512);	

	//1-------------->Read the whole buffer
	Log_Read(temp,bSize*512,bufferTemp);


	//2--------------->Read the proper offset from the buffer
	memcpy(buffer,bufferTemp+reminder,length);
	
	return 0;
}

/*-
	FILE_FREE
_*/
int File_Free(char* path){
    Dirp root=Dic_Path_Search(path,&dir);
    //----------------->Tokenize the file name
    char * ptr=path;
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
    
	   int i;
        int inum=-100;
        for(i=0;i<10;i++){
                if(strcmp(root->entry[i].name,ptr)==0){
                    
                        inum=root->entry[i].inum;
                        break;
                }
        }
    if(inum==-100){
        Error_handler("ERROR FILE_FREE: THE NAME OF FILE DOES NOT EXIST!!!");
        
    }
   
   
   
    strcpy(root->entry[i].name,"NULL");
    root->entry[i].inum=-10;
   if(strcmp(root->name,"root")!=0)
        File_Write(root->inum,0,sizeof(Dir),root);
     
    inodes[inum]=InitInode(inum,0);
	return 0;
}
void FreeAll(){
    
}

/*-
	FILE_CREATE
_*/
int File_Create(int inum,int type){
	if(debugFile1)
		printf("THe size of Inode is %lu\n",sizeof(Inode));
	if(inum==-1){
		ifile=InitInode(inum,type);
		return 0;
	}
	if(inodes[inum]!=NULL)
		Error_handler("Error from FILE_CREATE: The inum already exist!\n");
	ifile->size++;
	inodes[inum]=InitInode(inum,type);
	
	
	return 0;
}
/*-
	DIC_CREATE
_*/
int FileCreateByName(char *name,int type){
    Dirp root=Dic_Path_Search(name,&dir);
    
    //----------------->Tokenize the file name
    char * ptr=name;
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
	//1--------------->handle invalid
	if(root->entry[root->num].inum!=-10)
		Error_handler("Error from FILECREATEBYNAME: the dic entry have already full!\n");
	int i;
        for(i=0;i<10;i++){
                if(strcmp(root->entry[i].name,name)==0)
			Error_handler("Error from FILECREATEBYNAME: the file name aleary exist!\n");
        }

	File_Create(ifile->size,type);
    //If the file created is a dictionary, create the dic file array
    if(type==2){
        Dirp temp=malloc(sizeof(Dir));
        InitDir(temp,ptr,ifile->size-1);
        File_Write(ifile->size-1,0,sizeof(Dir),temp);
    }
	root->entry[root->num].inum=ifile->size-1;
    
    
    
   // printf("FILECREAEDBYNAME: the file name created is %s\n",ptr);
    // printf("FILECREAEDBYNAME: the file inum created is %d\n",ifile->size-1);
    // printf("FILECREAEDBYNAME: the file entry created is %d\n",root->num);
    
    
	strcpy(root->entry[root->num].name,ptr);
	root->num++;
    if(root->inum!=-2){
        File_Write(root->inum,0,sizeof(Dir),root);
        //printf("FILE CREATED BY NAME:THe ROOT INUM IS:%d\n",root->inum);
    }
	
	return 0;	
}

/*
    IMPLEMENTED IN PHASE TWO,RETURN THE POINTER CURRENT DIC
*/
void *Dic_Path_Search(char *path,Dirp root){
    char* dup=strdup(path);
    char* dicPath=dup;
    char* ptr2=dup;
    char* ptr=path;
   
    while(*ptr!='/'&&*ptr!='\0'){
        ptr++;
        ptr2++;
    }
    //Hit the bottom of dic tree, return parent dic
    if(*ptr=='\0'){
        /*
        int inum=Dic_File_Search(dicPath,root);
        if(inum==-100)
            return root;
        //FIND THE PATH OF A DIC
        if(inodes[inum]->type==2){
            void *buffer=malloc(1024);
            File_Read(inum,0,1024,buffer);
            Dirp cur=(Dirp)buffer;
            return cur;
        }*/
        
        return root;
    }
    //Current dic name=path
    *ptr2='\0';
    int inum=Dic_File_Search(dicPath,root);
    if(inum==-100){
        printf("ERROR FROM DIC_PATH_SEARCH: NO SUCH FILE OR DICTIONARY:%s\n",dicPath);
        exit(1);
    }
    //Read dic
   // printf("DIC_PATH_SEACTH: the inum found is :%d\n",inum);
    void *buffer=malloc(1024);
    File_Read(inum,0,1024,buffer);
    Dirp cur=(Dirp)buffer;
    //printf("\n\nDIC_PATH_SEARCH: the next root is %s\n\n",dicPath);
    int i;
    for(i=0;i<10;i++){
        if(cur->entry[i].inum!=-10){
            
            printf("%s ",cur->entry[i].name);
        }
    }
     printf("\n");
    return Dic_Path_Search(ptr+1,cur);
    
}
//USED BY READDIR
void *Dic_Path_Search2(char *path,Dirp root){
    // printf("\n\nDIC_pATH_SEARCH2...... START, IN ROOT:%s, SEARCH:%s",root->name,path);
    if(strcmp(path,"\0")==0)
       return root;
   // printf("\n\nDIC_PATH_SEARCH2........\n\n");
    char* dup=strdup(path);
    char* dicPath=dup;
    char* ptr2=dup;
    char* ptr=path;
   
    while(*ptr!='/'&&*ptr!='\0'){
        ptr++;
        ptr2++;
    }
    //Hit the bottom of dic tree, return parent dic
    if(*ptr=='\0'){
        // printf("\n\nDIC_pATH_SEARCH2: WHATTTTTTTTTT......\n\n");
        int inum=Dic_File_Search(dicPath,root);
        if(inum==-100){
            //printf("\n\nDIC_pATH_SEARCH2: FILE NOT FOUND: %s\n\n",dicPath);
            return root;
        }
        
        //FIND THE PATH OF A DIC
        
           
            void *buffer=malloc(1024);
            File_Read(inum,0,1024,buffer);
            Dirp cur=(Dirp)buffer;
            // printf("\n\nDIC_pATH_SEARCH2: FILE FIND: %s\n\n",cur->name);
            return cur;
        
        
    
    }
    //Current dic name=path
    *ptr2='\0';
    int inum=Dic_File_Search(dicPath,root);
    if(inum==-100){
        printf("ERROR FROM DIC_PATH_SEARCH: NO SUCH FILE OR DICTIONARY:%s\n",dicPath);
        exit(1);
    }
    //Read dic
   // printf("DIC_PATH_SEACTH: the inum found is :%d\n",inum);
    void *buffer=malloc(1024);
    File_Read(inum,0,1024,buffer);
    Dirp cur=(Dirp)buffer;
    //printf("\n\nDIC_PATH_SEARCH: the next root is %s\n\n",dicPath);
    int i;
    for(i=0;i<10;i++){
        if(cur->entry[i].inum!=-10){
            
            printf("%s ",cur->entry[i].name);
        }
    }
     printf("\n");
     printf("\n\nDIC_pATH_SEARCH2: NEXT LEVEL SEARCH, IN ROOT:%s, SEARCH:%s",cur->name,ptr+1);
    return Dic_Path_Search2(ptr+1,cur);
    
}
int Dic_File_Search(char *name,Dirp root){
       
        int i;
        int inum=-100;
        for(i=0;i<10;i++){
                if(strcmp(root->entry[i].name,name)==0)
                        inum=root->entry[i].inum;
        }
        return inum;
}
/*-
	DIC_WRITE
_*/
int Dic_File_Write(char *name,int offset,int size,void *buffer){
    Dirp root=Dic_Path_Search(name,&dir);
    //----------------->Tokenize the file name
    char * ptr=name;
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
    
	int i;
	int inum=-100;
	for(i=0;i<10;i++){
		if(strcmp(root->entry[i].name,ptr)==0)
			inum=root->entry[i].inum;
	}
    
	if(inum==-100){
        Error_handler("ERROR FROM DIC_FILE_WRITE: THE NAME OF FILE DOES NOT EXIST!!!");
        
    }
	File_Write(inum,offset,size,buffer);
			
	return 0;
}
/*-
	DIC_READ
_*/
int Dic_File_Read(char *name,int offset,int size,void *buffer){
    
    Dirp root=Dic_Path_Search(name,&dir);
    //----------------->Tokenize the file name
    char * ptr=name;
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
    
	   int i;
        int inum=-100;
        for(i=0;i<10;i++){
                if(strcmp(root->entry[i].name,ptr)==0)
                        inum=root->entry[i].inum;
        }
    if(inum==-100){
        Error_handler("ERROR FROM DIC_FILE_READ: THE NAME OF FILE DOES NOT EXIST!!!");
        
    }
    //Is a dictionary
    if(inodes[inum]->type==2){
        return 1;
    }
	File_Read(inum,offset,size,buffer);
	return 0;
}

/*-
	ERROR
_*/
int Error_handler(char *message){
	fprintf(stderr,"%s\n",message);
	exit(1);
}

/*-
	INITESEGMENT
_*/
Segmentp InitSegment(){
	Segmentp tail2=NULL;
	tail2=malloc(sizeof(Segment));
        tail2->super=malloc(bSize*512);
	tail2->super->sSize=sSize;
	tail2->super->offset=1;
        tail2->blocks=malloc(sSize*sizeof(void *));
        int i;
        void **temp=tail2->blocks;
        for(i=0;i<sSize;i++){
                *temp=malloc(bSize*512);
                temp++;
        }
	return tail2;
}

/*-
	INITINODE
_*/
Inodep InitInode(int inum,int type){
	Inodep ifile2=NULL;
	ifile2=malloc(sizeof(Inode));
        ifile2->inum=inum;
        ifile2->type=type;
        ifile2->size=0;
	int i;
	for(i=0;i<4;i++){
		ifile2->blocks[i].bNum=-1;
		ifile2->blocks[i].sNum=-1;
	}
	return ifile2;
}

/*-
	InitDir
_*/
void InitDir(Dirp input,char * name,int inum){
	input->num=0;
	int i;
	for(i=0;i<10;i++){
		input->entry[i].inum=-10;
	}
    strcpy(input->name,name);
    input->inum=inum;
}
/*
void printAll(){
                void *buffer=malloc(bSize*512);
                Flash_Read(flash,0,bSize,buffer);
                int l;
                printf("THIS IS FROM DIC READ DEBUG: ");
                for(l=0;l<bSize*512;l++)
                        printf("%c",((int *)buffer)[l]);
                printf("\n");

}*/
