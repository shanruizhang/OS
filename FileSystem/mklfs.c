//Filename:mklfs.c
//Author:Shanruizhang
//Purpose: Create and format a flash for LFS

#include<stdio.h>
#include<stdlib.h>
#include "flash.h"
#include<string.h>
#include "log.h"

int debug=0;
int main(int argc, char **argv){
/*-----------------------------
	SET PREMETERS
------------------------------*/
	
	//Default values for flash of LFS
	int bSize=2;
	int sSize=32;
	int fSize=100;
	int wear=1000;
	//User setting,not handle invalid syntax
	int i;
	for(i=1;i<argc-1;i++){
		if(strcmp(argv[i],"-b")==0){
			i++;
			bSize=atoi(argv[i]);
			continue;
		}	
		if(strcmp(argv[i],"-l")==0){
			i++;
			sSize=atoi(argv[i]);
			if(sSize*bSize%16!=0){
				fprintf(stderr,"Segment size must be multiple of the flash erase block!!\n");
				return 1;
			}
			continue;
		}	
		if(strcmp(argv[i],"-s")==0){
			i++;
			fSize=atoi(argv[i]);
			continue;
		}	
		if(strcmp(argv[i],"-w")==0){
			i++;
			wear=atoi(argv[i]);
			continue;
		}	
	}
	if(debug)
		printf("bSize=%d,sSize=%d,fSize=%d,wear=%d\n",bSize,sSize,fSize,wear);	
/*-----------------------------
	CREATE FLASH
------------------------------*/
	Flash_Create(argv[argc-1],wear,(fSize*sSize*bSize)/16);
/*-----------------------------
	WRITE SUPERBLOCK
------------------------------*/
	Superp super=malloc(sizeof(Super));
	super->bSize=bSize;
	super->sSize=sSize;
	super->fSize=fSize;
	super->wear=wear;
	super->offset=1;

	u_int FlashSize;
	Flash flash;
	flash=Flash_Open(argv[argc-1],FLASH_SILENT,&FlashSize);
	if(flash==NULL){
		fprintf(stderr,"flash is NULL!\n");
	}
	void *buffer=malloc(sSize*bSize*512);
	memcpy(buffer,super,sizeof(Super));	
	int resWrite=-1;
	//char *buffer="DEBUG";
	resWrite=Flash_Write(flash,0,sSize*bSize,buffer);
	if(resWrite==1){
		fprintf(stderr,"flash write error!!\n");
		
	}
	if(debug){
		void *test=malloc(sSize*bSize*512);
		u_int readRes=-1;
		readRes=Flash_Read(flash,0,sSize*bSize,test);
		if(readRes==1){
			fprintf(stderr,"flash read error!!\n");
		}
		Superp data=((Superp)test);
		printf("bSize=%d,sSize=%d,fSize=%d,wear=%d,offset=%d\n",data->\
			bSize,data->sSize,data->fSize,data->wear,data->offset);
				
	}
	return 0;
}

