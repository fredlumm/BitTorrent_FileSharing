/***********BitTorrent************/

/*****Created by
            
            Weichen Ning
                            *****/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <openssl/sha.h>
#include <string.h>
#include <stddef.h>
#include "csapp.h"
#include <pthread.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ctype.h>
#include <setjmp.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <semaphore.h>
#include <fcntl.h>
#include "sha1.h"


/*****set structure*****/
#define false 0
#define true 1
#define MAXUSER 10
#define MAXTRACKER 10
#define BUFFER_SIZE 1024
#define maxhashlen 20
#define perhashlen 4

#define chord_ip "127.0.0.1"
#define chord_port 8999
//#define local_ip "127.0.0.1"

typedef int bool;

typedef struct User{
    char * IP;
    int port;
}User;

typedef struct Tracker{
    char * filename;
    int user_num;
    User user_arr[MAXUSER];
    //int user_state[MAXUSER]=0;
}Tracker;

int clientfd;

char * myIP;
int myport;

char * NodeIP=chord_ip;
int NodePort=chord_port;

char filename[MAXLINE];
char *filenames[10];

User * user_list = NULL;
int total_num=0;
int rec_num=0;

int rep_flag=0;

/*****declaration functions*****/
int getFileSize(char* filename);
int getPieceSize(int size, int num);
void sendnode_message(char* IP, int port, char* cmd, char* name);
void* sendpeer_message(void* args);
void receive_message();
void query();
void get_tracker();
void request_pieces();
void combine_pieces();
void update_tracker();
void reset();
void send_newinfo();
void upload();
void download();
void* server(void* args);
unsigned hashFunction(char* oristr);
void debug_info();


// server function //
int getFileSize(char* filename){
    struct stat temp;
    stat(filename,&temp);
    return temp.st_size;
}


int getPieceSize(int size, int num){
    int flag = size % num;
    if(flag!=0){
        return (size+num-flag)/num;
    }
    else{
        return size/num;
    }
}

unsigned hashFunction(char* oristr) {
	SHA1Context sha;
	unsigned result = 0;
	SHA1Reset(&sha);
	SHA1Input(&sha, (const unsigned char*)oristr, strlen(oristr));
    
	if((SHA1Result(&sha)) == 0) {
		printf("fail to compute message digest!\n");
	}
	else {
		result = sha.Message_Digest[0] ^ sha.Message_Digest[1];
		for(int i = 2; i < 5; i++) {
			result = result ^ sha.Message_Digest[i];
		}
	}
	return result;
}

//**********************//




/*****support functions*****/
void sendnode_message(char* IP, int port, char* cmd, char* name){
    //printf("IP is %s\n",IP);
    //printf("port is %d\n",port);
    char sendbuf[MAXLINE];
    sprintf(sendbuf, "%s %s %s %d\r\n", cmd, name, myIP, myport);
    clientfd = Open_clientfd(IP, port);
    if(clientfd<0){
        perror("Open_clientfd Failed!\n");
        exit(1);
    }
    //printf("sendbuf is %s\n",sendbuf);
    Rio_writen(clientfd, sendbuf, strlen(sendbuf));
    memset(sendbuf, 0, strlen(sendbuf));
}

void* sendpeer_message(void* args){
    int i = (((int*)args)[0]);
    int client_socket_fd;
    //printf("%s %d\n",user_list[i].IP,user_list[i].port);
    client_socket_fd=Open_clientfd(user_list[i].IP,user_list[i].port);

    if (client_socket_fd == -1) {
        perror("the Ip and port is:");
        exit(1);
    }

    char sendbuf[BUFFER_SIZE];
    sprintf(sendbuf, "%s %s %d %d\r\n", "Get_piece", filename, total_num, i+1);
    //printf("sendbuf=%s\n",sendbuf);
    if(send(client_socket_fd, sendbuf, BUFFER_SIZE, 0) < 0)
    {
        perror("Send File Name Failed:");
        exit(1);
    }
    //set piece name
    int temp = 0xaa + i;
    char temp2[10];
    sprintf(temp2,"%x",temp);
    char piecename[100];
    sprintf(piecename,"%s%s","x",temp2);
    //printf("piecename=%s\n",piecename);
    
    FILE *fp = fopen(piecename, "w");
    if(NULL == fp)
    {
        printf("File:\t%s Can Not Open To Write\n", piecename);
        exit(1);
    }
    
    char buffer[BUFFER_SIZE];
    bzero(buffer, BUFFER_SIZE);
    int length = 0;
    while((length = recv(client_socket_fd, buffer, BUFFER_SIZE, 0)) > 0)
    {
        if(fwrite(buffer, sizeof(char), length, fp) < length)
        {
            printf("File:\t%s Write Failed\n", piecename);
            break;
        }
        bzero(buffer, BUFFER_SIZE);
    }
    rec_num++;
    close(client_socket_fd);
    fclose(fp);
}

void receive_message(){
    char buf[MAXLINE];
    char header[MAXLINE];
    char buf1[MAXLINE];
    char buf2[MAXLINE];
    char buf3[MAXLINE];
    rio_t client;
    
    Rio_readinitb(&client, clientfd);
    while(1) {
        //rio_readp(clientfd,buf,MAXLINE);
        Rio_readlineb(&client,buf,MAXLINE);
        if (strcmp(buf,"error\r\n")==0){
            printf("The file you want to download doesn't exist!\n");
            close(clientfd);
            exit(1);
        }
        if (strcmp(buf, "finish\r\n") == 0){
            break;
        }
        sscanf(buf, "%s %s %s %s", header, buf1, buf2, buf3);
        if (strcmp(header, "total") == 0){
            total_num = atoi(buf1);
            user_list = malloc(sizeof(user_list)*total_num);
        }
        if (strcmp(header,"user")==0){
            int i = atoi(buf1);
            user_list[i-1].IP = strdup(buf2);
            user_list[i-1].port = atoi(buf3);
        }
        //printf("buf is %s\n",buf);
        memset(buf, 0, strlen(buf));
    }
    close(clientfd);
}

void query(){
    char buf[MAXLINE];
    char in[MAXLINE];
    char cmd[MAXLINE];
	char ip[MAXLINE], ports[MAXLINE], keyy[MAXLINE];
    rio_t client;
    
    printf("Please intput the filename:\n");
    int dummy = scanf("%s",in);
    strcpy(filename,in);
    
    //decide re-download
    for (int i = 0; i < 10; i++) {
        if(filenames[i] != NULL && strcmp(filenames[i],in)==0){
            rep_flag=1;
            break;
        }
    }
    
    unsigned key = hashFunction(filename);
    sprintf(buf, "Query q q %u\r\n", key);
    clientfd = open_clientfd(NodeIP, NodePort);
    if (clientfd == -1) {
        printf("The IP and port you entered is not in the ring.\n");
        exit(1);
    }
    Rio_writen(clientfd, buf, strlen(buf));
    memset(buf, 0, sizeof(buf));
    
    Rio_readinitb(&client, clientfd);
    while(1) {
        Rio_readlineb(&client,buf,MAXLINE);
        if (strcmp(buf, "finish\r\n") == 0) {
            break;
        }
        sscanf(buf, "%s %s %s %s", cmd, ip, ports, keyy);
        if (strcmp(cmd, "ResQuery") == 0) {
            //NodeIP = strdup(ip);
            NodePort = atoi(ports);
        }
    }
    close(clientfd);
}

void get_tracker(){
    sendnode_message(NodeIP, NodePort, "Get_Tracker", filename);
    receive_message();
}

void request_pieces(){
    pthread_t tid[total_num];
    int* args[total_num];
    for(int i=0; i<total_num; i++){
        args[i]=malloc(sizeof(*args[i]));
        *args[i]=i;
        Pthread_create(&tid[i],NULL,sendpeer_message,args[i]);
        Pthread_detach(tid[i]);
    }
    
    while(1){
    	printf("Please waiting...\n");
    	//printf("total_num is %d\n",total_num);
        if(rec_num==total_num){
            break;
        }
        sleep(1);
    }
    //printf("rec_num=%d\n",rec_num);

    //sleep(2);
}

void combine_pieces(){
    char *temp;
    char s[10] = "xaa";
    for(int i=0;i<total_num;i++){
        char rec[MAXLINE];
        if(i!=0){
            sprintf(rec,"%s %s",temp,s);
        }
        else{
            sprintf(rec,"%s",s);
            //printf("rec=%s\n",rec);
        }
        s[2]=s[2]+1;
        temp=strdup(rec);
        //printf("temp=%s\n",temp);
    }
    char act[MAXLINE];
    sprintf(act,"%s %s%s%s","cat",temp,"> ",filename);
    //printf("act is %s\n",act);
    int dummy = system(act);
}

void update_tracker(){
    sendnode_message(NodeIP, NodePort, "Update_Tracker",filename);
    
    rio_t client;
    char buf[MAXLINE];
    Rio_readinitb(&client, clientfd);
    while(1){
        Rio_readlineb(&client,buf,MAXLINE);
        if (strcmp(buf,"Complete\r\n")==0){
            close(clientfd);
            break;
        }
        memset(buf, 0, strlen(buf));
    }

    for (int i = 0; i < 10; i++) {
    	if (filenames[i] == NULL) {
    		filenames[i] = strdup(filename);
    		break;
    	}
    }
}

void reset(){
    if(total_num!=0){
        char *temp;
        char s[10] = "xaa";
        for(int i=0;i<total_num;i++){
            char rec[MAXLINE];
            if(i!=0){
                sprintf(rec,"%s %s",temp,s);
            }
            else{
                sprintf(rec,"%s",s);
            }
            s[2]=s[2]+1;
            temp=strdup(rec);
        }
        char act[MAXLINE];
        sprintf(act,"%s %s","rm",temp);
        int dummy = system(act);
    }
    
    NodeIP=chord_ip;
    //NodeIP=local_ip;
    NodePort=8999;
    memset(filename, 0, strlen(filename));
    if (user_list != NULL) {
    	free(user_list);
    	user_list = NULL;
    }
    total_num=0;
    rec_num=0;
    rep_flag=0;
}

void send_newinfo(){
    // as the first seeder, communicate with the chord ring, and set the tracker info
    char buf[MAXLINE];
    char cmd[MAXLINE];
    char in[MAXLINE];
    printf("Please input the filename that you want to upload:\n");
    int dummy = scanf("%s", in);
    strcpy(filename,in);

    // send the filename and its own ip and port info
    strcpy(cmd,"NewTracker");
    clientfd = open_clientfd(NodeIP, NodePort);
    if (clientfd == -1) {
        printf("The IP and port you entered is not in the ring.\n");
        exit(1);
    }
    sprintf(buf,"%s %s %d %s\r\n",cmd,myIP,myport,filename);
    Rio_writen(clientfd, buf, strlen(buf));
    memset(buf, 0, strlen(buf));
    close(clientfd);

    for (int i = 0; i < 10; i++) {
    	if (filenames[i] == NULL) {
    		filenames[i] = strdup(filename);
    		break;
    	}
    }
}

/*****implement functions*****/
void upload(){
    
    //***1.send new message to node***/
    send_newinfo();
    
    //***2.garbage collection***/
    reset();
}

void download(){
    
    /***1.we need to where our file's tracker should be stored***/
    query();
    //printf("Step 1 finished\n");
    
    if(rep_flag==0){
        /***2.we need to query from chord to get tracker information***/
        get_tracker();
        //printf("Step 2 finished\n");
        //debug_info();
    
        /***3.contact to the peers to get different pieces***/
        request_pieces();
        //printf("Step 3 finished\n");
        
        /***4.combine different pieces to a whole***/
        combine_pieces();
        //printf("Step 4 finished\n");
        
        /***5.update tracker information***/
        update_tracker();
        //printf("Step 5 finished\n");
    }
    else{
        printf("You have already downloaded this file. You don't need to download it again!\n");
    }
    
    /***6.garbage collection***/
    reset();
    //printf("Step 6 finished\n");
}

void* server(void* args){
    int clientlen,connfd;
    char cmd[BUFFER_SIZE];
    char filename[BUFFER_SIZE];
    int piece_num;
    int piece_pos;
    int listenfd = Open_listenfd(myport);
    int optval = 1;
    rio_t client;
    struct sockaddr_in clientaddr;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int));
    char buf1[BUFFER_SIZE];
    
    while(1) {
        
        // Get_piece
        clientlen = sizeof(clientaddr);
        
        /* accept a new connection from a client */
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        if(connfd < 0)
        {
            perror("Server Accept Failed:");
            break;
        }
        //char buf1[BUFFER_SIZE];
        bzero(buf1, BUFFER_SIZE);
        if(recv(connfd, buf1,BUFFER_SIZE, 0) < 0)
        {
            perror("Server Recieve Data Failed:");
            break;
        }
        //printf("the receive request buffer is %s\n ",buf1);
        
        // read client request about which file and which piece
        char num_temp[20];
        char pos_temp[20];
        sscanf(buf1, "%s %s %s %s", cmd,filename,num_temp,pos_temp);
        piece_num=atoi(num_temp);
        piece_pos=atoi(pos_temp);
        memset(buf1, 0, sizeof(buf1));
        // split the file
        if(strcmp(cmd,"Get_piece")==0){
            int size= getFileSize(filename);
            //printf("the file size is %d\n",size);
            //decide how many pieces to divide
            int piece_size = getPieceSize(size,piece_num);
            
            // divide the file
            char shell_cmd[BUFFER_SIZE];
            sprintf(shell_cmd,"%s %s %d %s","split","-b",piece_size,filename);
            int dummy = system(shell_cmd);
            
            
            // get the specific piece name
            int temp = 0xaa + piece_pos-1;
            char temp2[10];
            sprintf(temp2,"%x",temp);
            char piecename[100];
            sprintf(piecename,"%s%s","x",temp2);
            
            //and send the requested file back to client
            
            char buffer[BUFFER_SIZE];
            FILE *fp = fopen(piecename, "r");
            if(NULL == fp)
            {
                printf("File:%s Not Found\n", piecename);
            }
            else
            {
                bzero(buffer, BUFFER_SIZE);
                int length = 0;
                while((length = fread(buffer, sizeof(char), BUFFER_SIZE, fp)) > 0)
                {
                    if(send(connfd, buffer, length, 0) < 0)
                    {
                        printf("Send File:%s Failed./n", piecename);
                        break;
                    }
                    bzero(buffer, BUFFER_SIZE);
                }
                fclose(fp);
                //printf("File:%s Transfer Successful!\n", piecename);
            }
            close(connfd);
        }
        if(piece_num!=0){
            char *temp;
            char s[10] = "xaa";
            for(int i=0;i<piece_num;i++){
                char rec[MAXLINE];
                if(i!=0){
                    sprintf(rec,"%s %s",temp,s);
                }
                else{
                    sprintf(rec,"%s",s);
                }
                s[2]=s[2]+1;
                temp=strdup(rec);
            }
            char act[MAXLINE];
            sprintf(act,"%s %s","rm",temp);
            int dummy = system(act);
        }
    }
}


/*****main function*****/
int main(int argc, char *argv[]){
    
    //initialize user IP and port
	if (argc != 2) {
		printf("Usage: ./torrent [your port number]\n");
		exit(0);
	}
	myport = atoi(argv[1]);

	while(1) {
		printf("Are you using a virtual machine? Please type Yes or No:\n");
		char input[MAXLINE];
		int dum = scanf("%s",input);

		if(strcasecmp(input,"No")==0){
			int fd;
			struct ifreq ifr;
			fd = socket(AF_INET, SOCK_DGRAM, 0);
			ifr.ifr_addr.sa_family = AF_INET;
			strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);
			ioctl(fd, SIOCGIFADDR, &ifr);
			close(fd);
			myIP = strdup(inet_ntoa(((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr));
			break;
		}
		else if(strcasecmp(input,"Yes")==0){
			printf("Please input your IP of physical machine:\n");
			char inputIP[MAXLINE];
			int dum1 = scanf("%s",inputIP);
			myIP = strdup(inputIP);
			break;
		}
		else {
			printf("Wrong command, please try again!\n");
		}
	}
    
    
    
    //set different thread
    pthread_t tid_server;    
    Pthread_create(&tid_server,NULL,server,NULL);
    Pthread_detach(tid_server);
    
    //choose next action
    while(1){
        printf("Please choose your action: upload or download(or type ""quit"" to leave)\n");
    
        char in[MAXLINE];
        int dummy = scanf("%s",in);
    
        if(strncmp(in, "file", 4) == 0) {
        	for (int i = 0; i < 10; i++) {
        		if (filenames[i] != NULL) {
        			printf("%s\n", filenames[i]);
        		}
        		else continue;
        	}
        }
        else if(strncmp(in,"quit",4)==0){
        	char buf[MAXLINE];
        	for (int i = 0; i < 10; i++) {
        		if (filenames[i] != NULL) {
        			int clientfd = Open_clientfd(NodeIP, NodePort);
        			sprintf(buf, "%s %s %d %s", "Delete_tracker", myIP, myport, filenames[i]);
        			Rio_writen(clientfd, buf, strlen(buf));
        			memset(buf, 0, strlen(buf));
        			close(clientfd);
        		}
        	}
            break;
        }
    
        else if(strncmp(in, "upload", 6)==0){
            upload();
        }
    
        else if(strncmp(in, "download", 8)==0){
            download();
        }
    
        else{
            printf("Wrong command,please try again!\n");
        }
    }
    return 0;
}

void debug_info(){
    printf("total_num is %d\n",total_num);
    for(int i=0; i<total_num; i++){
        printf("User %d is IP %s port %d\n",i+1,user_list[i].IP,user_list[i].port);
    }
}
