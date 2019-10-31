/* Patrick's DEBUG printing constant... */
/* ------------------------------------------------------------------------
 Filename:      Kernel.h
 Assignment:    CSC452 Phase1
 Name:          Shanrui Zhang, Chengyu Sun
 Purppose:      Use USLOSS tool to implement low-level CPU scheduling, create process
                join and quit process and zap and unzap process.
   ------------------------------------------------------------------------ */
#ifndef _KERNEL_H
#define _KERNEL_H
#define DEBUG 0

#include <usloss.h>
#include "phase1.h"
typedef struct procStruct procStruct;

typedef struct procStruct * procPtr;

struct procStruct {
   procPtr         nextProcPtr;
   procPtr         childProcPtr;
   procPtr         nextSiblingPtr;
   procPtr	   zoombieList;
   procPtr	   nextZoombie;
   procPtr	   Parent;
   char            name[MAXNAME];     /* process's name */
   char            startArg[MAXARG];  /* args passed to process */
   USLOSS_Context  state;             /* current context for process */
   short           pid;               /* process id */
   int             priority;
   int (* startFunc) (char *);   /* function where process begins -- launch */
   char           *stack;
   unsigned int    stackSize;
   int             status;        /* 1:READY, 2:Join BLOCKED, 3:QUIT, 4:zap blocked etc. */
   int 		   quitStatus;	//Used for quitStatus
   int		   zap;		//0 indicate not, 1 zap
   procPtr	   zappedBy;	//which process this process is zapped by
   procPtr	   nextZap;	
   int 		   time;
   /* other fields as needed... */
};

struct psrBits {
    unsigned int curMode:1;
    unsigned int curIntEnable:1;
    unsigned int prevMode:1;
    unsigned int prevIntEnable:1;
    unsigned int unused:28;
};

union psrValues {
   struct psrBits bits;
   unsigned int integerPart;
};
//This insert2 is to insert element at the end of of the linklist
//Used to insert a child process into the childProc list
procPtr insert2(procPtr header,procPtr input);
//This insert1 is to insert process to the ReadyList,insert in the order of priority.
procPtr insert1(procPtr header,procPtr input);
//This insert3 is to insert procPtr in to the zoombie list
procPtr insert3(procPtr header,procPtr input);
//This  insert4 is for insert zapp list
procPtr insert4(procPtr header,procPtr input);
//Remove the head of the zoombie list.
procPtr removeZoombieHead(procPtr header);
//Remove by pid in the list of children.
procPtr removeByPid(procPtr header,short pid);
void delete1(procPtr header,procPtr input);
procPtr removeHead(procPtr header);
void printList(procPtr header);
void printList2(procPtr header);
/* Some useful constants.  Add more as needed... */
#define NO_CURRENT_PROCESS NULL
#define MINPRIORITY 5
#define MAXPRIORITY 1
#define SENTINELPID 1
#define SENTINELPRIORITY (MINPRIORITY + 1)




#endif
