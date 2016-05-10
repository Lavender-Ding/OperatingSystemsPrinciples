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

#define THREADS 't'
#define ITERATIONS 'i'
#define YIELD 'y'
#define SYNC 's'

#define NO_LOCK 'n'
#define MUTEX_LOCK 'm'
#define SPIN_LOCK 's'
#define COMPARE_SWAP_LOCK 'c'

long long counter=0;
int thread_flag=0,iteration_flag=0,opt_yield=0;
int num_threads=1,num_iterations=1;
char lock_opt=NO_LOCK;
pthread_mutex_t mutexLock;
int spin_lock=0;

/*void add(long long *pointer,long long value){
        long long sum=*pointer+value;
        *pointer=sum;
}*/

void add(long long *pointer,long long value) {
        long long sum=*pointer+value;
        if(opt_yield)
            pthread_yield();
        *pointer=sum;
}

void *addIteration(){	
	int i;
	for(i=0;i<num_iterations;i++){
		if(lock_opt==NO_LOCK){
			add(&counter,1);
		}
		else if(lock_opt==MUTEX_LOCK){
			pthread_mutex_lock(&mutexLock);
			add(&counter,1);
			pthread_mutex_unlock(&mutexLock);
		}
		else if(lock_opt==SPIN_LOCK){
			while(__sync_lock_test_and_set(&spin_lock,1));
			add(&counter,1);
			__sync_lock_release(&spin_lock);
		}
		else if(lock_opt==COMPARE_SWAP_LOCK){
			long long previous,sum;
			do{
				previous=counter;
				sum=previous+1;
				if(opt_yield) pthread_yield();
			}while(__sync_val_compare_and_swap(&counter,previous,sum)!=previous);
		}
		else{
			exit(1);
		}
	}
	for(i=0;i<num_iterations;i++){
		if(lock_opt==NO_LOCK){
			add(&counter,-1);
		}
		else if(lock_opt==MUTEX_LOCK){
			pthread_mutex_lock(&mutexLock);
			add(&counter,-1);
			pthread_mutex_unlock(&mutexLock);
		}
		else if(lock_opt==SPIN_LOCK){
			while(__sync_lock_test_and_set(&spin_lock,1));
			add(&counter,-1);
			__sync_lock_release(&spin_lock);
		}
		else if(lock_opt==COMPARE_SWAP_LOCK){
			long long previous,sum;
			do{
				previous=counter;
				sum=previous-1;
				if(opt_yield) pthread_yield();
			}while(__sync_val_compare_and_swap(&counter,previous,sum)!=previous);
		}
		else{
			exit(1);
		}
	}
	return (void*)NULL;
}

int main(int argc, char *argv[]){
	int t;
	int opt;
	char *optstring="";
	void *status;
	struct option long_options[]={
		{"threads",required_argument,NULL,THREADS},
		{"iterations",required_argument,NULL,ITERATIONS},
		{"yield",no_argument,NULL,YIELD},
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
			opt_yield=1;
		}
		else if(opt==SYNC){
			lock_opt=*optarg;
			if(lock_opt!=MUTEX_LOCK&&lock_opt!=SPIN_LOCK&&lock_opt!=COMPARE_SWAP_LOCK){
				printf("Invalid lock type!\n");
				exit(1);
			}
		}
		else{
			printf("Invalid option!");
			exit(1);
		}
	}
	pthread_mutex_init(&mutexLock,NULL); 
	counter=0;
	struct timespec start_time;
	clock_gettime(CLOCK_MONOTONIC,&start_time);
	pthread_t threads[num_threads];
	for(t=0;t<num_threads;t++){
		if(pthread_create(&threads[t],NULL,addIteration,NULL)!=0){
			fprintf(stderr,"error creating thread!\n");
			exit(1);
		}
	}
	for(t=0;t<num_threads;t++){
		if(pthread_join(threads[t],&status)!=0){
			fprintf(stderr,"error joining thread!\n");
			exit(1);
		}
	}
	struct timespec end_time;
	clock_gettime(CLOCK_MONOTONIC,&end_time);
	long long elapsed_time=(end_time.tv_sec-start_time.tv_sec)*1000000000;
	elapsed_time+=end_time.tv_nsec;
	elapsed_time-=start_time.tv_nsec;
	int num_operations=num_threads*num_iterations*2;
	long long time_per=elapsed_time/num_operations;
	printf("%d threads x %d iterations x (add + subtract) = %d operations\n",num_threads,num_iterations,num_operations);
	if(counter!=0){
		fprintf(stderr,"ERROR: final count = %lld\n",counter);
	}
	printf("elapsed time: %lldns\n",elapsed_time);
	printf("per operation: %lldns\n",time_per);
	if(counter!=0) exit(1);
	else exit(0);
}








