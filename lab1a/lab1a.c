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

//#define BAUDRATE B38400
#define SHELL 's'

struct termios originalMode;
struct termios setMode;

int p_to_c[2];	
int c_to_p[2];
int n=128;
pthread_t t_p;
pid_t childpid;
int status;

void resetInputMode(int fd){
	tcsetattr(fd,TCSANOW,&originalMode);
}

void exitFunc(){
	waitpid(childpid,&status,0);
	if(WIFEXITED(status)){
		printf("Shell's exit status is: %d\n",WEXITSTATUS(status));
	}
	else{
		printf("Shell exit abnormally!\n");
	}
	resetInputMode(0);
}

void setInputMode(int fd){
	/*if(!isatty(0)){
		fprintf(stderr,"Not a terminal.\n");
		exit(1);
	}*/
	tcgetattr(fd,&originalMode);
	setMode=originalMode;
        //setMode.c_cflag = BAUDRATE | CRTSCTS | CS8 | CLOCAL | CREAD;
        //setMode.c_iflag = IGNPAR;
        //setMode.c_oflag = 0;
	//setMode.c_lflag = 0;
	setMode.c_lflag &=~ECHO;
	setMode.c_lflag &=~ICANON;
	setMode.c_cc[VMIN]=1;
	setMode.c_cc[VTIME]=0;
	tcflush(0,TCIFLUSH);
	tcsetattr(fd,TCSANOW,&setMode);
}

void handler(){
	printf("SIGPIPE caught!\n");
	resetInputMode(0);
	exit(1);
}

void siginthandler()
{
	kill(childpid,SIGINT);
}

void *func(){
	char buff_from_shell[1];
	while (1){
		char newline[2]={0x0D,0x0A};
		int count=read(c_to_p[0],buff_from_shell,1);
		if(count==0||buff_from_shell[0]==0x04){
			resetInputMode(0);
			//resetInputMode(c_to_p[0]);
			exit(1);
		}
		if(buff_from_shell[0]==0x0A||buff_from_shell[0]==0x0D)
			write(1,newline,2);
		else write(1,buff_from_shell,count);
	}
	return NULL;
}

int main(int argc, char *argv[]){
	int flag=0;
	int opt;
	char *optstring="";
	struct option long_options[]={
		{"shell",no_argument,NULL,SHELL},
		{0,0,0,0}
	};
	while((opt=getopt_long(argc,argv,optstring,long_options,NULL))!=-1){
		if(opt==SHELL){
			flag=1;
		}
	}
	if(flag==1){
		atexit(exitFunc);
		//Create two pipes
		signal(SIGINT,siginthandler);
		signal(SIGPIPE,handler);
		if((pipe(p_to_c))==-1){
			fprintf(stderr,"pipe failed!\n");
			exit(2);
		}
		if((pipe(c_to_p))==-1){
			fprintf(stderr,"pipe failed!\n");
			exit(2);
		}
		if((childpid=fork())==-1){
			perror("fork");
			exit(2);
		}
	
		//Parent process
		if(childpid>0){
			close(p_to_c[0]);
			close(c_to_p[1]);
			setInputMode(0);
			//setInputMode(c_to_p[0]);
			//Read from shell
		
			if(pthread_create(&t_p,NULL,func,NULL)!=0){
				fprintf(stderr,"error creating thread!\n");
				exit(2);
			}
		
			//Non-canonical input from keyboard
			char buff[n];
			char newline[2]={0x0D,0x0A};
		
			while(1){
				int res=read(0,buff,n);
				if(res==0||buff[0]==0x04){
					pthread_cancel(t_p);
					kill(childpid,SIGHUP);
					close(p_to_c[1]);
					close(c_to_p[0]);
					resetInputMode(0);
					exit(0);
				}
				if(buff[0]==0x03){
					kill(childpid,SIGINT);
				}
				if(buff[0]==0x0D||buff[0]==0x0A){
					write(1,newline,2);
					write(p_to_c[1],"\n",2);
				}
				else{
					write(1,buff,res);
					write(p_to_c[1],buff,res);
				}
			}
		}
	
		//Child process
		else if(childpid==0){
			close(c_to_p[0]);
			//close(0);
			dup2(p_to_c[0],0);
			close(p_to_c[0]);
			//close(1);
			dup2(c_to_p[1],1);
			close(c_to_p[1]);
			char *execvp_argv[2];
			execvp_argv[0]="/bin/bash";
			execvp_argv[1]=NULL;
			int err=execvp("/bin/bash",execvp_argv);
			if(err==-1){
				fprintf(stderr,"execvp failed!\n");
				exit(2);
			}
		}
	
		else{
			fprintf(stderr,"fork failed!\n");
			exit(2);
		}
	}
	else{
		setInputMode(0);
		char buff[n];
		char newline[2]={0x0D,0x0A};
	
		while(1){
			int res=read(0,buff,n);
			if(res==0||buff[0]==0x04){
				resetInputMode(0);
				exit(0);
			}
			if(buff[0]==0x0D||buff[0]==0x0A){
				write(1,newline,2);
			}
			else{
				write(1,buff,res);
			}
		}
		resetInputMode(0);
	}
	
	return 0;
}




