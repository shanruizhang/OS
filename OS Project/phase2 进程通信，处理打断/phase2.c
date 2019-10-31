/* ------------------------------------------------------------------------
   Filename:	phase2.c
   Assignment:	CSC452 phase2
   Author:	Shanrui Zhang, Chenyu Sun 
   Purpose:	Implement low-level process synchornization and communication via messages
		and interrupt handlers.
   University of Arizona
   Computer Science 452

   ------------------------------------------------------------------------ */

#include <usloss.h>
#include <usyscall.h>
#include <phase1.h>
#include <phase2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <message.h>

/* ------------------------- Prototypes ----------------------------------- */
int start1 (char *);
void disableInterrupts();
void enableInterrupts();
void checkKernelMode();
void clockHandler2(int dev, void *arg);
void diskHandler(int dev, void *arg);
void termHandler(int dev, void *arg);
void nullsys(USLOSS_Sysargs *args);
void syscallHandler(int dev, void *arg);

/* -------------------------- Globals ------------------------------------- */
int haha=0;//My debug flag;
int debugflag2 = 0;
int clockTimes=0;//Check how many times a clock dev occur;
//int termNum=0;//The unit used for term dev;
//int diskNum=0;//The unit used for disk dev;
unsigned int boxPid=0;//id for mail box;
mailbox MailBoxTable[MAXMBOX];//Array of mailBoxes;
mailSlot slotTable[MAXSLOTS];//The array of mail slots;
process	ProcTable[MAXPROC];//The process table;
void (*systemCallVec[MAXSYSCALLS])(USLOSS_Sysargs *args);
//systemCallVec[MAXSYSCALLS];

/* -------------------------- Functions ----------------------------------- */

/* ------------------------------------------------------------------------
   Name - start1
   Purpose - Initializes mailboxes and interrupt vector.
             Start the phase2 test process.
   Parameters - one, default arg passed by fork1, not used here.
   Returns - one to indicate normal quit.
   Side Effects - lots since it initializes the phase2 data structures.
   ----------------------------------------------------------------------- */
int start1(char *arg)
{
    int kidPid;
    int status;

    if (DEBUG2 && debugflag2)
        USLOSS_Console("start1(): at beginning\n");

    // Initialize the mail box table, slots, & other data structures.
    USLOSS_IntVec[USLOSS_CLOCK_DEV]=clockHandler2;
    USLOSS_IntVec[USLOSS_DISK_DEV]=diskHandler;
    USLOSS_IntVec[USLOSS_TERM_DEV]=termHandler;
    USLOSS_IntVec[USLOSS_SYSCALL_INT]=syscallHandler;
    //Initialize the syscal vector
    for(int i=0;i<MAXSYSCALLS;i++){
   	systemCallVec[i]=nullsys;
    }
	
    //Create 7 I/O mailbox
    for(int i=0;i<7;i++){
        MailBoxTable[i].mboxID=i;
        MailBoxTable[i].status=0;
	MailBoxTable[i].slotsNum=0;
	MailBoxTable[i].slotsCur=0;
	MailBoxTable[i].slot_size=MAX_MESSAGE;
	MailBoxTable[i].waitingList=NULL;
	MailBoxTable[i].waitingList2=NULL;
        MailBoxTable[i].slot=NULL;
    }
     // Initialize the mail box table
    for(int i=7;i<MAXMBOX;i++){
	MailBoxTable[i].mboxID=-1;
	MailBoxTable[i].status=-1;
	MailBoxTable[i].slot=NULL;
    }
    for(int i=0;i<MAXSLOTS;i++){
	slotTable[i].slot_size=-1;
	slotTable[i].mboxID=-1;
	slotTable[i].status=-1;
    }
    for(int i=0;i<MAXPROC;i++){
	ProcTable[i].pid=-1;
	ProcTable[i].nextProcPtr=NULL;
    }
    // Initialize USLOSS_IntVec and system call handlers,

    // allocate mailboxes for interrupt handlers.  Etc... 

    // Create a process for start2, then block on a join until start2 quits
    if (DEBUG2 && debugflag2)
        USLOSS_Console("start1(): fork'ing start2 process\n");
    kidPid = fork1("start2", start2, NULL, 4 * USLOSS_MIN_STACK, 1);
    if ( join(&status) != kidPid ) {
        USLOSS_Console("start2(): join returned something other than ");
        USLOSS_Console("start2's pid\n");
    }

    return 0;
} /* start1 */


/* ------------------------------------------------------------------------
   Name - MboxCreate
   Purpose - gets a free mailbox from the table of mailboxes and initializes it 
   Parameters - maximum number of slots in the mailbox and the max size of a msg
                sent to the mailbox.
   Returns - -1 to indicate that no mailbox was created, or a value >= 0 as the
             mailbox id.
   Side Effects - initializes one element of the mail box array. 
   ----------------------------------------------------------------------- */
int MboxCreate(int slots, int slot_size)
{
	//Check if in kernel mode
	checkKernelMode();
	//DisableInterrupts
	disableInterrupts();
	 //Check if the input is valid
	if(slots<0||slot_size<0||slot_size>MAX_MESSAGE)
		return -1;
	//Check if there is mailbox avaiable from mailbox table
	if(MailBoxTable[boxPid%MAXMBOX].status==-1){
		MailBoxTable[boxPid%MAXMBOX].status=0;
		MailBoxTable[boxPid%MAXMBOX].slot=NULL;
		MailBoxTable[boxPid%MAXMBOX].mboxID=boxPid;
		MailBoxTable[boxPid%MAXMBOX].slotsNum=slots;
		MailBoxTable[boxPid%MAXMBOX].slotsCur=0;
		MailBoxTable[boxPid%MAXMBOX].slot_size=slot_size;
		MailBoxTable[boxPid%MAXMBOX].waitingList=NULL;
	        MailBoxTable[boxPid%MAXMBOX].waitingList2=NULL;

		boxPid++;
	}
	else{
		int temp2=0;
		while(MailBoxTable[boxPid%MAXMBOX].status!=-1){
			if(temp2==MAXMBOX){
				return -1;//No mailboxes are avaliable;
			}
			boxPid++;
			temp2++;
		}
		MailBoxTable[boxPid%MAXMBOX].status=0;
                MailBoxTable[boxPid%MAXMBOX].slot=NULL;
                MailBoxTable[boxPid%MAXMBOX].mboxID=boxPid;
		MailBoxTable[boxPid%MAXMBOX].slotsNum=slots;
                MailBoxTable[boxPid%MAXMBOX].slotsCur=0;
		MailBoxTable[boxPid%MAXMBOX].slot_size=slot_size;
		MailBoxTable[boxPid%MAXMBOX].waitingList=NULL;
		MailBoxTable[boxPid%MAXMBOX].waitingList2=NULL;
                boxPid++;
	}
	//EnableInterrupts
	enableInterrupts();
	return (boxPid-1)%MAXMBOX;	
} /* MboxCreate */


/* ------------------------------------------------------------------------
   Name - MboxSend
   Purpose - Put a message into a slot for the indicated mailbox.
             Block the sending process if no slot available.
   Parameters - mailbox id, pointer to data of msg, # of bytes in msg.
   Returns - zero if successful, -1 if invalid args.
   Side Effects - none.
   -----------------------------------------------------------------------ZZ */
int MboxSend(int mbox_id, void *msg_ptr, int msg_size)
{		
	//Check if in kernel mode
        checkKernelMode();
        //DisableInterrupts
        disableInterrupts();
	//If the mailbox is inactive
	if(MailBoxTable[mbox_id%MAXMBOX].status==-1){
		return -1;
	}
	//If the message size is too large for this mailbox
	if(msg_size>MailBoxTable[mbox_id%MAXMBOX].slot_size||msg_size<0){
		return -1;
	}
	 //If there is no slot in the slot table
	int sum=0;
	for(int i=0;i<MAXSLOTS;i++){
		if(sum==MAXSLOTS){
			USLOSS_Console("THere is no space in the slot table....\n");
			USLOSS_Halt(1);
		}
		if(slotTable[i].status==-1){
			break;
		}
		sum++;
	}
	//If the mail box is full, block
	if(MailBoxTable[mbox_id%MAXMBOX].slotsCur==MailBoxTable[mbox_id%MAXMBOX].slotsNum){
		if(haha)
        		USLOSS_Console("slotsCur=slotsNum\n");
		//This mailbox is speical zero-slot mailbox.
		if(MailBoxTable[mbox_id%MAXMBOX].slotsNum==0){
			if(haha)
                        USLOSS_Console("MailBoxTable[mbox_id%MAXMBOX].slotsNum==0");
			if(MailBoxTable[mbox_id%MAXMBOX].waitingList2!=NULL){
                                if(haha){
                                USLOSS_Console("before remove from waiting list....\n");
                                }
                                int pid=-1;
                                MailBoxTable[mbox_id%MAXMBOX].waitingList2=remove1(MailBoxTable[mbox_id%MAXMBOX].waitingList2,&pid);
				char *temp3=(char *)msg_ptr;
				memcpy(&ProcTable[pid%MAXPROC].data,temp3,(size_t)msg_size);
				ProcTable[pid%MAXPROC].length=msg_size;
                                unblockProc(pid);
       		 		//The mail box is released.
        			if(MailBoxTable[mbox_id%MAXMBOX].status==-1){
               		 		return -3;
        			}
				//EnableInterrupts
			        enableInterrupts();
        			//If this process is zapped return -3;
        			if(isZapped()){
                			return -3;
       				 }
        			//RETURN 0: send succeed.....;
        			return 0;
                        }
			else{
				ProcTable[getpid()%MAXPROC].pid=getpid();
                        	MailBoxTable[mbox_id%MAXMBOX].waitingList=insert1(MailBoxTable[mbox_id%MAXMBOX].waitingList,&ProcTable[getpid()%MAXPROC]);
				char *temp3=(char *)msg_ptr;
                                memcpy(&ProcTable[getpid()%MAXPROC].data,temp3,(size_t)msg_size);
				ProcTable[getpid()%MAXPROC].length=msg_size;
				blockMe(11);
				//DisableInterrupts
                                disableInterrupts();
				//The mail box is released.
                                if(MailBoxTable[mbox_id%MAXMBOX].status==-1){
                                        return -3;
                                }
                                //EnableInterrupts
                                enableInterrupts();
                                //If this process is zapped return -3;
                                if(isZapped()){
                                        return -3;
                                 }
                                //RETURN 0: send succeed.....;
                                return 0;
			}
		}
		else{
			ProcTable[getpid()%MAXPROC].pid=getpid();
			MailBoxTable[mbox_id%MAXMBOX].waitingList=insert1(MailBoxTable[mbox_id%MAXMBOX].waitingList,&ProcTable[getpid()%MAXPROC]);
			memcpy(&ProcTable[getpid()%MAXPROC].data,msg_ptr,(size_t)msg_size);
			ProcTable[getpid()%MAXPROC].length=msg_size;
			blockMe(11);//11 means this process is blocked by MboxSend
		}
	}
	//DisableInterrupts
        disableInterrupts();
	//The mail box is released.
        if(MailBoxTable[mbox_id%MAXMBOX].status==-1){
                return -3;
        }

	//If there is some process waiting for messages.
	if(MailBoxTable[mbox_id%MAXMBOX].waitingList2!=NULL){
		if(haha){
			USLOSS_Console("before remove from waiting list....\n");
		}
		int pid=-1;
		MailBoxTable[mbox_id%MAXMBOX].waitingList2=remove1(MailBoxTable[mbox_id%MAXMBOX].waitingList2,&pid);
		memcpy(&ProcTable[pid%MAXPROC].data,msg_ptr,(size_t)msg_size);
		ProcTable[pid%MAXPROC].length=msg_size;
		unblockProc(pid);
		if(haha){
                        USLOSS_Console("After UNBLOCKPROCCESS....\n");
                }
		//EnableInterrupts
	        enableInterrupts();
        	//If this process is zapped return -3;
        	if(isZapped()){
               		 return -3;
       		}
        	//RETURN 0: send succeed.....;
        	return 0;
	}
	//Normal condition
	for(int i=0;i<MAXSLOTS;i++){
		if(slotTable[i].status==-1){
			slotTable[i].nextSlot=NULL;
			slotTable[i].status=0;
			slotTable[i].mboxID=mbox_id;
			slotTable[i].slot_size=msg_size;
			//char	temp[MailBoxTable[mbox_id%MAXMBOX].slot_size];
			memcpy(slotTable[i].buffer,msg_ptr,(size_t)msg_size);
			//slotTable[i].buffer=temp;
		        MailBoxTable[mbox_id%MAXMBOX].slot=addSlot(MailBoxTable[mbox_id%MAXMBOX].slot,&slotTable[i]);
			 if(haha){
                                USLOSS_Console("buffer=%s\n",MailBoxTable[mbox_id%MAXMBOX].slot->buffer);
                        }
			MailBoxTable[mbox_id%MAXMBOX].slotsCur++;
			break;
		}
	}
	//EnableInterrupts
        enableInterrupts();
	//If this process is zapped return -3;
        if(isZapped()){
                return -3;
        }
	//RETURN 0: send succeed.....;
	return 0;
} /* MboxSend */

/* ------------------------------------------------------------------------
   Name - MboxCondSend
   Purpose - Put a message into a slot for the indicated mailbox.
	     if no slots in the mailbox or slotstable avaible, return -2
	     do not block.
   Parameters - mailbox id, pointer to data of msg, # of bytes in msg.
   Returns - zero if successful, -1 if invalid args.
   Side Effects - none.
   -----------------------------------------------------------------------ZZ */
int MboxCondSend(int mbox_id, void *msg_ptr, int msg_size)
{
        //Check if in kernel mode
        checkKernelMode();
        //DisableInterrupts
        disableInterrupts();
        //If the mailbox is inactive
        if(MailBoxTable[mbox_id%MAXMBOX].status==-1){
                return -1;
        }
        //If the message size is too large for this mailbox
        if(msg_size>MailBoxTable[mbox_id%MAXMBOX].slot_size||msg_size<0){
                return -1;
        }
         //If there is no slot in the slot table
	int flag=0;
        for(int i=0;i<MAXSLOTS;i++){
                if(slotTable[i].status==-1){
			flag=1;
                }
        }
	if(flag==0){
		return -2;
	}
        //If the mail box is full, return -2;
        if(MailBoxTable[mbox_id%MAXMBOX].slotsCur==MailBoxTable[mbox_id%MAXMBOX].slotsNum){
		//If it is a zero-slot mailbox
		if(MailBoxTable[mbox_id%MAXMBOX].slotsNum==0){
			//If there is some process waiting
			if(MailBoxTable[mbox_id%MAXMBOX].waitingList2!=NULL){
				if(haha)
					USLOSS_Console("FIND SOME  process WAITING...\n");
				int pid=-1;
                                MailBoxTable[mbox_id%MAXMBOX].waitingList2=remove1(MailBoxTable[mbox_id%MAXMBOX].waitingList2,&pid);
				char *temp3=(char *)msg_ptr;
				if(haha)
                                        USLOSS_Console("From condSend befor the process weak the value= %d\n",*temp3);
				memcpy(&ProcTable[pid%MAXPROC].data,temp3,(size_t)msg_size);
				ProcTable[pid%MAXPROC].length=msg_size;
				if(haha)
                                        USLOSS_Console("From condSend after process has been weak and the value= %d\n",ProcTable[pid%MAXPROC].data);
                                unblockProc(pid);	
				return 0;
			}
			//No process waiting, return -2;
			else{
				//if(haha)
                                  //      USLOSS_Console("No process Waiting\n");
				return -2;
			}
		}
		return -2;
        }
	//If there is some process waiting for messages.
        if(MailBoxTable[mbox_id%MAXMBOX].waitingList2!=NULL){
                if(haha){
                        USLOSS_Console("before remove from waiting list....\n");
                }
                int pid=-1;
                MailBoxTable[mbox_id%MAXMBOX].waitingList2=remove1(MailBoxTable[mbox_id%MAXMBOX].waitingList2,&pid);
                memcpy(&ProcTable[pid%MAXPROC].data,msg_ptr,(size_t)msg_size);
                ProcTable[pid%MAXPROC].length=msg_size;
                unblockProc(pid);
                //EnableInterrupts
                enableInterrupts();
                //If this process is zapped return -3;
                if(isZapped()){
                         return -3;
                }
                //RETURN 0: send succeed.....;
                return 0;
        }
	//Normal condition
        for(int i=0;i<MAXSLOTS;i++){
                if(slotTable[i].status==-1){
                        slotTable[i].nextSlot=NULL;
                        slotTable[i].status=0;
                        slotTable[i].mboxID=mbox_id;
                        slotTable[i].slot_size=msg_size;
                        //char  temp[MailBoxTable[mbox_id%MAXMBOX].slot_size];
                        memcpy(slotTable[i].buffer,msg_ptr,(size_t)msg_size);
                        //slotTable[i].buffer=temp;
                        MailBoxTable[mbox_id%MAXMBOX].slot=addSlot(MailBoxTable[mbox_id%MAXMBOX].slot,&slotTable[i]);
                         if(haha){
                                USLOSS_Console("buffer=%s\n",MailBoxTable[mbox_id%MAXMBOX].slot->buffer);
                        }
                        MailBoxTable[mbox_id%MAXMBOX].slotsCur++;
			break;
		}
        }
        //EnableInterrupts
        enableInterrupts();
        //If this process is zapped return -3;
        if(isZapped()){
                return -3;
        }
        //RETURN 0: send succeed.....;
        return 0;
}

/* ------------------------------------------------------------------------
   Name - MboxReceive
   Purpose - Get a msg from a slot of the indicated mailbox.
             Block the receiving process if no msg available.
   Parameters - mailbox id, pointer to put data of msg, max # of bytes that
                can be received.
   Returns - actual size of msg if successful, -1 if invalid args.
   Side Effects - none.
   ----------------------------------------------------------------------- */
int MboxReceive(int mbox_id, void *msg_ptr, int msg_size)
{
	//Check if in kernel mode
        checkKernelMode();
        //DisableInterrupts
        disableInterrupts();
        //If the mailbox is inactive
	//Check if the input is invalid
	if(MailBoxTable[mbox_id%MAXMBOX].status==-1){
		return -1;
	}
	//If it's a zero-slot mail box
	if(MailBoxTable[mbox_id%MAXMBOX].slotsNum==0){
	}
	else{
		if(msg_size<MailBoxTable[mbox_id%MAXMBOX].slot_size){
			return -1;
		}
	}
/*	if(MailBoxTable[mbox_id%MAXMBOX].slot!=NULL&&msg_size<MailBoxTable[mbox_id%MAXMBOX].slot_size){
		return -1;
	}*/
	//Input valid....
	//No message in the mailbox, Blocking......
	if(haha)
		USLOSS_Console("slotsCur= %d, slotsNum= %d\n",MailBoxTable[mbox_id%MAXMBOX].slotsCur,MailBoxTable[mbox_id%MAXMBOX].slotsNum);	
	if(MailBoxTable[mbox_id%MAXMBOX].slotsCur==0){
		//If the mail box is zero-slot mailbox
		if(MailBoxTable[mbox_id%MAXMBOX].slotsNum==0){
			if(MailBoxTable[mbox_id%MAXMBOX].waitingList!=NULL){
                		int pid=-1;
                		MailBoxTable[mbox_id%MAXMBOX].waitingList=remove1(MailBoxTable[mbox_id%MAXMBOX].waitingList,&pid);
				memcpy(msg_ptr,&ProcTable[pid%MAXPROC].data,(size_t)msg_size);
                		unblockProc(pid);
				//The mail box is released.
                                if(MailBoxTable[mbox_id%MAXMBOX].status==-1){
                                        return -3;
                                }
                                 //EnableInterrupts
                                enableInterrupts();
                                //If this process is zapped return -3;
                                if(isZapped()){
                                        return -3;
                                }
                                //RETURN size of the message.
				return ProcTable[pid%MAXPROC].length;

        		}
			else{
				if(haha)
					USLOSS_Console("MailBoxTable[mbox_id%MAXMBOX].waitingList=NULL\n");
				ProcTable[getpid()%MAXPROC].pid=getpid();
                        	MailBoxTable[mbox_id%MAXMBOX].waitingList2=insert1(MailBoxTable[mbox_id%MAXMBOX].waitingList2,&ProcTable[getpid()%MAXPROC]);
				blockMe(12);
				memcpy(msg_ptr,&ProcTable[getpid()%MAXPROC].data,(size_t)msg_size);
				//DisableInterrupts
			        disableInterrupts();
       		 		//The mail box is released.
       		 		if(MailBoxTable[mbox_id%MAXMBOX].status==-1){
        	       	 		return -3;
       		 		}
				 //EnableInterrupts
        			enableInterrupts();
        			//If this process is zapped return -3;
        			if(isZapped()){
                			return -3;
        			}
        			//RETURN size of the message.
				return ProcTable[getpid()%MAXPROC].length;
			}
		}
		//If the mail box is NOT zero-slot mailbox
		else{
			if(MailBoxTable[mbox_id%MAXMBOX].waitingList!=NULL){
				if(haha)
                                        USLOSS_Console("Testcase24 DEBUG.....ENTER MailBoxTable[mbox_id%MAXMBOX].waitingList!=NULL\n");
				int pid=-1;
        		        MailBoxTable[mbox_id%MAXMBOX].waitingList=remove1(MailBoxTable[mbox_id%MAXMBOX].waitingList,&pid);
				memcpy(msg_ptr,&ProcTable[pid%MAXPROC].data,ProcTable[pid%MAXPROC].length);
                		unblockProc(pid);
				//EnableInterrupts
	                        enableInterrupts();
       	                 	//If this process is zapped return -3;
                        	if(isZapped()){
					return -3;
                       		 }
                        	//RETURN size of the message.
				return ProcTable[pid%MAXPROC].length;
			}
			ProcTable[getpid()%MAXPROC].pid=getpid();
			MailBoxTable[mbox_id%MAXMBOX].waitingList2=insert1(MailBoxTable[mbox_id%MAXMBOX].waitingList2,&ProcTable[getpid()%MAXPROC]);
			blockMe(12);
			if(MailBoxTable[mbox_id%MAXMBOX].status==-1){
		                return -3;
        		}
			memcpy(msg_ptr,&ProcTable[getpid()%MAXPROC].data,(size_t)ProcTable[getpid()%MAXPROC].length);
			//EnableInterrupts
			enableInterrupts();
        		//If this process is zapped return -3;
        		if(isZapped()){
				return -3;
        		}
			//RETURN size of the message.
			return ProcTable[getpid()%MAXPROC].length;
		}
	}
	//DisableInterrupts
        disableInterrupts();
	//The mail box is released.
	if(MailBoxTable[mbox_id%MAXMBOX].status==-1){
		return -3;
	}
	//Normal case
	int output=-1;
	MailBoxTable[mbox_id%MAXMBOX].slot=deleteSlot(MailBoxTable[mbox_id%MAXMBOX].slot,msg_ptr,&output);
	MailBoxTable[mbox_id%MAXMBOX].slotsCur--;
	//Unblock the process which is waiting
	if(MailBoxTable[mbox_id%MAXMBOX].waitingList!=NULL){
		int pid=-1;
		MailBoxTable[mbox_id%MAXMBOX].waitingList=remove1(MailBoxTable[mbox_id%MAXMBOX].waitingList,&pid);
		unblockProc(pid);
	}
	//EnableInterrupts
        enableInterrupts();
	//If this process is zapped return -3;
	if(isZapped()){
		return -3;
	}
	//RETURN size of the message.
	return output;
} /* MboxReceive */
/* ------------------------------------------------------------------------
   Name - MboxCondReceive
   Purpose - Get a msg from a slot of the indicated mailbox.
	     Do not block.
   Parameters - mailbox id, pointer to put data of msg, max # of bytes that
                can be received.
   Returns - if no message is available return -2 otherwise return -1,
	     if the process is zapped, return -1; 
   Side Effects - none.
   ----------------------------------------------------------------------- */
int MboxCondReceive(int mbox_id, void *msg_ptr, int msg_size)
{
        //Check if in kernel mode
        checkKernelMode();
        //DisableInterrupts
        disableInterrupts();
        //If the mailbox is inactive
        //Check if the input is invalid
	 if(MailBoxTable[mbox_id%MAXMBOX].status==-1){
                return -1;
        }
        if(MailBoxTable[mbox_id%MAXMBOX].slot!=NULL&&msg_size<strlen(MailBoxTable[mbox_id%MAXMBOX].slot->buffer))
                return -1;
        //Input valid....
        //No message in the mailbox,return -2; 
        if(MailBoxTable[mbox_id%MAXMBOX].slotsCur==0){
		return -2;
        }
	//Normal case
        int output=-1;
        MailBoxTable[mbox_id%MAXMBOX].slot=deleteSlot(MailBoxTable[mbox_id%MAXMBOX].slot,msg_ptr,&output);
        MailBoxTable[mbox_id%MAXMBOX].slotsCur--;
        //EnableInterrupts
        enableInterrupts();
        //If this process is zapped return -3;
        if(isZapped()){
                return -3;
        }
        //RETURN size of the message.
        return output;
}
/* ------------------------------------------------------------------------
   Name - MboxRelease
   Purpose: Release a mailbox.
   Parameters - mailbox id.
   Returns - if success, return 0, if the process has been zap'd return -3. 
	     if the mailboxID is not a mailbox that is in use return -1;
   Side Effects - none.
   ----------------------------------------------------------------------- */
int MboxRelease(int mboxPid){
	//Check if in kernel mode
        checkKernelMode();
        //DisableInterrupts
        disableInterrupts();
	//Check if the mail box is inactive return -1.....
	if(MailBoxTable[mboxPid%MAXMBOX].status==-1){
		return -1;
	}	
	procPtr list1=MailBoxTable[mboxPid%MAXMBOX].waitingList;
	procPtr list2=MailBoxTable[mboxPid%MAXMBOX].waitingList2;	
	//Reset the mailbox
	MailBoxTable[mboxPid%MAXMBOX].status=-1;
	slotPtr temp2=MailBoxTable[mboxPid%MAXMBOX].slot;
	while(temp2!=NULL){
		temp2->slot_size=-1;
        	temp2->mboxID=-1;
        	temp2->status=-1;
		temp2=temp2->nextSlot;
	}
	MailBoxTable[mboxPid%MAXMBOX].mboxID=-1;
	MailBoxTable[mboxPid%MAXMBOX].slotsNum=-1;
	MailBoxTable[mboxPid%MAXMBOX].slotsCur=0;
	MailBoxTable[mboxPid%MAXMBOX].slot_size=-1;
	MailBoxTable[mboxPid%MAXMBOX].waitingList=NULL;
	MailBoxTable[mboxPid%MAXMBOX].waitingList2=NULL;
	//Unblock the process
	procPtr temp=list1;
	while(temp!=NULL){
		unblockProc(temp->pid);
		temp=temp->nextProcPtr;
	}
	temp=list2;
        while(temp!=NULL){
                unblockProc(temp->pid);
                temp=temp->nextProcPtr;
        }
	enableInterrupts();
	//If this process has been zap'd
	if(isZapped()){
		return -3;
	}
	//Normal case, return -1.
	return 0;
	
}

/* ------------------------------------------------------------------------
   Name - check_io
   Purpose - Determine if there any processes blocked on any of the
             interrupt mailboxes.
   Returns - 1 if one (or more) processes are blocked; 0 otherwise
   Side Effects - none.

   Note: Do nothing with this function until you have successfully completed
   work on the interrupt handlers and their associated mailboxes.
   ------------------------------------------------------------------------ */
int check_io(void)
{
    if (DEBUG2 && debugflag2)
        USLOSS_Console("check_io(): called\n");
    for(int i=0;i<7;i++){
	if(MailBoxTable[i].waitingList2!=NULL){
		return 1;
	}
    }
    return 0;
} /* check_io */
void disableInterrupts(){
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
}
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
void checkKernelMode(){
	//Check if the current process is in kernel mode, if not print
	//error messages and call USLOSS_HALT
	union psrValues temp;
        temp.integerPart=USLOSS_PsrGet();
        if(temp.bits.curMode!=1){
                 USLOSS_Console("Current process is in User mode, Halting...\n");
                 USLOSS_Halt(1);
        }
}
/* an error method to handle invalid syscalls */
void nullsys(USLOSS_Sysargs *args)
{
    USLOSS_Console("nullsys(): Invalid syscall %d. Halting...\n",args->number);
    USLOSS_Halt(1);
} /* nullsys */

//The clockhandler
void clockHandler2(int dev, void *arg)
{
//	if(haha)
//		USLOSS_Console("CLOCK HANDLER WORKING, BEFORE MboxCondSend....\n");
	//First check if valid
	if(dev!=USLOSS_CLOCK_DEV){
		USLOSS_Console("It is not CLOCK DEVICE!!!!\n");
		return;
	}
	//If valid,first call time slice.
	timeSlice();
	//Send to the clock I/O box.
	if(clockTimes==4){
		int statusRegister=0;
		int a=USLOSS_DeviceInput(USLOSS_CLOCK_DEV,0,&statusRegister);
		if(a==1){
			USLOSS_Console("Error when acquire status of clock...\n");
			USLOSS_Halt(1);
		}
		MboxCondSend(0,&statusRegister,4);
		clockTimes=0;
	}	
	else{
		clockTimes++;
	}
	if (DEBUG2 && debugflag2)
	USLOSS_Console("clockHandler2(): called\n");


} /* clockHandler */


void diskHandler(int dev, void *arg)
{
	long unit =(long)arg;//Cast the input 
	if(dev!=USLOSS_DISK_DEV){
		USLOSS_Console("It is not DISK DEVICE!!!!\n");
		return;
        }
	if (DEBUG2 && debugflag2)
	USLOSS_Console("diskHandler(): called\n");
	int statusRegister=0;
	int a=USLOSS_DeviceInput(USLOSS_DISK_DEV,unit,&statusRegister);
	if(haha)
		USLOSS_Console("DISK Handler return status regiester= %d\n",a);
	if(a==1){
		USLOSS_Console("Error when acquire status of DISK...\n");
		USLOSS_Halt(1);
	}
	if(haha)
		USLOSS_Console("CLOCK DISK WORKING, BEFORE MboxCondSend....\n");
	MboxCondSend(unit+5,&statusRegister,4);
} /* diskHandler */


void termHandler(int dev, void *arg)
{
	long unit =(long)arg;//Cast the input 
	if(haha)
		USLOSS_Console("input is %d\n",unit);
   	if (DEBUG2 && debugflag2)
      		USLOSS_Console("termHandler(): called\n");
	if(dev!=USLOSS_TERM_DEV){
                USLOSS_Console("It is not TERM DEVICE!!!!\n");
                return;
        }
        if (DEBUG2 && debugflag2)
        	USLOSS_Console("term Handler(): called\n");
        int statusRegister=0;
        int a=USLOSS_DeviceInput(USLOSS_TERM_DEV,unit,&statusRegister);
        if(haha)
                USLOSS_Console("TERM Handler return status regiester= %d\n",a);
        if(a==1){
                USLOSS_Console("Error when acquire status of TERM...\n");
                USLOSS_Halt(1);
        }
        MboxCondSend(unit+1,&statusRegister,4);
} /* termHandler */


void syscallHandler(int dev, void *arg)
{
   long *num=(long *)arg;
   if(*num<0||*num>MAXSYSCALLS-1){
	 USLOSS_Console("syscallHandler(): sys number %d is wrong.  Halting...\n",*num);
	 USLOSS_Halt(1);
   }
   systemCallVec[*num]((USLOSS_Sysargs *)arg);
   if (DEBUG2 && debugflag2)
      USLOSS_Console("syscallHandler(): called\n");


} /* syscallHandler */
// type = interrupt device type, unit = # of device (when more than one),
// status = where interrupt handler puts device's status register.
int waitDevice(int type, int unit, int *status){
	if(type==USLOSS_CLOCK_INT){
		if(haha)
			USLOSS_Console("ENTER WAITDEVICE ,TYPE= CLOCK..\n");
		MboxReceive(0,status,4);
	}
	else if(type==USLOSS_TERM_INT){
			MboxReceive(unit+1,status,4);
	}
	else if(type==USLOSS_DISK_INT){
                        MboxReceive(unit+5,status,4);
        }
	if(isZapped())
		return -1;
	return 0;
}

