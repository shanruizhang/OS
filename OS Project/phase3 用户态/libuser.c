/*
 *  File:  libuser.c
 *
 *  Description:  This file contains the interface declarations
 *                to the OS kernel support package.
 *
 */
/* ------------------------------------------------------------------------
 Filename:      libuser.c
 Assignment:    CSC452 Phase3
 Name:          Shanrui Zhang, Chengyu Sun
 Purpose:	The user mode process's function's interface
   ------------------------------------------------------------------------ */


#include <usloss.h>
#include <usyscall.h>
#include <phase1.h>
#include <phase2.h>
#include <phase3.h>
#include <libuser.h>

#define CHECKMODE {    \
    if (USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE) { \
        USLOSS_Console("Trying to invoke syscall from kernel\n"); \
        USLOSS_Halt(1);  \
    }  \
}

/*
 *  Routine:  Spawn
 *
 *  Description: This is the call entry to fork a new user process.
 *
 *  Arguments:    char *name    -- new process's name
 *                PFV func      -- pointer to the function to fork
 *                void *arg     -- argument to function
 *                int stacksize -- amount of stack to be allocated
 *                int priority  -- priority of forked process
 *                int  *pid     -- pointer to output value
 *                (output value: process id of the forked process)
 *
 *  Return Value: 0 means success, -1 means error occurs
 *
 */
int Spawn(char *name, int (*func)(char *), char *arg, int stack_size, 
    int priority, int *pid)   
{
    USLOSS_Sysargs sysArg;
    
    CHECKMODE;
    sysArg.number = SYS_SPAWN;
    sysArg.arg1 = (void *) func;
    sysArg.arg2 = arg;
    sysArg.arg3 = (void *)(long)(stack_size);
    sysArg.arg4 = (void *)(long)priority;
    sysArg.arg5 = name;

    USLOSS_Syscall(&sysArg);

    *pid = (int)((long)sysArg.arg1);
    return (int)((long)sysArg.arg4);
} /* end of Spawn */


/*
 *  Routine:  Wait
 *
 *  Description: This is the call entry to wait for a child completion
 *
 *  Arguments:    int *pid -- pointer to output value 1
 *                (output value 1: process id of the completing child)
 *                int *status -- pointer to output value 2
 *                (output value 2: status of the completing child)
 *
 *  Return Value: 0 means success, -1 means error occurs
 *
 */
int Wait(int *pid, int *status)
{
    USLOSS_Sysargs sysArg;
    
    CHECKMODE;
    sysArg.number = SYS_WAIT;

    USLOSS_Syscall(&sysArg);

    *pid = (int)((long)sysArg.arg1);
    *status = (int)((long)sysArg.arg2);
    return (int)((long)sysArg.arg4);
    
} /* end of Wait */


/*
 *  Routine:  Terminate
 *
 *  Description: This is the call entry to terminate 
 *               the invoking process and its children
 *
 *  Arguments:   int status -- the commpletion status of the process
 *
 */
void Terminate(int status)
{
 	USLOSS_Sysargs sysArg;
	CHECKMODE;
	sysArg.number = SYS_TERMINATE;
	sysArg.arg1 = (void *) (long)((int)status);
	USLOSS_Syscall(&sysArg);    
} /* end of Terminate */


/*
 *  Routine:  SemCreate
 *
 *  Description: Create a semaphore.
 *
 *  Arguments:
 *
 */
int SemCreate(int value, int *semaphore)
{
	USLOSS_Sysargs sysArg;
	CHECKMODE;
	sysArg.number = SYS_SEMCREATE;
	sysArg.arg1 = (void *) (long)((int) value);
	USLOSS_Syscall(&sysArg);
	*semaphore = (int)((long)sysArg.arg1);
	return (int)((long)sysArg.arg4);
} /* end of SemCreate */


/*
 *  Routine:  SemP
 *
 *  Description: "P" a semaphore.
 *
 *  Arguments:
 *
 */
int SemP(int semaphore)
{
	USLOSS_Sysargs sysArg;
	CHECKMODE;
	sysArg.number = SYS_SEMP;
	sysArg.arg1 = (void *) (long)((int) semaphore);
	USLOSS_Syscall(&sysArg);
	return (int)((long)sysArg.arg4);
} /* end of SemP */


/*
 *  Routine:  SemV
 *
 *  Description: "V" a semaphore.
 *
 *  Arguments:
 *
 */
int SemV(int semaphore)
{
	USLOSS_Sysargs sysArg;
	CHECKMODE;
	sysArg.number = SYS_SEMV;
	sysArg.arg1 = (void *) (long)((int) semaphore);
	USLOSS_Syscall(&sysArg);    
	return (int)((long)sysArg.arg4);
} /* end of SemV */


/*
 *  Routine:  SemFree
 *
 *  Description: Free a semaphore.
 *
 *  Arguments:
 *
 */
int SemFree(int semaphore)
{
	USLOSS_Sysargs sysArg;
	CHECKMODE;
	sysArg.number = SYS_SEMFREE;
	sysArg.arg1 = (void *) (long)((int) semaphore);
	USLOSS_Syscall(&sysArg);
	return (int)((long)sysArg.arg4);
} /* end of SemFree */


/*
 *  Routine:  GetTimeofDay
 *
 *  Description: This is the call entry point for getting the time of day.
 *
 *  Arguments:
 *
 */
void GetTimeofDay(int *tod)                           
{
	USLOSS_Sysargs sysArg;    
	CHECKMODE;
	sysArg.number = SYS_GETTIMEOFDAY;
	USLOSS_Syscall(&sysArg);
	*tod = (int)((long)sysArg.arg1);
} /* end of GetTimeofDay */


/*
 *  Routine:  CPUTime
 *
 *  Description: This is the call entry point for the process' CPU time.
 *
 *  Arguments:
 *
 */
void CPUTime(int *cpu)                           
{
	USLOSS_Sysargs sysArg;
	CHECKMODE;
	sysArg.number = SYS_CPUTIME;
	USLOSS_Syscall(&sysArg);
	*cpu = (int)((long)sysArg.arg1);
} /* end of CPUTime */


/*
 *  Routine:  GetPID
 *
 *  Description: This is the call entry point for the process' PID.
 *
 *  Arguments:
 *
 */
void GetPID(int *pid)                           
{
	USLOSS_Sysargs sysArg;
	CHECKMODE;
	sysArg.number = SYS_GETPID;
	USLOSS_Syscall(&sysArg);
	*pid = (int)((long)sysArg.arg1);
} /* end of GetPID */

/* end libuser.c */
