/* ------------------------------------------------------------------------
 Filename:      Phase3.c
 Assignment:    CSC452 Phase3
 Name:          Shanrui Zhang, Chengyu Sun
 Purpose:	Design and implement a support level that creates user mode processes
		and then provides system services to the user processes.
   ------------------------------------------------------------------------ */

#include <usloss.h>
#include <usyscall.h>
#include <phase1.h>
#include <phase2.h>
#include <phase3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sems.h"
#include "libuser.h"
/* ------------------------------------------------------------------------
			FUCNTION PROTOTYPES
 ------------------------------------------------------------------------ */
void checkKernelMode();
void (*systemCallVec[MAXSYSCALLS])(USLOSS_Sysargs *args);//System call vector.
void nullsys3(USLOSS_Sysargs *args);//If the syscall number is invalid, ternimate the process.
void spawn(USLOSS_Sysargs *arg);//Spwan a new user process.
void wait(USLOSS_Sysargs *arg);//Wait for a user process.
void terminate(USLOSS_Sysargs *arg);//Terminate the current user process.
void semCreate(USLOSS_Sysargs *arg);//Create a semphore.
void semP(USLOSS_Sysargs *arg);//Do a P operation on a semephore.
void semV(USLOSS_Sysargs *arg);//Do a V operation on a semephore.
void semFree(USLOSS_Sysargs *arg);//Free a semephore.
void getTimeOfDay(USLOSS_Sysargs *arg);//Returns the value of USLOSS time-of-day clock.
void cpuTime(USLOSS_Sysargs *arg);//Returns the CPU time of the process.
void getPid(USLOSS_Sysargs *arg);//Returns the process ID of the currently running process.
void setUserMode();
void InitProcess(int pid);
void InitSemaphore(int id);
void spawnLaunch();//The actuall start function for a user process, set this process to user mode
//This insert1 is to insert element at the end of of the linklist
//Used to insert a child process into the childProc list
procPtr insert1(procPtr header,procPtr input);
//Delete a child process from the child List
procPtr delete1(procPtr header);
//Insert a process to the list of waiting list
void insert2(procPtr *waitingList,procPtr input);
//Delete a process from the waiting list 
int  delete2(procPtr *waitingList);
int  spawnReal(char *name, int (*startFunc)(char *), char *arg,
          int stacksize, int priority);//Spwan a new user process.
int  waitReal(int *status);//Wait for a user process.
void terminateReal(int pid);//Terminate the current user process.
int  semCreateReal(int num);//Create a semphore.
void semPReal(int input);//Do a P operation on a semephore.
void semVReal(int input);//Do a V operation on a semephore.
int  semFreeReal(int input);//Free a semephore.
void getTimeOfDayReal(USLOSS_Sysargs *arg);//Returns the value of USLOSS time-of-day clock.
void cpuTimeReal(USLOSS_Sysargs *arg);//Returns the CPU time of the process.
void getPidReal(USLOSS_Sysargs *arg);//Returns the process ID of the currently running process.

/* ------------------------------------------------------------------------
			GLOBAL VARIABLES
 ------------------------------------------------------------------------ */
Process ProcessTable[MAXPROC];//Process table
semaphore SemTable[MAXSEM];//Semaphore table
void (*systemCallVec[MAXSYSCALLS])(USLOSS_Sysargs *args);//System call vector.
int myFlag=0;//My debug variable.
/* ------------------------------------------------------------------------
			FUNCTION IMPLEMENTATION
 ------------------------------------------------------------------------ */

int
start2(char *arg)
{
	int pid;
	int status;
    /*
     * Check kernel mode here.
     */
	checkKernelMode();
    /*
     * Data structure initialization as needed...
     */
	//First fill the syscall vector with nullsys3 function.
	for(int i=0;i<USLOSS_MAX_SYSCALLS;i++){
		systemCallVec[i]=nullsys3;
	}
	//Then fill the specific syscall function
	systemCallVec[SYS_SPAWN]=spawn;
	systemCallVec[SYS_WAIT]=wait;
	systemCallVec[SYS_TERMINATE]=terminate;
	systemCallVec[SYS_SEMCREATE]=semCreate;
	systemCallVec[SYS_SEMP]=semP;
	systemCallVec[SYS_SEMV]=semV;
	systemCallVec[SYS_SEMFREE]=semFree;
	systemCallVec[SYS_GETTIMEOFDAY]=getTimeOfDay;
	systemCallVec[SYS_CPUTIME]=cpuTime;
	systemCallVec[SYS_GETPID]=getPid;
	//Initialize the process table
	for(int i=0;i<MAXPROC;i++){
		InitProcess(i);
	}
	//Initialize the semaphore table
	for(int i=0;i<MAXSEM;i++){
		InitSemaphore(i);
	}		
    /*
     * Create first user-level process and wait for it to finish.
     * These are lower-case because they are not system calls;
     * system calls cannot be invoked from kernel mode.
     * Assumes kernel-mode versions of the system calls
     * with lower-case names.  I.e., Spawn is the user-mode function
     * called by the test cases; spawn is the kernel-mode function that
     * is called by the syscallHandler (via the systemCallVec array);s
     * spawnReal is the function that contains the implementation and is
     * called by spawn.
     *
     * Spawn() is in libuser.c.  It invokes USLOSS_Syscall()
     * The system call handler calls a function named spawn() -- note lower
     * case -- that extracts the arguments from the sysargs pointer, and
     * checks them for possible errors.  This function then calls spawnReal().
     *
     * Here, start2 calls spawnReal(), since start2 is in kernel mode.
     *
     * spawnReal() will create the process by using a call to fork1 to
     * create a process executing the code in spawnLaunch().  spawnReal()
     * and spawnLaunch() then coordinate the completion of the phase 3
     * process table entries needed for the new process.  spawnReal() will
     * return to the original caller of Spawn, while spawnLaunch() will
     * begin executing the function passed to Spawn. spawnLaunch() will
     * need to switch to user-mode before allowing user code to execute.
     * spawnReal() will return to spawn(), which will put the return
     * values back into the sysargs pointer, switch to user-mode, and 
     * return to the user code that called Spawn.
     */
	pid = spawnReal("start3", start3, NULL, USLOSS_MIN_STACK, 3);

    /* Call the waitReal version of your wait code here.
     * You call waitReal (rather than Wait) because start2 is running
     * in kernel (not user) mode.
     */
	pid = waitReal(&status);
	return pid;

} /* start2 */

void checkKernelMode(){
	// turn the interrupts OFF iff we are in kernel mode
	// if not in kernel mode, print an error message and
	// halt USLOSS
        union psrValues temp1;
        temp1.integerPart=USLOSS_PsrGet();
        if(temp1.bits.curMode!=1){
                 USLOSS_Console("Current process is in User mode, Halting...\n");
                 USLOSS_Halt(1);
        }
}
//Set the current process back to user mode.
void setUserMode(){
	 if(USLOSS_PsrSet( USLOSS_PsrGet() & ~USLOSS_PSR_CURRENT_MODE)==USLOSS_ERR_INVALID_PSR){
                USLOSS_Console("Invalid PSR....\n");
                USLOSS_Halt(1);
	}
}
//If the syscall number is invalid, ternimate the process.
void nullsys3(USLOSS_Sysargs *args){
	terminateReal(-1);
}
//Spwan a new user process.
void spawn(USLOSS_Sysargs *arg){
	//get variables from syscall arg
	char *name=arg->arg5;
	int (*startFunc)(char *)=arg->arg1;
	int stacksize=(int)((long)arg->arg3);
	int priority=(int)((long)arg->arg4);
	char *input=arg->arg2;
	//Check if the input is valid
	if(priority>MINPRIORITY||priority<MAXPRIORITY||stacksize<USLOSS_MIN_STACK||startFunc==NULL){
		arg->arg4=(void *)((long)(-1));
		return;
	}
	//If valid,call spawnReal and set return variables to sys arg
	int kidpid=spawnReal(name,startFunc,input,stacksize,priority);		
	arg->arg1=(void *)((long)kidpid);
	arg->arg4=(void *)((long)(0));
	setUserMode();
	return;
}
//spawnReal, which is called by the kernel mode process, the purpose is to create a new User level process.
int spawnReal(char *name, int (*startFunc)(char *), char *arg,
         int stacksize, int priority){
	int kidpid=fork1(name,(void *)spawnLaunch,arg,stacksize,priority);
	//If fork one returns -1
	if(kidpid==-1){
		return -1;
	}
	//Fill the process table;
	ProcessTable[kidpid%MAXPROC].pid=kidpid;
	ProcessTable[kidpid%MAXPROC].priority=priority;
	ProcessTable[kidpid%MAXPROC].startFunc=startFunc;
	ProcessTable[kidpid%MAXPROC].ParentPid=getpid();
	ProcessTable[kidpid%MAXPROC].ParentPid=getpid();
	if (arg != NULL)
        	memcpy(ProcessTable[kidpid%MAXPROC].input, arg, strlen(arg));
	ProcessTable[getpid()%MAXPROC].childList=insert1(ProcessTable[getpid()%MAXPROC].childList, &ProcessTable[kidpid % MAXPROC]);
	//If the new created child user mode process's priority is higher than the current one,let the child process run first.Because we don't have block function, use private mailbox to do this.
	if (ProcessTable[kidpid % MAXPROC].priority < ProcessTable[getpid() % MAXPROC].priority)
        	MboxSend(ProcessTable[kidpid % MAXPROC].MboxID, NULL, 0);	
	return kidpid;
}//Spwan a new user process.
void spawnLaunch(){
	//If the current user mode process is a new create one and it's parent has not done spawn yet, wait.
	if (ProcessTable[getpid() % MAXPROC].pid == -1) {
		MboxReceive(ProcessTable[getpid()%MAXPROC].MboxID, NULL, 0);
	} 
	//If this process is zapped, which means the parent process have terminated,terminated immidieatly.
	if (isZapped()){
       		terminateReal(0);
        	return;
	}
    	//Before running ,first set to user mode.
	setUserMode();
	int res=ProcessTable[getpid() % MAXPROC].startFunc(ProcessTable[getpid() % MAXPROC].input);
	//If the process is not terminate by itself, we do terminate.
	Terminate(res);
}
	
//Wait for a user process.
void wait(USLOSS_Sysargs *arg){
	int status;
	int pid=waitReal(&status);
	arg->arg1 = (void*) ((long)pid);
	arg->arg2 = (void*) ((long)status);	
	//Go back to user mode
	setUserMode();
}
//Actual wait function, used in kernel mode.
int waitReal(int *status){
	return join(status);
}
//Terminate the current user process.
void terminate(USLOSS_Sysargs *arg){
	if(myFlag==1){
		USLOSS_Console("Now we are in terminate..../n");
	}
	//Call kernel version of terminate.
	terminateReal((long)arg->arg1);
	setUserMode();
}
void terminateReal(int status){
	 if(myFlag==1){
                USLOSS_Console("Now we are in terminateReal..../n");
        }
	//First terminate all it's children by zap.
	procPtr ptr=ProcessTable[getpid() % MAXPROC].childList;
	while(ptr!=NULL){
		int pid=ptr->pid;
		zap(pid);
		ptr=ProcessTable[getpid() % MAXPROC].childList;
	}
	//Clear from the childList of parent.
	int parentPid=ProcessTable[getpid() % MAXPROC].ParentPid;
	ProcessTable[parentPid % MAXPROC].childList=delete1(ProcessTable[parentPid % MAXPROC].childList);
	InitProcess(getpid());
	quit(status);
}		
//Create a semphore.
void semCreate(USLOSS_Sysargs *arg){
	//get the initial value
	 int num = (int)((long)arg->arg1);
	//If the input is invalid
	if (num < 0){
        	arg->arg4 = (void*)((long) -1);
        	return;
	}
	int semID = semCreateReal(num);
	//No slots avaliable!!
	if(semID==-10)
		arg->arg4 = (void*)((long) -1);
	//Suceed!
	else{
		arg->arg1 = (void*)(long) semID;
        	arg->arg4 = (void*)(long) 0;
	}
	setUserMode();
}
int semCreateReal(int value){
	//FInd a empty slot
	int temp=0;
	while(SemTable[temp].id!=-1){
		temp++;
		if(temp==MAXSEM)
			return -10;
	}
	//Fill all fields
	SemTable[temp].id=temp;
	SemTable[temp].num=value;
	SemTable[temp].mailboxID=MboxCreate(1, 0);
	return temp;	
}
	
//Do a P operation on a semephore.
void semP(USLOSS_Sysargs *arg){
    	//take the id of semephore being handled
    	int ID = (int)((long)arg->arg1);
    	//Invalid handle
    	if (ID < 0||ID >= MAXSEM){
		arg->arg4 =(void*)((long) -1);
		return;
	}
    	// call kernel version of semP.
    	semPReal(ID);
	arg->arg4 =(void*)((long) 0);
	// Set back to user mode
	setUserMode();
}
void semPReal(int input){
	//Protect shared data by mutul exclusive using 1 slot mailbox to ensure there is at most one process access the same semephore at the same time.
	MboxSend(SemTable[input].mailboxID, NULL, 0);
	//If there is at least one semephore
	if (SemTable[input].num > 0){
		SemTable[input].num --;
		MboxReceive(SemTable[input].mailboxID, NULL, 0);
	}
	//Else if the current number of semepore is zero, we need to block on this semephore until one semephore comes in.
	else{
        	MboxReceive(SemTable[input].mailboxID, NULL, 0);
        	//Put current process on this semephore's waiting list.
        	insert2(&SemTable[input].waitingList, &ProcessTable[getpid() % MAXPROC]);
       		//Block me by receive from private mailbox
		int res = 0;
        	MboxReceive(ProcessTable[getpid() % MAXPROC].MboxID, &res, sizeof(int)); 
        	//Awoke, if it's awoke because the semephore have been freed,terminate.
		if (res == -1)
			terminateReal(1);
    	}
}	
//Do a V operation on a semephore.
void semV(USLOSS_Sysargs *arg){
	int semID = (int)((long)arg->arg1);
	//If the handle is invalid.
	if (semID < 0 || semID >= MAXSEM){
		arg->arg4 = (void*)((long) -1);
		return;
	}
	//Call the kernel version of semVReal.
	semVReal(semID);
	arg->arg4 = (void*)((long) 0);
    	//Set back to user mode
    	setUserMode();
}
void semVReal(int input){
	//Protect shared data by mutul exclusive using 1 slot mailbox to ensure there is at most one process access the same semephore at the same time.
	MboxSend(SemTable[input].mailboxID, NULL, 0);
	//If there is process wait for semephore, awoke them
	if (SemTable[input].waitingList != NULL){
		//Put the process waiting out of the waiting list.
        	int pid=delete2(&SemTable[input].waitingList);
		//Awoke the process
        	MboxSend(ProcessTable[pid % MAXPROC].MboxID, NULL, 0);
	}
	//If there is no process wait,just add the semephore num by one.
	else{
		SemTable[input].num++;
	}
    	MboxReceive(SemTable[input].mailboxID, NULL, 0);
}//Do a V operation on a semephore.
//Free a semephore.
void semFree(USLOSS_Sysargs *arg){
	//Acquire the id.
	int semID = (int)((long)arg->arg1);
	//IF the handle is invlid.
	if (semID < 0 || semID >= MAXSEM){
		arg->arg4 = (void*)((long) -1);
		return;
	}
	//Call the kernel version of semFreeReal.
	int res = semFreeReal(semID);
	arg->arg4 = (void*)((long) res);
	//Set back to user mode.
	setUserMode();
}
int semFreeReal(int input){
	//If there is some process waiting
	if(SemTable[input].waitingList!=NULL){
		procPtr ptr=SemTable[input].waitingList;
		while(ptr!=NULL){
			int PID =ptr->pid;
			ptr =ptr->nextProcPtr;
			//Awoke the blocked process
			int msg = -1;
			MboxSend(ProcessTable[PID].MboxID, &msg, sizeof(int));
		}
		InitSemaphore(input);
		return 1;
	}
	//No process waiting
	else{
		InitSemaphore(input);
                return 0;
	}	
}

//Returns the value of USLOSS time-of-day clock.
void getTimeOfDay(USLOSS_Sysargs *arg){
	int res;
        int temp=USLOSS_DeviceInput(USLOSS_CLOCK_DEV,0,&res);
        temp++;
	arg->arg1 = (void*) ((long)(res)) ;
}
//Returns the CPU time of the process.
void cpuTime(USLOSS_Sysargs *arg){
	 arg->arg1 = (void*) (long) readtime();
}
//Returns the process ID of the currently running process.
void getPid(USLOSS_Sysargs *arg){
	 arg->arg1 = (void*) (long) getpid();
}
void InitProcess(int pid){
	ProcessTable[pid%MAXPROC].pid=-1;
	ProcessTable[pid%MAXPROC].priority=-1;
	ProcessTable[pid%MAXPROC].stacksize=-1;
	ProcessTable[pid%MAXPROC].ParentPid=-1;
	ProcessTable[pid%MAXPROC].nextSiblingPtr=NULL;
	ProcessTable[pid%MAXPROC].MboxID=MboxCreate(0,MAX_MESSAGE);
	ProcessTable[pid%MAXPROC].childList=NULL;
	ProcessTable[pid%MAXPROC].nextProcPtr=NULL;
	ProcessTable[pid%MAXPROC].startFunc=NULL;
}
//Insert a process into the list of children list
procPtr insert1(procPtr header,procPtr input){
        //If there is no process in the child list.
        if(header==NULL){
                header=input;
        }
        //If header is not null
        else{
                procPtr ptr=header;
                while(ptr->nextSiblingPtr!=NULL){
                        ptr=ptr->nextSiblingPtr;
                }
                ptr->nextSiblingPtr=input;
        }
        return header;
}
//Delete the first child process from the parent child list
procPtr delete1(procPtr header){
        if(header==NULL){
                return NULL;
        }
        procPtr temp=header->nextSiblingPtr;
        header->nextSiblingPtr=NULL;
        header=temp;
        return header;
}
void InitSemaphore(int num){
	SemTable[num%MAXSEM].id=-1;
	SemTable[num%MAXSEM].num=-1;
	SemTable[num%MAXSEM].waitingList=NULL;
	SemTable[num%MAXSEM].mailboxID=-1;
}
//Insert a process to the list of waiting list
void insert2(procPtr *waitingList,procPtr input){
        if(*waitingList==NULL){
                *waitingList=input;
        }
        else{
                procPtr ptr=*waitingList;
                while(ptr->nextProcPtr!=NULL){
                        ptr=ptr->nextProcPtr;
                }
                ptr->nextProcPtr=input;
        }
}
//Delete a process from the waiting list 
int delete2(procPtr *waitingList)
{
    if (*waitingList == NULL)
    {
        USLOSS_Console("waitingList is NULL");
        return -1;
    }
    // Delete process at the head of the waiting list
    procPtr ptr= *waitingList;
    *waitingList =ptr->nextProcPtr;
    return ptr->pid;
}

