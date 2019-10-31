//Filename:log.h
//Author:ShanruiZHang
//Purpose:Define some useful data structures used in log


/*----------------------------
	data structures
----------------------------*/
extern int bSize;//block size, in sectors
extern int sSize;//segment size, in blocks
extern int fSize;//flash size,in segments
extern int wear;//wear limit of each block
extern int offset;//off set of current segment.



typedef struct Super * Superp;
typedef struct Block * Blockp;
typedef struct Segment * Segmentp;
typedef struct logAddress * logAddressp;
typedef struct Super  Super;
typedef struct Block  Block;
typedef struct Segment  Segment;
typedef struct logAddress  logAddress;
typedef struct Inode  Inode;
typedef struct Inode  *Inodep;
typedef struct Superblock  Superblock;
typedef struct Superblock *Superblockp;
typedef struct Dir  Dir;
typedef struct Dir *Dirp;
typedef struct DirE  DirE;
typedef struct DirE *DirEp;



extern Dir dir;
extern Inodep *inodes;



struct DirE{
	char name[10];
	int inum;
};
struct Dir{
	DirE entry[10];
	int num;
    char name[10];
    int inum;
};
struct Super{
	int bSize;//block size, in sectors
	int sSize;//segment size, in blocks
	int fSize;//flash size,in segments
	int wear;//wear limit of each block
	int offset;//off set of current segment.
};

struct Segment{
	Superblockp super;
	void **blocks;
};
//super block 
struct Superblock{
	int sSize;
	int offset;
	
};

struct logAddress{
	int bNum;//block number within the Segment
	int sNum;//Segment number within the log
};
struct Inode{
	int inum;
	int type;
	int size;
	logAddress blocks[4];
};
/*----------------------------
	log functions
----------------------------*/
int Lfs_start(char *); 
int Log_Read(logAddress *address,int length,void *buffer);
int Log_Write(int inum,int block,int length,void *buffer,logAddress *address);
int Log_Free(logAddress address,int length);
/*----------------------------
	file functions
----------------------------*/
int File_Create(int inum,int type);
int File_Write(int inum,int offset,int length,void *buffer);
int File_Read(int inum,int offset,int length,void *buffer);
int File_Free(char *path);
/*----------------------------
	Dirctory functions
----------------------------*/
int Dic_File_Search(char *name,Dirp);
int FileCreateByName(char *,int);
int Dic_File_Write(char *,int offset,int size,void *buffer);
int Dic_File_Read(char *,int offset,int size,void *buffer);
void *Dic_Path_Search2(char *path,Dirp root);//USED BY READDIR
void *Dic_Path_Search(char *path,Dirp root);
void FreeAll();






/*---------------------------------------------------------
    FOR MY DEBUG FILE USE, IS HIDDEN IN ACTUALL FUSE LAYER
_________________________________________________________*/
extern Segmentp tail;
extern Inodep ifile;

extern Superp super;
extern Inodep *inodes;
extern Dir dir;//Dirctory entry