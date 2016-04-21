#include <unistd.h>
#include <stdio.h>
#include <mcrypt.h>
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
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

#define PORT 'p'
#define ENCRYPT 'e'

struct termios originalMode;
struct termios setMode;

int p_to_c[2];	
int c_to_p[2];
int n=1;
int sockfd, newsockfd,portno,clilen;
pthread_t t_p;
pid_t childpid;
int status;
int en_opt=0;
MCRYPT td;
char password[256];

void exitFunc(){
	/*waitpid(childpid,&status,0);
	if(WIFEXITED(status)){
		printf("Shell's exit status is: %d\n",WEXITSTATUS(status));
	}
	else{
		printf("Shell exit abnormally!\n");
	}*/
	mcrypt_generic_end(td);
}

void handler(){
	printf("SIGPIPE caught!\n");
	exit(2);
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
			kill(childpid,SIGINT);
			close(sockfd);
			exit(2);
		}
		char buffe=buff_from_shell[0];
		if(en_opt==1) mcrypt_generic(td,&buffe,1);
		write(1,&buffe,count);
	}
	return NULL;
}
void en_func(){
	int i,keycnt;
	char *key;
	char key_buff[256];
	char *IV;
	int keysize=16; /* 128 bits */
	key=calloc(1, keysize);
	int key_fd=open("my.key",O_RDONLY);
	keycnt=read(key_fd,key_buff,256);
	strncpy(password,key_buff,keycnt);
	memmove(key, password, keycnt);
	td = mcrypt_module_open("twofish", NULL, "cfb", NULL);
	if (td==MCRYPT_FAILED) {
		exit(1);
	}
	IV = malloc(mcrypt_enc_get_iv_size(td));
	for (i=0; i< mcrypt_enc_get_iv_size( td); i++) {
		IV[i]=rand();
	}
	i=mcrypt_generic_init( td, key, keysize, IV);
	if (i<0) {
		mcrypt_perror(i);
		exit(1);
	}
}

int main(int argc, char *argv[]){
	//int ofd=open("test.txt",O_WRONLY|O_CREAT|O_TRUNC,00777);
	int flag=0;
	en_opt=0;
	int opt;
	char *optstring="",*portnoStr="";
	struct option long_options[]={
		{"port",required_argument,NULL,PORT},
		{"encrypt",no_argument,NULL,ENCRYPT},
		{0,0,0,0}
	};
	while((opt=getopt_long(argc,argv,optstring,long_options,NULL))!=-1){
		if(opt==PORT){
			flag=1;
			portnoStr=optarg;
		}
		else if(opt==ENCRYPT){
			en_opt=1;
		}
		else{
			exit(1);
		}
	}
	if(flag!=1){
		fprintf(stderr,"ERROR, no port provided\n");
	 	exit(1);
	}
	if(en_opt==1){
		en_func();
	}
	//Socket
	struct sockaddr_in serv_addr,cli_addr;
	if ((sockfd=socket(AF_INET,SOCK_STREAM,0))<0){
		error("ERROR opening socket");
	}
	memset((char*) &serv_addr,0,sizeof(serv_addr));
	portno=atoi(portnoStr);
	serv_addr.sin_family=AF_INET;
	serv_addr.sin_addr.s_addr=INADDR_ANY;
	serv_addr.sin_port=htons(portno);
	if(bind(sockfd,(struct sockaddr*) &serv_addr,sizeof(serv_addr))<0){ 
		error("ERROR on binding");
	}
	listen(sockfd,5);
	clilen=sizeof(cli_addr);
	if((newsockfd=accept(sockfd,(struct sockaddr*) &cli_addr,&clilen))<0){
		error("ERROR on accept");
	}

	atexit(exitFunc);
	//Create two pipes
	signal(SIGINT,siginthandler);
	signal(SIGPIPE,handler);
	if((pipe(p_to_c))==-1){
		fprintf(stderr,"pipe failed!\n");
		exit(1);
	}
	if((pipe(c_to_p))==-1){
		fprintf(stderr,"pipe failed!\n");
		exit(1);
	}
	if((childpid=fork())==-1){
		perror("fork");
		exit(1);
	}

	//Parent process
	if(childpid>0){
		close(p_to_c[0]);
		close(c_to_p[1]);
		dup2(newsockfd,0);
		dup2(newsockfd,1);
		dup2(newsockfd,2);
		//Read from shell
	
		if(pthread_create(&t_p,NULL,func,NULL)!=0){
			fprintf(stderr,"error creating thread!\n");
			exit(1);
		}
	
		//Non-canonical input from client
		char buff[n];
		char newline[2]={0x0D,0x0A};
		
		while(1){
			int res=read(0,buff,1);//gaile n->1
			char buffe=buff[0];
			if(en_opt==1) mdecrypt_generic(td,&buffe,1);
			if(res==0||buffe==0x04){
				pthread_cancel(t_p);
				close(sockfd);
				kill(childpid,SIGHUP);
				close(p_to_c[1]);
				close(c_to_p[0]);
				exit(1);
			}
			/*if(buffe==0x03){
				kill(childpid,SIGINT);
			}*/
			//write(ofd,&buffe,1);
			write(p_to_c[1],&buffe,res);
			/*if(buffe==0x0D||buffe==0x0A){
				write(p_to_c[1],"\n",2);
			}
			else{
				write(p_to_c[1],&buffe,res);
			}*/
		}
	}

	//Child process
	else if(childpid==0){
		close(c_to_p[0]);
		dup2(p_to_c[0],0);
		close(p_to_c[0]);
		dup2(c_to_p[1],1);
		dup2(c_to_p[1],2);
		close(c_to_p[1]);
		char *execvp_argv[2];
		execvp_argv[0]="/bin/bash";
		execvp_argv[1]=NULL;
		int err=execvp("/bin/bash",execvp_argv);
		if(err==-1){
			fprintf(stderr,"execvp failed!\n");
			exit(1);
		}
	}

	else{
		fprintf(stderr,"fork failed!\n");
		exit(1);
	}
	
	return 0;
}




