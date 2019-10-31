#include"message.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

//The purpose of this function is to add slots in to the lost of slots of a mailbox
slotPtr addSlot(slotPtr head,slotPtr input){
	if(head==NULL){
		head=input;
		return head;
	}
	else{
		slotPtr temp=head;
		while(temp->nextSlot!=NULL){
			temp=temp->nextSlot;
		}
		temp->nextSlot=input;
		return head;
	}
}
//Delete a slot from a mail box
slotPtr deleteSlot(slotPtr head,char *buffer,int *output){
	slotPtr temp=head;
	head=head->nextSlot;
	memcpy(buffer,temp->buffer,(size_t)temp->slot_size);
	*output=temp->slot_size;
	temp->nextSlot=NULL;
        temp->status=-1;
        //temp->buffer=NULL;
        temp->mboxID=-1;
        temp->slot_size=-1;
	return head;
}
//Insert a process into the waiting list of the mailbox.
procPtr insert1(procPtr header,procPtr input){
        //If there is no process in the child list.
        if(header==NULL){
                header=input;
        }
        //If header is not null
        else{
                procPtr ptr=header;
                while(ptr->nextProcPtr!=NULL){
                        ptr=ptr->nextProcPtr;
                }
                ptr->nextProcPtr=input;
        }
        return header;
}

//The process highest which has the highest priority has finished->
procPtr remove1(procPtr header,int *pid){
	procPtr temp=header;
	header=header->nextProcPtr;
	*pid=temp->pid;
	temp->pid=-1;
	temp->nextProcPtr=NULL;
	return header;
}


	
	
