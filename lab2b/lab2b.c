#define _GNU_SOURCE

#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <assert.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/wait.h>
#include <time.h>
#include <string.h>
#include "SortedList.h"

#define THREADS 't'
#define ITERATIONS 'i'
#define YIELD 'y'
#define SYNC 's'

#define NO_LOCK 'n'
#define MUTEX_LOCK 'm'
#define SPIN_LOCK 's'
//#define COMPARE_SWAP_LOCK 'c'

char *char_yield;
int opt_yield=0x00;
int is_yield=0;
int thread_flag=0,iteration_flag=0;
int num_threads=1,num_iterations=1;
char lock_opt=NO_LOCK;
pthread_mutex_t mutexLock;
int spin_lock=0;
SortedList_t *listHead=NULL;
//char ***randomKey;
SortedListElement_t **elements;

struct thread_info{
	pthread_t thread_id;
	int thread_num;
};

void listInitialization(){
	listHead=malloc(sizeof(SortedList_t));
	listHead->next=NULL;
}

void randomInitialization(){
	srand(time(NULL));
	int i,j,k,r;
	
	elements=(SortedListElement_t**)malloc(num_threads*sizeof(SortedListElement_t*));
	for(i=0;i<num_threads;i++){
		elements[i]=(SortedListElement_t*)malloc(num_iterations*sizeof(SortedListElement_t));
		for(j=0;j<num_iterations;j++){
			char *randomKey=(char*)malloc(4*sizeof(char));
			for(k=0;k<3;k++){
				r=rand()%26;
				//r+='A';
				if(r%2==1) r+='a';
				else r+='A';	
				randomKey[k]=(char)r;
			}
			randomKey[3]='\0';
			elements[i][j].key=randomKey;
			elements[i][j].prev=NULL;
			elements[i][j].next=NULL;
		}
	}
}
/*
void printList(){
	SortedListElement_t *listElement=listHead;
	while(listElement->next!=NULL){
		write(1,listElement->next->key,4);
		write(1," ",1);
		listElement=listElement->next;
	}
	write(1,"\n",1);
}
*/
void *funcIteration(void *ptid){
	struct thread_info *thread=ptid;	
	int tid=thread->thread_num;
	int i;
	//insert
	for(i=0;i<num_iterations;i++){
		//no lock
		if(lock_opt==NO_LOCK){
			SortedList_insert(listHead,&elements[tid][i]);
		}
		//mutex lock
		else if(lock_opt==MUTEX_LOCK){
			pthread_mutex_lock(&mutexLock);
			SortedList_insert(listHead,&elements[tid][i]);
			pthread_mutex_unlock(&mutexLock);
		}
		//spin lock
		else if(lock_opt==SPIN_LOCK){
			while(__sync_lock_test_and_set(&spin_lock,1));
			SortedList_insert(listHead,&elements[tid][i]);
			//printList();
			__sync_lock_release(&spin_lock);
		}
		else exit(1);
	}
	//get length
	//no lock
	if(lock_opt==NO_LOCK){
		SortedList_length(listHead);
		//if(len<0) return (void*)NULL;
	}
	//mutex lock
	else if(lock_opt==MUTEX_LOCK){
		pthread_mutex_lock(&mutexLock);
		SortedList_length(listHead);
		//if(len<0) return (void*)NULL;
		pthread_mutex_unlock(&mutexLock);
	}
	//spin lock
	else if(lock_opt==SPIN_LOCK){
		while(__sync_lock_test_and_set(&spin_lock,1));
		SortedList_length(listHead);
		__sync_lock_release(&spin_lock);
	}
	else exit(1);
	
	//look up and delete
	for(i=0;i<num_iterations;i++){
		//no lock
		if(lock_opt==NO_LOCK){
			SortedListElement_t *tmp=SortedList_lookup(listHead,elements[tid][i].key);
			SortedList_delete(tmp);
		}
		//mutex lock
		else if(lock_opt==MUTEX_LOCK){
			pthread_mutex_lock(&mutexLock);
			SortedListElement_t *tmp=SortedList_lookup(listHead,elements[tid][i].key);
			SortedList_delete(tmp);
			pthread_mutex_unlock(&mutexLock);
		}
		//spin lock
		else if(lock_opt==SPIN_LOCK){
			while(__sync_lock_test_and_set(&spin_lock,1));
			SortedListElement_t *tmp=SortedList_lookup(listHead,elements[tid][i].key);
			SortedList_delete(tmp);
			__sync_lock_release(&spin_lock);
		}
		else exit(1);
	}
	return (void*)NULL;
}

int main(int argc, char *argv[]){
	int t,i;
	int opt;
	char *optstring="";
	void *status;
	struct option long_options[]={
		{"threads",required_argument,NULL,THREADS},
		{"iterations",required_argument,NULL,ITERATIONS},
		{"yield",required_argument,NULL,YIELD},
		{"sync",required_argument,NULL,SYNC},
		{0,0,0,0}
	};
	while((opt=getopt_long(argc,argv,optstring,long_options,NULL))!=-1){
		if(opt==THREADS){
			thread_flag=1;
			num_threads=atoi(optarg);
		}
		else if(opt==ITERATIONS){
			iteration_flag=1;
			num_iterations=atoi(optarg);
		}
		else if(opt==YIELD){
			is_yield=1;
			char_yield=optarg;
		}
		else if(opt==SYNC){
			lock_opt=*optarg;
			if(lock_opt!=MUTEX_LOCK&&lock_opt!=SPIN_LOCK){
				printf("Invalid lock type!\n");
				exit(1);
			}
		}
		else{
			printf("Invalid option!");
			exit(1);
		}
	}
	if(is_yield==1){
		for(i=0;i<strlen(char_yield);i++){
			if(char_yield[i]=='i') opt_yield=opt_yield|INSERT_YIELD;
			if(char_yield[i]=='d') opt_yield=opt_yield|DELETE_YIELD;
			if(char_yield[i]=='s') opt_yield=opt_yield|SEARCH_YIELD;
		}
	}
	listInitialization();
	randomInitialization();

	pthread_mutex_init(&mutexLock,NULL);
	struct thread_info *threads;
	threads=malloc(num_threads*sizeof(struct thread_info));
	struct timespec start_time;
	clock_gettime(CLOCK_MONOTONIC,&start_time);
	for(t=0;t<num_threads;t++){
		threads[t].thread_num=t;
		if(pthread_create(&threads[t].thread_id,NULL,funcIteration,&threads[t])!=0){
			fprintf(stderr,"error creating thread!\n");
			exit(1);
		}
	}
	for(t=0;t<num_threads;t++){
		if(pthread_join(threads[t].thread_id,&status)!=0){
			fprintf(stderr,"error joining thread!\n");
			exit(1);
		}
	}
	struct timespec end_time;
	clock_gettime(CLOCK_MONOTONIC,&end_time);
	int lenList=SortedList_length(listHead);
	for(i=0;i<num_threads;i++) free(elements[i]);
	long long elapsed_time=(end_time.tv_sec-start_time.tv_sec)*1000000000;
	elapsed_time+=end_time.tv_nsec;
	elapsed_time-=start_time.tv_nsec;
	int num_operations=num_threads*num_iterations*2;
	long long time_per=elapsed_time/num_operations;
	printf("%d threads x %d iterations x (insert + lookup/delete) = %d operations\n",num_threads,num_iterations,num_operations);
	if(lenList!=0){
		fprintf(stderr,"ERROR: final length = %d\n",lenList);
	}
	printf("elapsed time: %lldns\n",elapsed_time);
	printf("per operation: %lldns\n",time_per);
	if(lenList!=0) exit(1);
	else exit(0);

}








