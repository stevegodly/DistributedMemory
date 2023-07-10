#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <memory.h>
#include <errno.h>
#include <arpa/inet.h>
#include<pthread.h>
#include <string.h>

char *pt="127.0.0.1";
char *data;
int PORT;

typedef struct{
    int key;
    int val;
}hash_table;

hash_table table;

struct client_info{
    int fd;
    int port;
};

typedef struct{
    int id;
    int k;
    int tport;
}app_messg;

void createIPv4address(struct sockaddr_in *servaddr,int port){
    servaddr->sin_family = AF_INET;
	servaddr->sin_port = htons(port);
	inet_pton(AF_INET,pt,&(servaddr->sin_addr.s_addr));
}

void displayHashTable(void){
    printf("KEY : %d\n",table.key);
    printf("VALUE : %d\n",table.val);
}

void putTcp(app_messg *mssg){
    int sockfd;
    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
	createIPv4address(&servaddr,mssg->tport);
    if(connect(sockfd,(struct sockaddr*)&servaddr, sizeof(servaddr))<0){
        perror("Couldn't connect with the server");
        exit(EXIT_FAILURE);
    }
    char *buffer=malloc(sizeof(app_messg));
    memcpy(buffer,mssg,sizeof(app_messg));
    ssize_t bytes_sent = send(sockfd, buffer, sizeof(app_messg), 0);
    if (bytes_sent < 0) {
        perror("sendto failed");
        exit(EXIT_FAILURE);
    }
    int x;
    if(recv(sockfd,&x,sizeof(int),0)<0){
        perror("recv failed");
        exit(EXIT_FAILURE);
    }
    table.val=x;
    close(sockfd);
}

void putUdp(int port, app_messg *mssg){
    printf("Putudp");
    int sockfd;
	char *buffer=malloc(sizeof(app_messg));
	struct sockaddr_in servaddr;
	
	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}
	memset(&servaddr, 0, sizeof(servaddr));
	createIPv4address(&servaddr,port);
	
	memcpy(buffer,mssg,sizeof(mssg));
		
	if(sendto(sockfd, (const char *)buffer, strlen(buffer),MSG_CONFIRM, (const struct sockaddr *) &servaddr,sizeof(servaddr))<0){
        printf("port:%d",port);
        perror("sendto failed");
        exit(EXIT_FAILURE);
    }
}



void udp_client(void *arg){
    printf("udp_client");
    struct client_info *client=malloc(sizeof(struct client_info));
    client=(struct client_info*)arg;
    int fd=client->fd;
    app_messg *mssg=malloc(sizeof(app_messg));
    ssize_t nbytes=recvfrom(fd,mssg,sizeof(app_messg),0,NULL,NULL);
    printf("\nPORT:%d==mssg.k%d\n",PORT,mssg->k);
    if(nbytes<0){
        perror("recv failed");
        printf("error in udp_client %d",client->port);
        exit(EXIT_FAILURE);
    }
    printf("message recieved");
    if(((PORT%2000)/2)==(mssg->k)%6){
        printf("\n%d==%d:condition satisfied\n",PORT,mssg->k);
        close(fd);
        putTcp(mssg);
        return ; 
    }
    close(fd);
    putUdp(client->port,mssg);
}

void service_client_module(void *arg){
    struct client_info *client=malloc(sizeof(struct client_info));
    client=(struct client_info*)arg;
    int fd=client->fd;
    app_messg *mssg=malloc(sizeof(app_messg));
    char *buffer=malloc(sizeof(int));
    buffer=data;
    ssize_t nbytes=recv(fd,mssg,sizeof(app_messg),0);
    if(nbytes<0){
        perror("recv failed");
        exit(EXIT_FAILURE);
    }
    if(mssg->id==0) {
        if(send(fd,buffer,sizeof(int), 0)<0){
            perror("sendto failed");
            exit(EXIT_FAILURE);
        }
    }    
    close(fd);
}

void setupConnection(int uPort,int u1Port,int tPort){
    system("clear");
	printf ("       INSTRUCTIONS \n\n  =================NODE %d=======================\n",tPort);
	puts("   1.'put' request format : PUT(<integer>,<integer>)\n");
	puts("   2.'get' request format : GET(<integer>)\n");
	puts("   3.To print Hash Table : 'r'\n");
	puts("-----------------------------------\n\nENTER GET/PUT REQUEST :");
    PORT=tPort;
    int server_fd,client_fd,server_ufd;
    struct sockaddr_in servaddr, servuaddr,cliaddr;
    int addrlen=sizeof(cliaddr);
    table.key=(tPort%2000)/2;
    fd_set readfds;
    if((server_fd=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP))==-1){
        perror("Socket creation failed");
        exit(1);
    }

    createIPv4address(&servaddr,tPort);
    createIPv4address(&servuaddr,uPort);

    if (bind(server_fd, (struct sockaddr *)&servaddr, sizeof(struct sockaddr)) == -1)
    {
        printf("socket bind failed\n");
        return;
    } 
    
    if (listen(server_fd, 6)<0)  
    {
        printf("listen failed\n");
        return;
    }
    if((server_ufd=socket(AF_INET,SOCK_DGRAM,0))==-1){
        perror("Socket creation failed");
        exit(1);
    }

    if (bind(server_ufd, (struct sockaddr *)&servuaddr, sizeof(struct sockaddr)) == -1)
    {
        printf("socket bind failed\n");
        return;
    } 
    struct client_info *client_info_t=calloc(1,sizeof(struct client_info));

    while(1){
        FD_ZERO(&readfds);
        FD_SET(server_fd,&readfds);
        FD_SET(server_ufd,&readfds);
        FD_SET(0,&readfds);

        int master_fd=(server_fd>server_ufd?server_fd:server_ufd);
        select(master_fd+1,&readfds,NULL,NULL,NULL);
        
        if(FD_ISSET(server_fd,&readfds)){
            printf("reached tcp:%d\n",tPort);
            client_fd=accept(server_fd,(struct sockaddr*)&cliaddr,&addrlen);
            if(client_fd < 0){
                printf("accept error : errno = %d\n", errno);
                exit(0);
            }
            client_info_t->fd=client_fd;
            client_info_t->port=tPort;
            service_client_module((void *)client_info_t);
        }
        if(FD_ISSET(server_ufd,&readfds)){
            
            printf("reached udp:%d\n",tPort);
            
            client_info_t->fd=server_ufd;
            client_info_t->port=u1Port;
            udp_client((void *)client_info_t);
            printf("end");
        }
        if(FD_ISSET(0,&readfds)){
            printf("reached stdin\n");
            app_messg *app=malloc(sizeof(app_messg));
            char recv_buffer[1024];
            fgets(recv_buffer, sizeof(recv_buffer), stdin);
            if (strncmp(recv_buffer, "r",1) == 0 || strncmp(recv_buffer, "R",1) == 0) {
                displayHashTable();
                fflush(stdout);
                continue;
            }

            int y;
            char *token;
            char *dup=strdup(recv_buffer);
            char del[]="(,";
            token=strtok(dup,del);
            if(token==NULL|| (strncmp(recv_buffer,"PUT",3)!=0 && strncmp(recv_buffer, "GET",3)!=0)) {  printf("INVALID ENTRY !!");fflush(stdout);continue;}
            if(strncmp(token,"PUT",3)==0) app->id=0;
            else app->id=1;
            token=strtok(NULL,del);
            app->k=atoi(token);
            token=strtok(NULL,del);
            strcpy(data,token);
            app->tport=tPort;
            fflush(stdout);
            printf("key:%d,tPort:%d",app->k,app->tport);
            putUdp(u1Port,app);
            
        }
    }
}

int main(int argc,char **argv)
{
    setupConnection(atoi(argv[1]),atoi(argv[2]),atoi(argv[3]));
}