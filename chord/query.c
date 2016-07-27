/*
 * query.c
 *
 *  Created on: Mar 21, 2015
 *      Author: ubuntu
 */
#include "chord.h"

//nodeInfo myinfo;


int main(int argc, char *argv[]) {
	int clientfd, clientlen;
	char buf[MAXLINE];
	char ipport[MAXLINE];
	char cmd[MAXLINE];
	char ip[MAXLINE], ports[MAXLINE], keyy[MAXLINE];
	char input[MAXLINE];
	rio_t client;

	if (argc != 3) {
		printf("Usage: ./%s ServerIP ServerPort\n", argv[0]);
		exit(1);
	}

	strcpy(ipport, argv[1]);
	strcat(ipport, " ");
	strcat(ipport, argv[2]);
	//printf("%s\n", ipport);
	unsigned key = hashFunction(ipport);
	unsigned skey;
	printf("Connection to node %s, port %s,position %u:\n", argv[1], argv[2], key);
	while(1) {
		memset(buf, 0, sizeof(buf));
		printf("Please enter your search key: (or type ""quit"" to leave)\n");
		int a = scanf("%s", input);
		if (strcmp(input, "quit") == 0) {
			break;
		}
		skey = hashFunction(input);

		sprintf(buf, "Query q q %u\r\n", skey);
		clientfd = open_clientfd(argv[1], atoi(argv[2]));
		if (clientfd == -1) {
			printf("The IP and port you entered is not in the ring.\n");
			exit(1);
		}
		printf("Hash value is %u\n", skey);
		printf("Response from node %s, port %s, position %u:\n", argv[1], argv[2], key);
		Rio_writen(clientfd, buf, strlen(buf));
		memset(buf, 0, sizeof(buf));
		Rio_readinitb(&client, clientfd);
		while(1) {
			Rio_readlineb(&client,buf,MAXLINE);
			//printf("%s", buf);
			if (strcmp(buf, "finish\r\n") == 0) {
				break;
			}
			sscanf(buf, "%s %s %s %s", cmd, ip, ports, keyy);
			//printf("%s, %s, %s, %s\n" ,cmd, ip, ports, keyy);
			if (strcmp(cmd, "ResQuery") == 0) {
				printf("Its position is node %s, port %s, position %s.\n\n", ip, ports, keyy);
			}
		}
		close(clientfd);
	}
	return 0;
}



