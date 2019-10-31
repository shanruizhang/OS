#define DEBUG2 1

typedef struct mailSlot *slotPtr;
typedef struct mailSlot mailSlot;
typedef struct mailbox   mailbox;
typedef struct mboxProc *mboxProcPtr;
typedef struct message message;
typedef struct message *messagePtr;
typedef struct process process;
typedef struct process *procPtr;
struct process{
	int pid;//The ID of the process
	procPtr nextProcPtr;//Next process which is in the waiting list.
	char data[150];//The message stored in the process.
	int length;//The length of the message stored in the proces.
};

struct mailbox {
    int       slotsNum;//The max slots this mail box can hold
    int       slotsCur;//How many slots are in used currently.
    int       slot_size;//The message length the slots can hold for this mail box; 
    int       mboxID;
    int       status;//-1:avaliable 0:In use  
    slotPtr   slot;//The slots in a mailBox
    procPtr   waitingList;//Waiting list of this mailbox.
    procPtr   waitingList2;//The waiting list of recieve messeages.
    
    // other items as needed...
};

struct mailSlot {
    slotPtr   nextSlot;//The pointer to the next slot in the same mail box;
    int       slot_size;//The length of message;
    int       mboxID;
    int       status;
    char buffer[150]; 
    // other items as needed...
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
//Add slot into the lost of slots of a mailbox.
slotPtr addSlot(slotPtr head,slotPtr input);
//Delete a slot from a mail box.
slotPtr deleteSlot(slotPtr head,char *buffer,int *output);
//Insert a process into the mailbox waitingList;
procPtr insert1(procPtr header,procPtr input);
//Remove the process out of the waiting list.
procPtr remove1(procPtr header,int *pid);

	
