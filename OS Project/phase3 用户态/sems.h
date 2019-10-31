/* ------------------------------------------------------------------------
 Filename:      sems.h
 Assignment:    CSC452 Phase3
 Name:          Shanrui Zhang, Chengyu Sun
 Purpode:	Data structre used in phase3
   ------------------------------------------------------------------------ */

#ifndef _SEMS_H
#define _SEMS_H
#include "phase3.h"


#define MINPRIORITY 5
#define MAXPRIORITY 1
#define MAXSEM 200
typedef struct procStruct Process;
typedef struct procStruct *procPtr;
typedef struct semaphore semaphore;

struct semaphore{
	int id;//The id of the semaphore.
	int num;//The number of the semephore.
	procPtr waitingList;//The waiting list of this semephore.
	int mailboxID;
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

struct procStruct{
    int     	    pid;
    char            input[MAXARG];
    procPtr         nextProcPtr; // to keep track of next blocked process waiting for a semaphore
    procPtr         childList; // used in terminate
    int             MboxID; // used in self blocked
    int             ParentPid; // used in terminate
    procPtr         childProcPtr;
    procPtr         nextSiblingPtr;
    procPtr         zoombieList;
    procPtr         nextZoombie;
    procPtr         Parent;
    char            name[MAXNAME];     /* process's name */
    USLOSS_Context  state;             /* current context for process */
    int             priority;
    int (* startFunc) (char *);   /* function where process begins -- launch */
    char           *stack;
    unsigned int    stacksize;
    int             status;        /* 1:READY, 2:Join BLOCKED, 3:QUIT, 4:zap blocked etc. */
    int             quitStatus;  //Used for quitStatus
    int             zap;         //0 indicate not, 1 zap
    procPtr         zappedBy;    //which process this process is zapped by
    procPtr         nextZap;
    int             time;
};





#endif
