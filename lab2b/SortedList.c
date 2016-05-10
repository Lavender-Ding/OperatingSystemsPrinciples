#define _GNU_SOURCE

#include "SortedList.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

extern int opt_yield;

void SortedList_insert(SortedList_t *list, SortedListElement_t *element){
	SortedListElement_t *previous=list;
	while(previous->next!=NULL){
		if(strcmp(previous->next->key,element->key)<=0)
			previous=previous->next;
		else break;
	}
	if(opt_yield&INSERT_YIELD) pthread_yield();//yield
	if(previous->next!=NULL){
		SortedListElement_t *successor=previous->next;
		previous->next=element;
		element->next=successor;
		element->prev=previous;
		successor->prev=element;
	}
	else{
		previous->next=element;
		element->prev=previous;
		element->next=NULL;
	}
	
	return;
}

int SortedList_delete(SortedListElement_t *element){
	//if(element==NULL) return 1;
	SortedListElement_t *previous=element->prev;
	SortedListElement_t *successor=element->next;
	if(successor==NULL){
		if(previous->next==element){
			previous->next=successor;
			return 0;
		}
		else return 1;
	}
	if(opt_yield&DELETE_YIELD) pthread_yield();//yield
	if(successor->prev==element&&previous->next==element){
		previous->next=successor;
		successor->prev=previous;
		return 0;
	}
	return 1;
}

SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key){
	SortedListElement_t *element=NULL;
	SortedListElement_t *current=list->next;
	if(opt_yield&SEARCH_YIELD) pthread_yield();//yield
	while(current!=NULL){
		//if(current->key==key){
		if(strcmp(current->key,key)==0){
			element=current;
			break;
		}
		current=current->next;
	}
	return element;
}

int SortedList_length(SortedList_t *list){
	int len=0;
	SortedListElement_t *current=list->next;
	SortedListElement_t *previous=list;
	if(opt_yield&SEARCH_YIELD) pthread_yield();//yield
	while(current!=NULL){
		if(current->prev==previous){
			len++;
			previous=current;
			current=current->next;
		}
		else return -1;
	}
	return len;
}


