/* ------------------------------------------------------------------------
 Filename:	Phase1.c
 Assignment: 	CSC452 Phase1
 Name: 		Shanrui Zhang, Chengyu Sun
 Purppose:	Use USLOSS tool to implement low-level CPU scheduling, create process
	  	join and quit process and zap and unzap process.
   ------------------------------------------------------------------------ */

#include "phase1.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "kernel.h"

/* ------------------------- Prototypes ----------------------------------- */
void illegalInstructionHandler(int dev, void *arg);
int sentinel (char *);
void dispatcher(void);
void launch();
static void checkDeadlock();
void disableInterrupts();
void enableInterrupts();
void print();
void resetProcess(procPtr input);
void resetProcess1(procPtr input);
void dumpProcesses();
void clockHandler(int dev,void *arg);
/* -------------------------- Globals ------------------------------------- */

// Patrick's debugging global variable...
int debugflag = 1;

// the process table
procStruct ProcTable[MAXPROC];

// Process lists
static procPtr ReadyList;

// current process ID
procPtr Current;

// the next pid to be assigned
unsigned int nextPid = SENTINELPID;


/* -------------------------- Functions ----------------------------------- */
/* ------------------------------------------------------------------------
   Name - startup
   Purpose - Initializes process lists and clock interrupt vector.
             Start up sentinel process and the test process.
   Parameters - argc and argv passed in by USLOSS
   Returns - nothing
   Side Effects - lots, starts the whole thing
   ----------------------------------------------------------------------- */
void startup(int argc, char *argv[])
{
//  USLOSS_Console("%d\n",readCurStartTime());
    int result; /* value returned by call to fork1() */

    /* initialize the process table */
    if (DEBUG && debugflag)
        USLOSS_Console("startup(): initializing process table, ProcTable[]\n");
    for(int i=0;i<MAXPROC;i++){
	ProcTable[i].nextProcPtr=NULL;
	ProcTable[i].childProcPtr=NULL;
	ProcTable[i].nextSiblingPtr=NULL;
	ProcTable[i].zoombieList=NULL;
	ProcTable[i].nextZoombie=NULL;
	ProcTable[i].Parent=NULL;
	ProcTable[i].name[0]='\0';
	ProcTable[i].startArg[0]='\0';
	ProcTable[i].state.start=NULL;
//	ProcTable[i].state.context
	ProcTable[i].state.pageTable=NULL;
	ProcTable[i].pid=-1;
	ProcTable[i].priority=-1;
	ProcTable[i].startFunc=NULL;
	ProcTable[i].stack=NULL;
	ProcTable[i].stackSize=USLOSS_MIN_STACK;
	ProcTable[i].status=-1;
	ProcTable[i].time=-1;
     }
    // Initialize the Ready list, etc.
    if (DEBUG && debugflag)
        USLOSS_Console("startup(): initializing the Ready list\n");
    ReadyList = NULL;
    //Initialize the Current procPtr;
    Current=NULL;

    // Initialize the illegalInstruction interrupt handler
    USLOSS_IntVec[USLOSS_ILLEGAL_INT] = illegalInstructionHandler;
    // Initialize the clock interrupt handler
//    USLOSS_IntVec[USLOSS_CLOCK_INT] = clockHandler(0,NULL);
    // startup a sentinel process
    if (DEBUG && debugflag)
        USLOSS_Console("startup(): calling fork1() for sentinel\n");
    result = fork1("sentinel", sentinel, NULL, USLOSS_MIN_STACK,
                    SENTINELPRIORITY);
    if (result < 0) {
        if (DEBUG && debugflag) {
            USLOSS_Console("startup(): fork1 of sentinel returned error, ");
            USLOSS_Console("halting...\n");
        }
        USLOSS_Halt(1);
    }
  
    // start the test process
    if (DEBUG && debugflag)
        USLOSS_Console("startup(): calling fork1() for start1\n");
    result = fork1("start1", start1, NULL, 2 * USLOSS_MIN_STACK, 1);
    if (result < 0) {
        USLOSS_Console("startup(): fork1 for start1 returned an error, ");
        USLOSS_Console("halting...\n");
        USLOSS_Halt(1);
    }

    USLOSS_Console("startup(): Should not see this message! ");
    USLOSS_Console("Returned from fork1 call that created start1\n");

    return;
} /* startup */

/* ------------------------------------------------------------------------
   Name - finish
   Purpose - Required by USLOSS
   Parameters - none
   Returns - nothing
   Side Effects - none
   ----------------------------------------------------------------------- */
void finish(int argc, char *argv[])
{
    if (DEBUG && debugflag)
        USLOSS_Console("in finish...\n");
} /* finish */

/* ------------------------------------------------------------------------
   Name - fork1
   Purpose - Gets a new process from the process table and initializes
             information of the process.  Updates information in the
             parent process to reflect this child process creation.
   Parameters - the process procedure address, the size of the stack and
                the priority to be assigned to the child process.
   Returns - the process id of the created child or -1 if no child could
             be created or if priority is not between max and min priority.
   Side Effects - ReadyList is changed, ProcTable is changed, Current
                  process information changed
   ------------------------------------------------------------------------ */
int fork1(char *name, int (*startFunc)(char *), char *arg,
          int stacksize, int priority)
{
	int procSlot = -1;

	if (DEBUG && debugflag)
		USLOSS_Console("fork1(): creating process %s\n", name);

	// Test if in kernel mode; halt if in user mode and Disable interupt
	disableInterrupts();
	// Return if stack size is too small
	if(stacksize<USLOSS_MIN_STACK){
		return -2;
	}

	//Return -1 if priority is out of range or startFunc is null or name is null
	if(name==NULL||priority>MINPRIORITY+1||priority<MAXPRIORITY||startFunc==NULL){
		return -1;
	}
	// Is there room in the process table? What is the next PID?
	if(ProcTable[nextPid%MAXPROC].status==-1){
		procSlot=nextPid%MAXPROC;
	}
	else{
		int temp=0; //Check if the table is full
		while(ProcTable[nextPid%MAXPROC].status!=-1){
			nextPid++;
			temp++;
			if(temp>MAXPROC){
				return -1;
			}
		}
		procSlot=nextPid%MAXPROC;
	}
	//USLOSS_Console("procSlot= %d\n",procSlot);
	nextPid++;	
	// fill-in entry in process table */
	////////////////////////////////////////////////////////////////////////////////

	//1: fill process name
	if ( strlen(name) >= (MAXNAME - 1) ) {
		USLOSS_Console("fork1(): Process name is too long.  Halting...\n");
		USLOSS_Halt(1);
    	}
	strcpy(ProcTable[procSlot].name, name);
	
	//2: fill function
	ProcTable[procSlot].startFunc = startFunc;

	//3: fill input arg
	if ( arg == NULL )
		ProcTable[procSlot].startArg[0] = '\0';
	else if ( strlen(arg) >= (MAXARG - 1) ) {
		USLOSS_Console("fork1(): argument too long.  Halting...\n");
		USLOSS_Halt(1);
	}
	else
		strcpy(ProcTable[procSlot].startArg, arg);

	//4: fill priority
	ProcTable[procSlot].priority=priority;

	//5: allocate stack 
	ProcTable[procSlot].stack=malloc(stacksize);
	
	//6: fill others
	ProcTable[procSlot].nextProcPtr=NULL;
	ProcTable[procSlot].childProcPtr=NULL;
	ProcTable[procSlot].nextSiblingPtr=NULL;
	ProcTable[procSlot].status=1;//The status is ready;
	ProcTable[procSlot].pid=nextPid-1;
	//7: set parrent,child,sibiling
	ProcTable[procSlot].Parent=Current;
	ProcTable[procSlot].zoombieList=NULL;
	ProcTable[procSlot].nextZoombie=NULL;
	ProcTable[procSlot].time=readCurStartTime();
	//Finish filling
	/////////////////////////////////////////////////////////////////////////
	
	// Initialize context for this process, but use launch function pointer for
	// the initial value of the process's program counter (PC)

	USLOSS_ContextInit(&(ProcTable[procSlot].state),
                       ProcTable[procSlot].stack,
                       ProcTable[procSlot].stackSize,
                       NULL,
                       launch);

	// for future phase(s)
	p1_fork(ProcTable[procSlot].pid);

	// More stuff to do here...
 	//Put the child into the child list of parent
	if(Current!=NULL){
		Current->childProcPtr=insert2(Current->childProcPtr,&ProcTable[procSlot]);
	}	
	//Put input the ready list
	ReadyList=insert1(ReadyList,&ProcTable[procSlot]);
	//USLOSS_Console("&(ProcTable[procSlot].state)= %p\n",&(ProcTable[procSlot]));
	//USLOSS_Console("&(ReadyList->state)= %p\n",ReadyList);
	//Call dispatcher
	if(procSlot!=SENTINELPID){
		dispatcher();	
		// USLOSS_ContextSwitch(NULL,&(ProcTable[procSlot].state));
	}
	//Enable interupt
	union psrValues temp;
        temp.integerPart=USLOSS_PsrGet();
        temp.bits.curIntEnable=1;
        if(USLOSS_PsrSet(temp.integerPart)==USLOSS_ERR_INVALID_PSR){
                USLOSS_Console("Invalid PSR....\n");
                USLOSS_Halt(1);
        }
	return nextPid-1;  // -1 is not correct! Here to prevent warning.
} /* fork1 */

/* ------------------------------------------------------------------------
   Name - launch
   Purpose - Dummy function to enable interrupts and launch a given process
             upon startup.
   Parameters - none
   Returns - nothing
   Side Effects - enable interrupts
   ------------------------------------------------------------------------ */
void launch()
{
    int result;
    enableInterrupts();
    if (DEBUG && debugflag)
        USLOSS_Console("launch(): started\n");

    // Enable interrupts

    // Call the function passed to fork1, and capture its return value
    result = Current->startFunc(Current->startArg);

    if (DEBUG && debugflag)
        USLOSS_Console("Process %d returned to launch\n", Current->pid);

    quit(result);

} /* launch */


/* ------------------------------------------------------------------------
   Name - join
   Purpose - Wait for a child process (if one has been forked) to quit.  If 
             one has already quit, don't wait.
   Parameters - a pointer to an int where the termination code of the 
                quitting process is to be stored.
   Returns - the process id of the quitting child joined on.
             -1 if the process was zapped in the join
             -2 if the process has no children
   Side Effects - If no child process has quit before join is called, the 
                  parent is removed from the ready list and blocked.
   ------------------------------------------------------------------------ */
int join(int *status)
{
    //Disable interrupts first
    disableInterrupts();
    //First case: No active children
    if(Current->childProcPtr==NULL&&Current->zoombieList==NULL){
	return -2;
    }
    //Second case:Children quit before parent call join.
    if(Current->zoombieList!=NULL){
	int res=Current->zoombieList->pid;
	*status=Current->zoombieList->quitStatus;
	procPtr temp=Current->zoombieList;
	Current->zoombieList=removeZoombieHead(temp);
	resetProcess(temp);						
	if(isZapped()==1){
        return -1;
    	}
	return res;
    }	
    //Third case:No() child has quit, must wait.
    Current->status=2;
//    printList(ReadyList);
    ReadyList=removeHead(ReadyList);
//    printList(ReadyList);
    dispatcher(); 
    
    //Process has returned by child process, now the zoombieList is not null.
    if(Current->zoombieList==NULL)
	USLOSS_Console("empty\n");
    int res=Current->zoombieList->pid;
    *status=Current->zoombieList->quitStatus;
    procPtr temp=Current->zoombieList;
    Current->zoombieList=removeZoombieHead(temp);
    resetProcess(temp);
    enableInterrupts();
    if(isZapped()==1){
	return -1;
    }
    else{
    	return res;
    }

} /* join */


/* ------------------------------------------------------------------------
   Name - quit
   Purpose - Stops the child process and notifies the parent of the death by
             putting child quit info on the parents child completion code
             list.
   Parameters - the code to return to the grieving parent
   Returns - nothing
   Side Effects - changes the parent of pid child completion status list.
   ------------------------------------------------------------------------ */
void quit(int status)
{
   //Disable interrupts
    disableInterrupts(); 
	
    //Case 1: If current has active children, halt the usloss
    if(Current->childProcPtr!=NULL){
	USLOSS_Console("quit(): process %d, '%s', has active children. Halting...\n",(int)Current->pid,Current->name);
	USLOSS_Halt(1);
    }
    //Clean the zoombie list
	while(Current->zoombieList!=NULL){
		procPtr temp=Current->zoombieList;
		Current->zoombieList= Current->zoombieList->nextZoombie;
		resetProcess(temp);
	}
    //Speical case: Current is start 1
    if(Current->pid==2){
	ReadyList=removeHead(ReadyList);
	resetProcess1(Current);
	if(ReadyList==NULL){
	USLOSS_Console("hehe\n");
	}
    }
    else{
    //Case 2: Parrent has join
    if(Current->Parent->status==2){
	Current->quitStatus=status;
        Current->Parent->status=1;
	Current->status=3;
	Current->Parent->zoombieList=insert3(Current->Parent->zoombieList,Current);
	Current->Parent->childProcPtr=removeByPid(Current->Parent->childProcPtr,Current->pid);	
	ReadyList=removeHead(ReadyList);
	ReadyList=insert1(ReadyList,Current->Parent);
    }
    //Case 3: Parrent has not join
    else if(Current->Parent->status!=2){
	Current->quitStatus=status;
        Current->status=3;
        ReadyList=removeHead(ReadyList);
	Current->Parent->childProcPtr=removeByPid(Current->Parent->childProcPtr,Current->pid);
	Current->Parent->zoombieList=insert3(Current->Parent->zoombieList,Current);

    }
    }
    //Check if the current process is zapped
    if(isZapped()){
	procPtr temp=Current->zappedBy;
	while(temp!=NULL){
		temp->status=1;//The process becomes Ready again.
        	ReadyList=insert1(ReadyList,temp);
		temp=temp->nextZap;
	}

    }
	
    p1_quit(Current->pid);
    //Enable interrupts
    enableInterrupts();
    dispatcher();
} /* quit */


/* ------------------------------------------------------------------------
   Name - dispatcher
   Purpose - dispatches ready processes.  The process with the highest
             priority (the first on the ready list) is scheduled to
               run.  The old process is swapped out and the new process
             swapped in.
   Parameters - none
   Returns - nothing
   Side Effects - the context of the machine is changed
   ----------------------------------------------------------------------- */
void dispatcher(void)
{
    //disable interrupts.
    disableInterrupts();
    procPtr temp=Current;
    procPtr nextProcess=ReadyList;
    if(Current==NULL){
	Current=nextProcess;
	Current->time=readCurStartTime();
	USLOSS_ContextSwitch(NULL,&(nextProcess->state));
    }
    else if(Current!=nextProcess){
	Current=nextProcess;
	Current->time=readCurStartTime();
	USLOSS_ContextSwitch(&(temp->state),&(ReadyList->state));
    }
    p1_switch(Current->pid, nextProcess->pid);
    
    //Enable interrupts
    enableInterrupts();

} /* dispatcher */


/* ------------------------------------------------------------------------
   Name - sentinel
   Purpose - The purpose of the sentinel routine is two-fold.  One
             responsibility is to keep the system going when all other
             processes are blocked.  The other is to detect and report
             simple deadlock states.
   Parameters - none
   Returns - nothing
   Side Effects -  if system is in deadlock, print appropriate error
                   and halt.
   ----------------------------------------------------------------------- */
int sentinel (char *dummy)
{
    if (DEBUG && debugflag)
        USLOSS_Console("sentinel(): called\n");
    while (1)
    {
        checkDeadlock();
        USLOSS_WaitInt();
    }
} /* sentinel */


/* check to determine if deadlock has occurred... */
static void checkDeadlock()
{
    //disable interrupts.
    disableInterrupts();
    for(int i=49;i>=2;i--){
	if(ProcTable[i].status!=-1){
	//USLOSS_Console("PID %d has status = %d\n",(int)ProcTable[i].pid,ProcTable[i].status);
		if(i==2){
			USLOSS_Console("checkDeadlock check start1 does not exit, status=%d\n",ProcTable[i].status);
			USLOSS_Halt(1);
		}
		else{
			 USLOSS_Console("checkDeadlock(): numProc = %d. Only Sentinel should be left. Halting...\n",ProcTable[i].pid);
                        USLOSS_Halt(1);
		}
		
	}
    }
	USLOSS_Console("All processes completed.\n");
	USLOSS_Halt(0);
} /* checkDeadlock */


/*
 * Disables the interrupts.
 */
void disableInterrupts()
{
    // turn the interrupts OFF iff we are in kernel mode
    // if not in kernel mode, print an error message and
    // halt USLOSS
	union psrValues temp;
        temp.integerPart=USLOSS_PsrGet();
        if(temp.bits.curMode!=1){
                 USLOSS_Console("Current process is in User mode, Halting...\n");
                 USLOSS_Halt(1);
        }
        //Disable interupt
        temp.bits.curIntEnable=0;
        if(USLOSS_PsrSet(temp.integerPart)==USLOSS_ERR_INVALID_PSR){
                USLOSS_Console("Invalid PSR....\n");
                USLOSS_Halt(1);
        }

} /* disableInterrupts */

/*
 * Enable interrupts
 */
void enableInterrupts(){
	
    // turn the interrupts On if we are in kernel mode
    // if not in kernel mode, print an error message and
    // halt USLOSS
        union psrValues temp;
        temp.integerPart=USLOSS_PsrGet();
        if(temp.bits.curMode!=1){
                 USLOSS_Console("Current process is in User mode, Halting...\n");
                 USLOSS_Halt(1);
        }
        //enable interupt
        temp.bits.curIntEnable=1;
        if(USLOSS_PsrSet(temp.integerPart)==USLOSS_ERR_INVALID_PSR){
                USLOSS_Console("Invalid PSR....\n");
                USLOSS_Halt(1);
        }
}


void illegalInstructionHandler(int dev, void *arg)
{
    if (DEBUG && debugflag)
        USLOSS_Console("illegalInstructionHandler() called\n");
} /* illegalInstructionHandler */
void resetProcess(procPtr input){
        input->nextProcPtr=NULL;
        input->childProcPtr=NULL;
        input->nextSiblingPtr=NULL;
        input->zoombieList=NULL;
        input->nextZoombie=NULL;
        input->Parent=NULL;
//      input->name="\0";
//      input->startArg="\0";
//      input->state->start=NULL;
//      input->state->context
//      input->state->pageTable=NULL;
        input->pid=-1;
        input->priority=-1;
        input->startFunc=NULL;
        free(input->stack);
        input->stackSize=USLOSS_MIN_STACK;
        input->status=-1;
	input->time=-1;
}
void resetProcess1(procPtr input){
        input->nextProcPtr=NULL;
        input->childProcPtr=NULL;
        input->nextSiblingPtr=NULL;
        input->zoombieList=NULL;
        input->nextZoombie=NULL;
	
        input->Parent=NULL;
//      input->name="\0";
//      input->startArg="\0";
//      input->state->start=NULL;
//      input->state->context
//      input->state->pageTable=NULL;
        input->pid=-1;
        input->priority=-1;
        input->startFunc=NULL;
//        free(input->stack);
        input->stackSize=USLOSS_MIN_STACK;
        input->status=-1;
	input->time=-1;
}
int blockMe(int status){
	if(status<=10){
		USLOSS_Console("BLOCKME ERROR!!! STATUS IS LESS THAN 10\n");
                USLOSS_Halt(1);
	}
	Current->status=status;
	ReadyList=removeHead(ReadyList);
	dispatcher();
	if(isZapped()){
                return -1;
        }
	return 0;
}
int unblockProc(int pid){
	//FIrst check
	int res;
	int check=ProcTable[pid%MAXPROC].status;
	if(check==-1||(check!=2&&check<=10)||&ProcTable[pid%MAXPROC]==Current){
		res=-2;
	}
	//zapped!!
	else if(isZapped()){
		res=-1;
	}	
	//Normal
	else{
		ProcTable[pid%MAXPROC].status=1;//The process becomes Ready again.
		ReadyList=insert1(ReadyList,&ProcTable[pid%MAXPROC]);
		dispatcher();
		res=0;
	}
	return res;
}
int   readtime(){
	//return USLOSS_DeviceInput();
	return -1;
}
int zap(int pid){
	//disableInterrupts
	disableInterrupts();
	//check if not valid
	if(pid==Current->pid){
		USLOSS_Console("zap(): process %d tried to zap itself. Halting...\n",Current->pid);
		USLOSS_Halt(1);
	}
	if(ProcTable[pid%MAXPROC].pid!=pid){
                USLOSS_Console("zap(): process being zapped does not exist.  Halting...\n",Current->pid);
                USLOSS_Halt(1);
        }
	//Else
	ProcTable[pid%MAXPROC].zap=1;
	ProcTable[pid%MAXPROC].zappedBy=insert4(ProcTable[pid%MAXPROC].zappedBy,Current);
	Current->status=4;
        ReadyList=removeHead(ReadyList);
        dispatcher();

	 //If the current process has already been zapped
        if(Current->zap==1)
                return -1;
	//enable interrupts
	enableInterrupts();
	return 0;
}
int isZapped(){
	disableInterrupts();
	return Current->zap;
	enableInterrupts();
}
void dumpProcesses(){
	printf("PID	Parent	Priority	Status		# Kids	CPUtime	Name\n");
	for(int i=0;i<50;i++){
		int b=0;
		procPtr ptr=ProcTable[i].childProcPtr;
		while(ptr!=NULL){
			b++;
			ptr=ptr->nextSiblingPtr;
		}
		printf("%d	",(int)ProcTable[i].pid);
		if(ProcTable[i].Parent!=NULL)
			printf("%d	",(int)ProcTable[i].Parent->pid);
		else{
			if(i==1||i==2)
				printf("-2	");
			else{
				printf("-1	");
			}
		}
		printf("%d		",ProcTable[i].priority);
		if(ProcTable[i].pid==Current->pid)
			printf("RUNNING         ");
		else if(ProcTable[i].status==-1)
			printf("EMPTY		");
		else if(ProcTable[i].status==1)
		 	printf("READY		");
		else if(ProcTable[i].status==2)
		 	printf("JOIN_BLOCK	");
		else if(ProcTable[i].status==3)
		 	printf("QUIT		");
		else if(ProcTable[i].status==4)
		 	printf("ZAP_BLOCK	");
		else{
			printf("%d		",ProcTable[i].status);
		}
		printf("%d	",b);
		printf("%d	",ProcTable[i].time);
		printf("%s\n",ProcTable[i].name);
	}
}
int   getpid(){
	return (int) Current->pid;
}
//void clockHandler(USLOSS_CLOCK_DEV,(void *)NULL){
		
//}
int   readCurStartTime(){
	int res;
	int temp=USLOSS_DeviceInput(USLOSS_CLOCK_DEV,0,&res);
	temp++;
	return res;
}
