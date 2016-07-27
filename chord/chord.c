/*
 * chord.c
 *
 *  Created on: Mar 18, 2015
 *      Author: Shuai Fu
 */
#include "chord.h"

finger fingertable[32];
int debug;
int debugfd;
int logfd;
nodeInfo info;
fingerlist list;
pthread_mutex_t mutex;




void debuginfo(){
    printf("I have %d trackers in total\n",tracker_num);
    for(int i=0;i < tracker_num;i++){
        printf("Tracker %d is %s\n",i+1, tracker_arr[i].filename);
        for(int j=0;j<tracker_arr[i].user_num;j++){
            printf("User %d: IP is %s, port is %d\n",j+1,tracker_arr[i].user_arr[j].IP,tracker_arr[i].user_arr[j].port);
        }
    }
}

int main(int argc, char *argv[])
{
	int clientfd, optval, clientlen;
    struct hostent *hp;
    char ipport[24];
    char serveripport[24];
    char buf[MAXLINE];
    char cmd[MAXLINE];
    char ip[MAXLINE], ports[MAXLINE], keyy[MAXLINE];
    char *cmdd, ipp, portss, keyyy;
    rio_t cli;
    sigset_t sig_pipe;
    pthread_t freshfd, timerfd, commandfd, returntrackerfd;
    int *args;

    /* Judge the command */
    if (argc != 2 && argc != 4) {
    	printf("Usage: ./%s port [ServerIP] [ServerPort]\n", argv[0]);
    	exit(1);
    }

    /* Get IP address of local host */
    int fd;
    struct ifreq ifr;
    fd = socket(AF_INET, SOCK_DGRAM, 0);

    /* I want to get an IPv4 IP address */
    ifr.ifr_addr.sa_family = AF_INET;

    /* I want IP address attached to "eth0" */
    strncpy(ifr.ifr_name, "eth0", IFNAMSIZ-1);
    ioctl(fd, SIOCGIFADDR, &ifr);
    close(fd);
    strcpy(info.IP, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
    info.ttl = 5;

    /*
     * If the user wants to create a new ring, initialize
     * the node's list and finger table with itself
     */
    if (argc == 2) {
    	/* Get the hash value of IP and port */
    	strcpy(info.port, argv[1]);
    	strcpy(ipport, info.IP);
    	strcat(ipport, " ");
    	strcat(ipport, info.port);
    	info.key = hashFunction(ipport);
    	printf("Creating the chord ring\n");
    	printf("You are listening on port %s\n", info.port);
    	printf("Your position is %u\n", info.key);

    	/* Initialize the list */
    	strcpy(list.suc1.IP, info.IP);
    	strcpy(list.suc1.port, info.port);
    	list.suc1.key = info.key;
    	list.suc1.ttl = 5;
    	strcpy(list.suc2.IP, info.IP);
    	strcpy(list.suc2.port, info.port);
    	list.suc2.key = info.key;
    	list.suc2.ttl = 5;
    	strcpy(list.pre1.IP, info.IP);
    	strcpy(list.pre1.port, info.port);
    	list.pre1.key = info.key;
    	list.pre1.ttl = 5;
    	strcpy(list.pre2.IP, info.IP);
    	strcpy(list.pre2.port, info.port);
    	list.pre2.key = info.key;
    	list.pre2.ttl = 5;

    	/* Initialize the finger table */
    	for (int i = 0; i < 32; i++) {
    		strcpy(fingertable[i].node.IP, info.IP);
    		strcpy(fingertable[i].node.port, info.port);
    		fingertable[i].node.key = info.key;
    		fingertable[i].node.ttl = 5;
    		unsigned temp = info.key + pow(2, i);
    		if (temp > pow(2, 32)) {
    			temp = temp - pow(2, 32);
    		}
    		fingertable[i].start = temp;
    	}
    	//printf("Your predecessor is node %s, port %s, position %u.\n", list.pre1.IP, list.pre1.port, list.pre1.key);
    	//printf("Your successor is node %s, port %s, position %u.\n", list.suc1.IP, list.suc1.port, list.suc1.key);
    }

    /*
     * If the user wants to add a node to a existed ring, initialize myself
     * and ask the server node what my successor is.
     */
    if (argc == 4) {
    	/* Get the hash value of IP and port */
    	strcpy(info.port, argv[1]);
    	strcpy(ipport, info.IP);
    	strcat(ipport, " ");
    	strcat(ipport, info.port);
    	info.key = hashFunction(ipport);
    	printf("Joining the chord ring\n");
    	printf("You are listening on port %s\n", info.port);
    	printf("Your position is %u\n", info.key);

    	/* Initialize the list */
    	strcpy(list.suc1.IP, info.IP);
    	strcpy(list.suc1.port, info.port);
    	list.suc1.key = info.key;
    	list.suc1.ttl = 5;
    	strcpy(list.suc2.IP, info.IP);
    	strcpy(list.suc2.port, info.port);
    	list.suc2.key = info.key;
    	list.suc2.ttl = 5;
    	strcpy(list.pre1.IP, info.IP);
    	strcpy(list.pre1.port, info.port);
    	list.pre1.key = info.key;
    	list.pre1.ttl = 5;
    	strcpy(list.pre2.IP, info.IP);
    	strcpy(list.pre2.port, info.port);
    	list.pre2.key = info.key;
    	list.pre2.ttl = 5;

    	/* Initialize the finger table */
    	for (int i = 0; i < 32; i++) {
    		strcpy(fingertable[i].node.IP, info.IP);
    		strcpy(fingertable[i].node.port, info.port);
    		fingertable[i].node.key = info.key;
    		fingertable[i].node.ttl = 5;
    		unsigned temp = info.key + pow(2, i);
    		if (temp > pow(2, 32)) {
    			temp = temp - pow(2, 32);
    		}
    		fingertable[i].start = temp;
    	}

    	/* Begin asking for my successor */
    	clientfd = open_clientfd(argv[2], atoi(argv[3]));
    	if (clientfd == -1) {
    		printf("The IP and port you entered is not in the ring.\n");
    		exit(1);
    	}
    	strcpy(cmd, "Position");
    	sprintf(buf, "%s %s %s %u\r\n", cmd, info.IP, info.port, info.key);
    	Rio_writen(clientfd, buf, strlen(buf));
    	memset(buf, 0, sizeof(buf));
    	Rio_readinitb(&cli, clientfd);
    	while(1) {
    		Rio_readlineb(&cli,buf,MAXLINE);
    		if (strcmp(buf, "finish\r\n") == 0) {
    			break;
    		}
    		sscanf(buf, "%s %s %s %s", cmd, ip, ports, keyy);
    		/* If I get the response from server, record my successor and let my predecessor = nil */
    		if (strcmp(cmd, "ResPosition") == 0) {
    			strcpy(list.pre1.IP, "0");
    			strcpy(list.pre1.port, "0");
    			list.pre1.key = 0;
    			strcpy(list.suc1.IP, ip);
    			strcpy(list.suc1.port, ports);
    			list.suc1.key = atoi(keyy);
    			list.suc1.ttl = 5;
    		}
    	}
    	close(clientfd);

    }



    /* Create three threads, one for refreshing the list and table, one for detecting node leaving,
     * and one for dealing with user input */
    Pthread_create(&freshfd, NULL, refresh, NULL);
    Pthread_detach(freshfd);
    Pthread_create(&timerfd, NULL, timer, NULL);
    Pthread_detach(timerfd);
    Pthread_create(&commandfd, NULL, command, NULL);
    Pthread_detach(commandfd);
    Pthread_create(&returntrackerfd, NULL, returntracker, NULL);
    Pthread_detach(returntrackerfd);

    /* Prepare to listen as a server */
    int listenfd, connfd;
    struct sockaddr_in clientaddr;
    rio_t client;
    unsigned key;
    char buf1[MAXLINE];
    listenfd = Open_listenfd(atoi(info.port));
    optval = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int));
    if(debug)
    	debugfd = Open(DEBUG_FILE, O_CREAT | O_TRUNC | O_WRONLY, 0666);
    logfd = Open(LOG_FILE, O_CREAT | O_TRUNC | O_WRONLY, 0666);
    /* if writing to log files, force each thread to grab a lock before writing
       to the files */
    while(1) {
    	clientlen = sizeof(clientaddr);

    	/* accept a new connection from a client */
    	connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
    	Rio_readinitb(&client, connfd);
    	Rio_readlineb(&client,buf1,MAXLINE);
    	sscanf(buf1, "%s %s %s %s", cmd, ip, ports, keyy);
    	//printf("%s", buf1);
    	memset(buf1, 0, sizeof(buf1));
    	

        /* If client needs one file's tracker, find it and send back */
        if (strcmp(cmd, "Get_Tracker") == 0) {
            int state = 0;
            int flag;
            for(int i=0; i<tracker_num; i++){
                if(strcmp(tracker_arr[i].filename,ip)==0){
                    state = 1;
                    flag = i;
                    break;
                }
            }
            
            if(state == 0){
                Rio_writen(connfd, "error\r\n", strlen("error\r\n"));
            }
            else{
                sprintf(buf1,"total %d %s %s\r\n",tracker_arr[flag].user_num,"NOP","NOP");
                Rio_writen(connfd, buf1, strlen(buf1));
                memset(buf1, 0, strlen(buf1));
                for(int i=0;i<tracker_arr[flag].user_num;i++){
                    sprintf(buf1,"user %d %s %d\r\n",i+1,tracker_arr[flag].user_arr[i].IP,tracker_arr[flag].user_arr[i].port);
                    Rio_writen(connfd, buf1, strlen(buf1));
                    memset(buf1, 0, strlen(buf1));
                }
    			Rio_writen(connfd, "finish\r\n", strlen("finish\r\n"));
                memset(buf1, 0, strlen(buf1));
            }
    	}
        
        /* If client wants to update one tracker, accept it */
    	if (strcmp(cmd, "Update_Tracker") == 0) {
            int flag;
            for(int i=0; i<tracker_num; i++){
                if(strcmp(tracker_arr[i].filename,ip)==0){
                    flag = i;
                    break;
                }
            }
            
            int temp = 0;
            for(int i=0;i<tracker_arr[flag].user_num;i++){
                if(strcmp(tracker_arr[flag].user_arr[i].IP,ports)==0 && (tracker_arr[flag].user_arr[i].port==atoi(keyy))){
                    temp = 1;
                    break;
                }
            }
            
            if(temp == 0){
                tracker_arr[flag].user_num++;
                tracker_arr[flag].user_arr[tracker_arr[flag].user_num-1].IP = strdup(ports);
                tracker_arr[flag].user_arr[tracker_arr[flag].user_num-1].port = atoi(keyy);
            }
            
            Rio_writen(connfd, "Complete\r\n", strlen("Complete\r\n"));
            memset(buf1, 0, strlen(buf1));
    	}
        
        
    	/* If client needs its successor, find it and send back */
    	if (strcmp(cmd, "Position") == 0) {
            key = atoi(keyy);
    		nodeInfo destinfo, suc;
    		strcpy(destinfo.IP, ip);
    		strcpy(destinfo.port, ports);
    		destinfo.key = key;
    		suc = find_suc(info, destinfo);
    		sprintf(buf1, "ResPosition %s %s %u\r\n", suc.IP, suc.port, suc.key);
    		Rio_writen(connfd, buf1, strlen(buf1));
    		Rio_writen(connfd, "finish\r\n", strlen("finish\r\n"));
    	}
        
    	/* If client needs my successor, send it back */
    	if (strcmp(cmd, "Successor") == 0) {
            key = atoi(keyy);
    		sprintf(buf1, "ResSuccessor %s %s %u\r\n", list.suc1.IP, list.suc1.port, list.suc1.key);
    		Rio_writen(connfd, buf1, strlen(buf1));
    		Rio_writen(connfd, "finish\r\n", strlen("finish\r\n"));
    	}

    	/* If client needs my predecessor, send it back */
    	if (strcmp(cmd, "Predecessor") == 0) {
            key = atoi(keyy);
    		sprintf(buf1, "ResPredecessor %s %s %u\r\n", list.pre1.IP, list.pre1.port, list.pre1.key);
    		Rio_writen(connfd, buf1, strlen(buf1));
    		Rio_writen(connfd, "finish\r\n", strlen("finish\r\n"));
    	}

    	/* If client needs to find its closest predecessor in my finger table, send it back */
    	if (strcmp(cmd, "Closepre") == 0) {
            key = atoi(keyy);
    		int bool = 0;
    		for (int i = 31; i >= 0; i--) {
    			if ((fingertable[i].node.key < atoi(keyy) && fingertable[i].node.key > info.key) ||
    				(info.key > atoi(keyy) && fingertable[i].node.key > info.key) ||
    				(info.key > atoi(keyy) && fingertable[i].node.key < atoi(keyy))) {
    				sprintf(buf1, "ResClosepre %s %s %u\r\n", fingertable[i].node.IP, fingertable[i].node.port,fingertable[i].node.key);
    				Rio_writen(connfd, buf1, strlen(buf1));
    				Rio_writen(connfd, "finish\r\n", strlen("finish\r\n"));
    				bool = 1;
    			}
    		}
    		if (bool == 0) {
    			sprintf(buf1, "ResClosepre %s %s %u\r\n", info.IP, info.port,info.key);
    			Rio_writen(connfd, buf1, strlen(buf1));
    			Rio_writen(connfd, "finish\r\n", strlen("finish\r\n"));
    		}
    	}

    	/* If client notify me that I'm its successor, upgrade my predecessor */
    	if (strcmp(cmd, "Notify") == 0) {
    		/* If client is between my predecessor and I, use it as my new predecessor */
            key = atoi(keyy);
    		if (((strcmp(list.pre1.IP, "0") == 0) && (strcmp(list.pre1.port, "0") == 0)) ||
    			(key > list.pre1.key && key < info.key) ||
    			(list.pre1.key > info.key && key > list.pre1.key) ||
    			(list.pre1.key > info.key && key < info.key) ||
    			(list.pre1.key == info.key)) {
    			strcpy(list.pre1.IP, ip);
    			strcpy(list.pre1.port, ports);
    			list.pre1.key = key;
    			list.pre1.ttl = 5;
    		}
    		/* If I'm notified, refresh the time to live of my predecessor*/
    		list.pre1.ttl = 5;
    	}

    	/* If client wants to query the position of a key, find it and send back */
    	if (strcmp(cmd, "Query") == 0) {
            key = atoi(keyy);
    		nodeInfo temp;
    		nodeInfo qsuc;
    		temp.key = key;
    		strcpy(temp.IP, ip);
    		strcpy(temp.port, ports);
    		qsuc = find_suc(info, temp);
    		sprintf(buf1, "ResQuery %s %s %u\r\n", qsuc.IP, qsuc.port,qsuc.key);
    		//printf("%s", buf1);
    		Rio_writen(connfd, buf1, strlen(buf1));
    		Rio_writen(connfd, "finish\r\n", strlen("finish\r\n"));
    	}
        
        // set up new tracker
        if(strcmp(cmd,"NewTracker")==0){
            nodeInfo destinfo, suc;
            strcpy(destinfo.IP, ip);
            strcpy(destinfo.port, ports);
            destinfo.key = hashFunction(keyy);
            suc = find_suc(info, destinfo);
            // get the successor, send the info to successor
            if(strcmp(suc.port,info.port)==0){
                if(tracker_num==0){
                    tracker_arr[tracker_num].filename = strdup(keyy);
                    tracker_arr[tracker_num].user_num = 1;
                    tracker_arr[tracker_num].user_arr[0].IP = strdup(ip);
                    tracker_arr[tracker_num].user_arr[0].port = atoi(ports);
                    tracker_num++;
                }
                else{
                	int flag = 0;
                	int index = 0;
                	for(int i=0; i<tracker_num; i++){
                		if(strcmp(tracker_arr[i].filename,keyy)==0){
                			flag = 1;
                			index = i;
                			break;
                		}
                	}
                	if(flag == 1){
                		int temp = 0;
                		for(int i=0;i<tracker_arr[index].user_num;i++){
                			if(strcmp(tracker_arr[index].user_arr[i].IP,ip)==0 && (tracker_arr[index].user_arr[i].port==atoi(ports))){
                				temp = 1;
                				break;
                			}
                		}
                		if(temp==0){
                			tracker_arr[index].user_arr[tracker_arr[index].user_num].IP = strdup(ip);
                			tracker_arr[index].user_arr[tracker_arr[index].user_num].port = atoi(ports);
                			tracker_arr[index].user_num++;
                		}
                	}
                	else{
                		tracker_arr[tracker_num].filename = strdup(keyy);
                		tracker_arr[tracker_num].user_num = 1;
                		tracker_arr[tracker_num].user_arr[0].IP = strdup(ip);
                		tracker_arr[tracker_num].user_arr[0].port = atoi(ports);
                		tracker_num++;
                	}
                }
                //debuginfo();
            }
            else{
                int clientfd = open_clientfd(suc.IP,atoi(suc.port));
                if (clientfd == -1) {
                    printf("The IP and port you entered is not in the ring.\n");
                    exit(1);
                }
                sprintf(buf1,"%s %s %s %s\r\n","SaveTracker",ip,ports,keyy);
                Rio_writen(clientfd, buf1, strlen(buf1));
                close(clientfd);
            }
        }
        if(strcmp(cmd,"SaveTracker")==0){
            if(tracker_num==0){
                tracker_arr[tracker_num].filename = strdup(keyy);
                tracker_arr[tracker_num].user_num = 1;
                tracker_arr[tracker_num].user_arr[0].IP = strdup(ip);
                tracker_arr[tracker_num].user_arr[0].port = atoi(ports);
                tracker_num++;
            }
            else{
                int flag = 0;
                int index = 0;
                for(int i=0; i<tracker_num; i++){
                    if(strcmp(tracker_arr[i].filename,keyy)==0){
                        flag = 1;
                        index = i;
                        break;
                    }
                }
                if(flag == 1){
                    int temp = 0;
                    for(int i=0;i<tracker_arr[index].user_num;i++){
                        if(strcmp(tracker_arr[index].user_arr[i].IP,ip)==0 && (tracker_arr[index].user_arr[i].port==atoi(ports))){
                            temp = 1;
                            break;
                        }
                    }
                    if(temp==0){
                        tracker_arr[index].user_arr[tracker_arr[index].user_num].IP = strdup(ip);
                        tracker_arr[index].user_arr[tracker_arr[index].user_num].port = atoi(ports);
                        tracker_arr[index].user_num++;
                    }
                }
                else{
                    tracker_arr[tracker_num].filename = strdup(keyy);
                    tracker_arr[tracker_num].user_num = 1;
                    tracker_arr[tracker_num].user_arr[0].IP = strdup(ip);
                    tracker_arr[tracker_num].user_arr[0].port = atoi(ports);
                    tracker_num++;
                }
            }
            //debuginfo();
        }
        /* If client need get its tracker array back, send them back. */
        if (strcmp(cmd, "Return_tracker") == 0) {
        	//printf("%s\n", cmd);
        	nodeInfo pre, now;
        	strcpy(now.IP, ip);
        	strcpy(now.port, ports);
        	now.key = atoi(keyy);
        	pre = my_pre(now);
        	int k = tracker_num;
        	for (int m = 0; m < k; m++) {
        		for (int i = 0; i < tracker_num; i++) {
        			if ((hashFunction(tracker_arr[i].filename) < now.key && hashFunction(tracker_arr[i].filename) > pre.key) ||
        					(pre.key > now.key && hashFunction(tracker_arr[i].filename) < now.key) ||
        					(pre.key > now.key && hashFunction(tracker_arr[i].filename) > pre.key)) {
        				for (int j = 0; j < tracker_arr[i].user_num; j++) {
        					int returnfd = Open_clientfd(ip, atoi(ports));
        					if (returnfd == 1) {
        						//printf("Return error.\n");
        						continue;
        					}
        					sprintf(buf1,"%s %s %d %s\r\n","SaveTracker",tracker_arr[i].user_arr[j].IP,tracker_arr[i].user_arr[j].port,tracker_arr[i].filename);
        					//printf("%s\n", buf1);
        					Rio_writen(returnfd, buf1, strlen(buf1));
        					close(returnfd);
        					tracker_arr[i].user_arr[j].IP = NULL;
        					tracker_arr[i].user_arr[j].port = 0;
        				}
        				tracker_arr[i].filename = NULL;
        				tracker_arr[i].user_num = 0;
        				tracker_num--;
        				reshape();
        			}
        		}
        	}

        }

        if (strcmp(cmd, "Delete_tracker") == 0) {
        	unsigned key = hashFunction(keyy);
        	nodeInfo temp;
        	nodeInfo qsuc;
        	temp.key = key;
        	strcpy(temp.IP, ip);
        	strcpy(temp.port, ports);
        	qsuc = find_suc(info, temp);
        	//printf("%d\n", qsuc.key);
        	if(qsuc.key == info.key) {
        		for (int i = 0; i < tracker_num; i++) {
        			if (strcmp(tracker_arr[i].filename, keyy) == 0) {
        				for (int j = 0; j < tracker_arr[i].user_num; j++) {
        					if (strcmp(tracker_arr[i].user_arr[j].IP, ip) == 0 && tracker_arr[i].user_arr[j].port == atoi(ports)) {
        						tracker_arr[i].user_arr[j].IP = NULL;
        						tracker_arr[i].user_arr[j].port = 0;
        						tracker_arr[i].user_num--;
        						reshape_user(keyy);
        						break;
        					}
        				}
        				if (tracker_arr[i].user_num == 0) {
        					tracker_arr[i].filename = NULL;
        					tracker_num--;
        					reshape();
        				}
        			}
        		}
        	}
        	else {
        		int clientfd = open_clientfd(qsuc.IP,atoi(qsuc.port));
        		if (clientfd == -1) {
        			printf("The IP and port you entered is not in the ring.\n");
        			exit(1);
        		}
        		sprintf(buf1,"%s %s %s %s\r\n","Delete_tracker_here",ip,ports,keyy);
        		Rio_writen(clientfd, buf1, strlen(buf1));
        		close(clientfd);
        	}
        }

        if (strcmp(cmd, "Delete_tracker_here") == 0) {
        	for (int i = 0; i < tracker_num; i++) {
        		if (strcmp(tracker_arr[i].filename, keyy) == 0) {
        			for (int j = 0; j < tracker_arr[i].user_num; j++) {
        				if (strcmp(tracker_arr[i].user_arr[j].IP, ip) == 0 && tracker_arr[i].user_arr[j].port == atoi(ports)) {
        					tracker_arr[i].user_arr[j].IP = NULL;
        					tracker_arr[i].user_arr[j].port = 0;
        					tracker_arr[i].user_num--;
        					reshape_user(keyy);
        					break;
        				}
        			}
        			if (tracker_arr[i].user_num == 0) {
        				tracker_arr[i].filename = NULL;
        				tracker_num--;
        				reshape();
        			}
        		}
        	}
        }
    	close(connfd);
    }
    if(debug) Close(debugfd);
    Close(logfd);
    return 0;
}

/* A thread to ensure the correctness of my list */
void *timer(void* args) {
	while(1) {
		/* The ttl of my successor and predecessor minus 1 every 1 second */
		sleep(1);
		list.suc1.ttl --;
		list.pre1.ttl --;
		//printf("suc1 ttl: %d\n", list.suc1.ttl);
		//printf("pre1 ttl: %d\n", list.pre1.ttl);

		/* If my successor or predecessor is not refreshed for 5 seconds, drop it and
		 * use the 2nd successor or predecessor instead */
		if (list.suc1.ttl == 0) {
			list.suc1 = list.suc2;
			list.suc1.ttl = 5;
		}
		if (list.pre1.ttl == 0) {
			list.pre1 = list.pre2;
			list.pre1.ttl = 5;
		}
	}
	return NULL;
}

void *command(void* args) {
	char input[MAXLINE];
	while(1) {
		memset(input, 0, sizeof(input));
		printf("\nPlease enter the information you want to know:\n");
		printf("Enter 'my' to see my IP, port and key.\n");
		printf("Enter 'list' to see predecessor and successor.\n");
		printf("Enter 'table' to see the whole finger table.\n");
		printf("Enter 'tracker' to see all the trackers.\n");
		printf("Enter 'quit' to shut down the node peacefully.\n");
		int a = scanf("%s", input);
		printf("\n");
		if (strcmp(input, "my") == 0) {
			printf("Your IP is %s, port is %s, position is %u.\n",info.IP, info.port, info.key);
		}
		if (strcmp(input, "list") == 0) {
			printf("Your predecessor is node %s, port %s, position %u.\n",
					list.pre1.IP, list.pre1.port, list.pre1.key);
			printf("Your successor is node %s, port %s, position %u.\n",
					list.suc1.IP, list.suc1.port, list.suc1.key);
		}
		if (strcmp(input, "table") == 0) {
			for (int i = 0 ;i < 32; i++) {
				if (i < 10) {
					printf("Index 0%d, IP %s, Port %s, Node key %u, Finger %u\n",
					i, fingertable[i].node.IP, fingertable[i].node.port, fingertable[i].node.key, fingertable[i].start);
				}
				else {
					printf("Index %d, IP %s, Port %s, Node key %u, Finger %u\n",
					i, fingertable[i].node.IP, fingertable[i].node.port, fingertable[i].node.key, fingertable[i].start);
				}
			}
		}
		/* The node transfer its tracker array to its successor when shutting down. */
		if (strcmp(input, "quit") == 0) {
			char buf1[MAXLINE];
			for (int i = 0; i < tracker_num; i++) {
				for (int j = 0; j < tracker_arr[i].user_num; j++) {
					int clientfd = open_clientfd(list.suc1.IP,atoi(list.suc1.port));
					if (clientfd == -1) {
						printf("The IP and port you entered is not in the ring.\n");
						exit(1);
					}
					sprintf(buf1,"%s %s %d %s\r\n","SaveTracker",tracker_arr[i].user_arr[j].IP,tracker_arr[i].user_arr[j].port,tracker_arr[i].filename);
					Rio_writen(clientfd, buf1, strlen(buf1));
					close(clientfd);
				}
			}
			exit(1);
		}
		if (strcmp(input, "tracker") == 0) {
			debuginfo();
		}
	}
	return NULL;
}

/* A thread to refresh my list and finger table */
void *refresh(void* args) {
	while (1) {
		usleep(100000);
		stabilize();
		backup();
		fix_fingers();
	}
	return NULL;
}

void *returntracker(void* args) {

	sleep(1);
	char buf[MAXLINE];
	/* Get the tracker array belongs to me*/
	if (list.suc1.key != info.key) {
		int clientfd = open_clientfd((list.suc1.IP), atoi(list.suc1.port));
		if (clientfd == -1) {
			//printf("succ error.\n");
			return NULL;
		}
		sprintf(buf, "%s %s %s %u\r\n", "Return_tracker", info.IP, info.port, info.key);
		//printf("send %s\n", buf);
		Rio_writen(clientfd, buf, strlen(buf));
		close(clientfd);
	}
	return NULL;
}

/* Get my newest successor and notify it */
void stabilize() {
	nodeInfo x;
	if (info.key == list.suc1.key) {
		x = list.pre1;
		list.suc1 = x;
		list.suc1.ttl = 5;
		list.pre1.ttl = 5;
	}
	else {
		x = my_pre(list.suc1);
		if (x.flag == -1) {
			return;
		}
		if ((x.key > info.key && x.key < list.suc1.key) ||
			(info.key > list.suc1.key && x.key > info.key) ||
			(info.key > list.suc1.key && x.key < list.suc1.key)) {
			list.suc1 = x;
		}
		list.suc1.ttl = 5;
	}
	if (info.key != list.suc1.key) {
		notify(list.suc1);
	}
}

/* Tell my newest successor that I'm your newest predecessor */
void notify(nodeInfo destinfo) {
	int clientfd;
	char buf[MAXLINE];
	char cmd[MAXLINE], ip[MAXLINE], ports[MAXLINE], keyy[MAXLINE];
	nodeInfo pre;
	rio_t client;
	clientfd = open_clientfd(destinfo.IP, atoi(destinfo.port));
	if (clientfd == -1) {
		return;
	}
	sprintf(buf, "Notify %s %s %u\r\n", info.IP, info.port, info.key);
	Rio_writen(clientfd, buf, strlen(buf));
	close(clientfd);
}

/* Refresh my finger table, one random finger each time */
void fix_fingers() {
	fingertable[0].node = list.suc1;
	int i = rand()%32;
	nodeInfo temp;
	nodeInfo judge;
	temp.key = fingertable[i].start;
	strcpy(temp.IP, "fix");
	strcmp(temp.port, "fix");
	judge = find_suc(info, temp);
	if (judge.flag == -1) {
		return;
	}
	else {
		fingertable[i].node = judge;
	}
}

/* Get my successor's successor and my predecessor's predecessor */
void backup() {
	if (list.suc1.key == info.key) {
		list.suc2 = info;
	}
	else {
		nodeInfo temp = my_suc(list.suc1);
		if (temp.flag == -1) {
			return;
		}
		else {
			list.suc2 =	temp;
		}
	}
	if (list.pre1.key == info.key) {
		list.pre2 = info;
	}
	else if(strcmp(list.pre1.port, "0") == 0 && strcmp(list.pre1.IP, "0") == 0){

	}
	else {
		nodeInfo temp = my_pre(list.pre1);
		if (temp.flag == -1) {
			return;
		}
		else {
			list.pre2 =	temp;
		}
	}
}

/* Deep copy two nodes */
void nodeCopy(nodeInfo a, nodeInfo b) {
	strcpy(a.IP, b.IP);
	strcpy(a.port, b.port);
	a.key = b.key;
	a.ttl = b.ttl;
}

/* Search for the right successor of a specific key */
nodeInfo find_suc(nodeInfo srcinfo, nodeInfo destinfo) {
	nodeInfo pre, suc;
	if (srcinfo.key == list.suc1.key) {
		return srcinfo;
	}
	//printf("findsuc dest: %u\n", destinfo.key);
	pre = find_pre(srcinfo, destinfo);
	if (pre.flag == -1) {
		return pre;
	}
	if (pre.key == info.key) {
		suc = list.suc1;
	}
	else {
		suc = my_suc(pre);
	}
	return suc;
}

/* Search for the right predecessor of a specific key */
nodeInfo find_pre(nodeInfo srcinfo, nodeInfo destinfo) {
	nodeInfo re, suc;
	re = srcinfo;
	//printf("findpre dest: %u\n", destinfo.key);
	if ((destinfo.key > re.key && destinfo.key <= list.suc1.key) ||
		(re.key > list.suc1.key && destinfo.key > re.key) ||
		(re.key > list.suc1.key && destinfo.key <= list.suc1.key)) {
		return re;
	}
	else {
		while(1) {
			if (re.key == info.key) {
				if (info.key < destinfo.key) {
					for (int i = 31; i >= 0; i--) {
						if (fingertable[i].node.key > info.key && fingertable[i].node.key < destinfo.key) {
							re = fingertable[i].node;
							break;
						}
					}
				}
				else {
					for (int i = 31; i >=0; i--) {
						if (fingertable[i].node.key > info.key || fingertable[i].node.key < destinfo.key) {
							re = fingertable[i].node;
							break;
						}
					}
				}
			}
			else {
				re = close_pre_finger(re, destinfo);
			}
			suc = my_suc(re);
			if (suc.flag == -1) {
				return suc;
			}
			if ((destinfo.key > re.key && destinfo.key <= suc.key) ||
				(re.key > suc.key && destinfo.key > re.key) ||
				(re.key > suc.key && destinfo.key < suc.key)) {
				return re;
			}
		}
	}
}

/* Tell the server that I need to find the closest predecessor of a key in its finger table */
nodeInfo close_pre_finger(nodeInfo srcinfo, nodeInfo destinfo) {
	int clientfd;
	char buf[MAXLINE];
	char cmd[MAXLINE], ip[MAXLINE], ports[MAXLINE], keyy[MAXLINE];
	nodeInfo re;
	nodeInfo false;
	rio_t client;
	false.flag = -1;
	clientfd = open_clientfd(srcinfo.IP, atoi(srcinfo.port));
	if (clientfd == -1) {
		return false;
	}
	sprintf(buf, "Closepre %s %s %u\r\n", destinfo.IP, destinfo.port, destinfo.key);
	Rio_writen(clientfd, buf, strlen(buf));
	memset(buf, 0, sizeof(buf));
	Rio_readinitb(&client, clientfd);
	while(1) {
		Rio_readlineb(&client,buf,MAXLINE);
		if (strcmp(buf, "finish\r\n") == 0) {
			break;
		}
		sscanf(buf, "%s %s %s %s", cmd, ip, ports, keyy);
		if (strcmp(cmd, "ResClosepre") == 0) {
			strcpy(re.IP, ip);
			strcpy(re.port, ports);
			re.key = atoi(keyy);
			re.ttl = 5;
		}
	}
	close(clientfd);
	return re;
}

/* Get the successor of a node */
nodeInfo my_suc(nodeInfo srcinfo) {
	int clientfd;
	char buf[MAXLINE];
	char cmd[MAXLINE], ip[MAXLINE], ports[MAXLINE], keyy[MAXLINE];
	nodeInfo suc;
	nodeInfo false;
	rio_t client;
	false.flag = -1;
	clientfd = open_clientfd(srcinfo.IP, atoi(srcinfo.port));
	if (clientfd == -1) {
		return false;
	}
	sprintf(buf, "Successor %s %s %u\r\n", srcinfo.IP, srcinfo.port, srcinfo.key);
	Rio_writen(clientfd, buf, strlen(buf));
	memset(buf, 0, sizeof(buf));
	Rio_readinitb(&client, clientfd);
	while(1) {
		Rio_readlineb(&client,buf,MAXLINE);
		if (strcmp(buf, "finish\r\n") == 0) {
			break;
		}
		sscanf(buf, "%s %s %s %s", cmd, ip, ports, keyy);
		if (strcmp(cmd, "ResSuccessor") == 0) {
			strcpy(suc.IP, ip);
			strcpy(suc.port, ports);
			suc.key = atoi(keyy);
			suc.ttl = 5;
		}
	}
	close(clientfd);
	return suc;
}

/* Get the predecessor of a node */
nodeInfo my_pre(nodeInfo srcinfo){
	int clientfd;
	char buf[MAXLINE];
	char cmd[MAXLINE], ip[MAXLINE], ports[MAXLINE], keyy[MAXLINE];
	nodeInfo pre;
	nodeInfo false;
	rio_t client;
	false.flag = -1;
	clientfd = open_clientfd(srcinfo.IP, atoi(srcinfo.port));
	if (clientfd == -1) {
		return false;
	}
	sprintf(buf, "Predecessor %s %s %u\r\n", srcinfo.IP, srcinfo.port, srcinfo.key);
	Rio_writen(clientfd, buf, strlen(buf));
	memset(buf, 0, sizeof(buf));
	Rio_readinitb(&client, clientfd);
	while(1) {
		Rio_readlineb(&client,buf,MAXLINE);
		if (strcmp(buf, "finish\r\n") == 0) {
			break;
		}
		sscanf(buf, "%s %s %s %s", cmd, ip, ports, keyy);
		if (strcmp(cmd, "ResPredecessor") == 0) {
			strcpy(pre.IP, ip);
			strcpy(pre.port, ports);
			pre.key = atoi(keyy);
			pre.ttl = 5;
		}
	}
	close(clientfd);
	return pre;
}

void reshape() {
	for (int i = 0; i < tracker_num; i++) {
		if (tracker_arr[i].filename == NULL) {
			tracker_arr[i].filename = strdup(tracker_arr[tracker_num].filename);
			for(int j = 0; j < tracker_arr[tracker_num].user_num; j++) {
				tracker_arr[i].user_arr[j].IP = strdup(tracker_arr[tracker_num].user_arr[j].IP);
				tracker_arr[tracker_num].user_arr[j].IP = NULL;
				tracker_arr[i].user_arr[j].port = tracker_arr[tracker_num].user_arr[j].port;
				tracker_arr[tracker_num].user_arr[j].port = 0;
				tracker_arr[i].user_num++;
			}
			tracker_arr[tracker_num].filename = NULL;
			tracker_arr[tracker_num].user_num = 0;
		}
	}
}

void reshape_user(char * filename) {
	for (int i = 0; i < tracker_num; i++) {
		if (strcmp(tracker_arr[i].filename, filename) == 0) {
			for(int j = 0; j < tracker_arr[i].user_num; j++) {
				if (tracker_arr[i].user_arr[j].IP == NULL) {
					tracker_arr[i].user_arr[j].IP = strdup(tracker_arr[i].user_arr[tracker_arr[i].user_num].IP);
					tracker_arr[i].user_arr[j].port = tracker_arr[i].user_arr[tracker_arr[i].user_num].port;
				}
			}
			tracker_arr[i].user_arr[tracker_arr[i].user_num].IP = NULL;
			tracker_arr[i].user_arr[tracker_arr[i].user_num].port = 0;
		}
	}
}
