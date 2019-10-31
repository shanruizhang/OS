/* ------------------------------------------------------------------------
 Filename:      Kernel.c
 Assignment:    CSC452 Phase1
 Name:          Shanrui Zhang, Chengyu Sun
 Purppose:      Use USLOSS tool to implement low-level CPU scheduling, create process
                join and quit process and zap and unzap process.
   ------------------------------------------------------------------------ */
#include "kernel.h"
#include <stdio.h>
//Insert a process into the propiority queue;
procPtr insert1(procPtr header,procPtr input){
	//If there is no process in the queue now
	if(header==NULL){
		 header=input;
	}
	//If the input process has the highest priority
	else if(input->priority<header->priority){
		input->nextProcPtr=header;
		header=input;
	}
		
	else{
		int curPri=input->priority;
		procPtr ptr=header;
		while(ptr->nextProcPtr!=NULL&&curPri>=ptr->nextProcPtr->priority){
			ptr=ptr->nextProcPtr;
		}
		procPtr temp=ptr->nextProcPtr;
		ptr->nextProcPtr=input;
		input->nextProcPtr=temp;
	}
	return header;
}
//Insert a process into the list of children list
procPtr insert2(procPtr header,procPtr input){
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
//Insert a process into the list of zoombie list
procPtr insert3(procPtr header,procPtr input){
        //If there is no process in the child list.
        if(header==NULL){
                header=input;
        }
        //If header is not null
        else{
                procPtr ptr=header;
                while(ptr->nextZoombie!=NULL){
                        ptr=ptr->nextZoombie;
                }
                ptr->nextZoombie=input;
        }
        return header;
}
//Insert a process into the list of children list
procPtr insert4(procPtr header,procPtr input){
        //If there is no process in the child list.
        if(header==NULL){
                header=input;
        }
        //If header is not null
        else{
                procPtr ptr=header;
                while(ptr->nextZap!=NULL){
                        ptr=ptr->nextZap;
                }
                ptr->nextZap=input;
        }
        return header;
}

//Delete the head of a zoombie list
procPtr removeZoombieHead(procPtr header){
	procPtr temp=header->nextZoombie;
        header->nextZoombie=NULL;
        header=temp;
	return header;
}

////Remove by pid in the list of children.
procPtr removeByPid(procPtr header,short pid){
	//If header is to be delete.
	if(header->pid==pid){
		header=header->nextSiblingPtr;
	}
	//Else
	else{
		procPtr ptr=header;
		while(ptr->nextSiblingPtr!=NULL&&ptr->nextSiblingPtr->pid!=pid){
			ptr=ptr->nextSiblingPtr;	
		}
		if(ptr->nextSiblingPtr==NULL){
			USLOSS_Console("There is no such child exist.....\n");
			USLOSS_Console("halting....\n");
			USLOSS_Halt(1);
		}
		procPtr temp=ptr->nextSiblingPtr;
		ptr->nextSiblingPtr=ptr->nextSiblingPtr->nextSiblingPtr;
		temp->nextSiblingPtr=NULL;
	}
	return header;
}
		
//The process highest which has the highest priority has finished->
procPtr removeHead(procPtr header){
	procPtr temp=header->nextProcPtr;
	header->nextProcPtr=NULL;
	header=temp;
	return header;
}
void printList(procPtr header){
	while(header!=NULL){
		USLOSS_Console("This is in slot : %d, status= %d priority= %d\n",(int)header->pid,header->status,header->priority);
		header=header->nextProcPtr;
	}
}
void printList2(procPtr header){
        while(header!=NULL){
                USLOSS_Console("This is in slot : %d, status= %d priority= %d\n",(int)header->pid,header->status,header->priority);
                header=header->nextZoombie;
        }
}
	
	
