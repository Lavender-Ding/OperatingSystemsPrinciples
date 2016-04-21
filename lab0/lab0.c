#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define INPUT 'i'
#define OUTPUT 'o'
#define SEGFAULT 's'
#define CATCH 'c'

void segErrorHandler(){
	fprintf(stderr,"Segmentation Fault Caught!\n");
	exit(3);
}

int main(int argc, char *argv[]){
	//long options
	int opt,ifd,ofd,n;
	char *in,*out;
	char *seg=NULL;
	char *optstring="";
	int flag[4]={0,0,0,0};
	char buf[10];
	struct option long_options[]={
		{"input",required_argument,NULL,INPUT},
		{"output",required_argument,NULL,OUTPUT},
		{"segfault",no_argument,NULL,SEGFAULT},
		{"catch",no_argument,NULL,CATCH},
		{0,0,0,0}
	};
	while((opt=getopt_long(argc,argv,optstring,long_options,NULL))!=-1){
		if(opt==INPUT){
			flag[0]=1;
			in=optarg;
			if(in==NULL) exit(1);
		}
		else if(opt==OUTPUT){
			flag[1]=1;
			out=optarg;
		}
		else if(opt==SEGFAULT){
			flag[2]=1;
		}
		else if(opt==CATCH){
			flag[3]=1;
		}
		else if(opt=='?'){
			exit(1);
		}
		else {
			fprintf(stderr,"Error command line!");
			exit(1);
		}
	}
	if(flag[3]==1){
		(void) signal(SIGSEGV,segErrorHandler);
	}
	if(flag[2]==1){
		seg[0]=1;
	}
	if(flag[0]==1){
		ifd=open(in,O_RDONLY);
		if(ifd<0){
			perror(in);
			exit(1);
		}
		close(0);
		dup2(ifd,0);
		close(ifd);
	}
	if(flag[1]==1){
		ofd=open(out,O_WRONLY|O_CREAT|O_TRUNC,00777);
		if(ofd<0){
			perror(out);
			exit(2);
		}
		close(1);
		dup2(ofd,1);
		close(ofd);
	}
	while(1){
		n=read(0,buf,10);
		if(n==0) break;
		if(n<0){
			perror(in);
			exit(1);
		}
		write(1,buf,n);
	}
	exit(0);
}

