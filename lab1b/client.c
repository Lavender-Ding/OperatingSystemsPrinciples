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
#include <netdb.h> 
#include <string.h>

#define PORT 'p'
#define LOG 'l'
#define ENCRYPT 'e'

struct termios originalMode;
struct termios setMode;

int p_to_c[2];	
int c_to_p[2];
int n=128;
pthread_t t_p;
pid_t childpid;
int status;
int sockfd, portno, n;
struct sockaddr_in serv_addr;
struct hostent *server;
char *sent="SENT 1 byte: ";
char *rece="RECEIVED 1 byte: ";
char newline[2]={0x0D,0x0A};
int log_fd,log_opt=0,en_opt=0;
char *logfile="";
MCRYPT td;
char password[256];


void resetInputMode(int fd){
	tcsetattr(fd,TCSANOW,&originalMode);
}

void exitFunc(){
	/*waitpid(childpid,&status,0);
	if(WIFEXITED(status)){
		printf("Shell's exit status is: %d\n",WEXITSTATUS(status));
	}
	else{
		printf("Shell exit abnormally!\n");
	}*/
	mcrypt_generic_end(td);
	resetInputMode(0);
}

void setInputMode(int fd){
	tcgetattr(fd,&originalMode);
	setMode=originalMode;
	setMode.c_lflag &=~ECHO;
	setMode.c_lflag &=~ICANON;
	setMode.c_cc[VMIN]=1;
	setMode.c_cc[VTIME]=0;
	tcflush(0,TCIFLUSH);
	tcsetattr(fd,TCSANOW,&setMode);
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

void *func(){
	char buff_from_socket[1];
	while (1){
		int count=read(sockfd,buff_from_socket,1);
		char buffe=buff_from_socket[0];
		if(en_opt==1) mdecrypt_generic (td,&buffe,1);
		if(count==0||buffe==0x04){
			close(sockfd);
			resetInputMode(0);
			exit(1);
		}
		if(log_opt==1){
			write(log_fd,rece,strlen(rece));
			write(log_fd,buff_from_socket,1);
			write(log_fd,"\n",1);
		}
		if(buffe==0x0A||buffe==0x0D){
			write(1,newline,2);
		}
		else{
			write(1,&buffe,count);
		}
	}
	return NULL;
}

int main(int argc, char *argv[]){
	int flag=0;
	int opt;
	char *optstring="",*portnoStr="";
	log_opt=0;
	en_opt=0;
	struct option long_options[]={
		{"port",required_argument,NULL,PORT},
		{"log",required_argument,NULL,LOG},
		{"encrypt",no_argument,NULL,ENCRYPT},
		{0,0,0,0}
	};
	while((opt=getopt_long(argc,argv,optstring,long_options,NULL))!=-1){
		if(opt==PORT){
			flag=1;
			portnoStr=optarg;
		}
		else if(opt==LOG){
			log_opt=1;
			logfile=optarg;
		}
		else if (opt==ENCRYPT){
			en_opt=1;
		}
		else{
			exit(1);
		}
	}
	if(log_opt==1){
		log_fd=open(logfile,O_WRONLY|O_CREAT|O_TRUNC,00777);
	}
	if(en_opt==1){
		en_func();
	}
	if(flag!=1){
		fprintf(stderr,"ERROR, no port provided\n");
		exit(1);
	}
	
	portno=atoi(portnoStr);
	if((sockfd=socket(AF_INET,SOCK_STREAM,0))<0){
		error("ERROR opening socket");
	}
	server=gethostbyname("localhost");
	if(server==NULL){
		fprintf(stderr,"ERROR, no such host\n");
		exit(1);
	}
	memset((char*) &serv_addr,0,sizeof(serv_addr));
	serv_addr.sin_family=AF_INET;
	bcopy((char*)server->h_addr,(char *)&serv_addr.sin_addr.s_addr,server->h_length);
	serv_addr.sin_port=htons(portno);
	if(connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr))<0){
		error("ERROR connecting");
	}
	
	atexit(exitFunc);
	setInputMode(0);
	//Read from socket
	if(pthread_create(&t_p,NULL,func,NULL)!=0){
		fprintf(stderr,"error creating thread!\n");
		exit(1);
	}

	//Non-canonical input from keyboard
	char buff[1];
	char newline[2]={0x0D,0x0A};
	int count;
	char bufe;
	while(1){
		int res=read(0,buff,n);
		char buffe=buff[0];
		if(buffe==0x04){
			
			pthread_cancel(t_p);
			close(sockfd);
			resetInputMode(0);
			exit(0);
		}
		if(buffe==0x0A||buffe==0x0D){
			buffe='\n';
		}
		if(buffe==0x0A||buffe==0x0D) write(1,newline,2);
		else write(1,&buffe,res);
		if(en_opt==1) mcrypt_generic(td,&buffe,1);
		write(sockfd,&buffe,res);
		if(log_opt==1){
			write(log_fd,sent,strlen(sent));
			write(log_fd,&buffe,1);
			write(log_fd,newline,2);
		}
	}
	
	return 0;
}




