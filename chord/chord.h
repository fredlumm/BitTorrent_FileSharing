/*
 * chord.h
 *
 *  Created on: Mar 21, 2015
 *      Author: ubuntu
 */

#ifndef CHORD_H_
#define CHORD_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include "sha1.h"
#include <net/if.h>
#include "csapp.h"

#define   FILTER_FILE   "proxy.filter"
#define   LOG_FILE      "proxy.log"
#define   DEBUG_FILE	"proxy.debug"

/*
 * Variables and functions declaration
 */

/*  */
#define MAXUSER 10
#define MAXTRACKER 10

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

struct _nodeInfo {
  char IP[20];
  char port[8];
  unsigned key;
  int ttl;
  int flag;
};

Tracker tracker_arr[MAXTRACKER];
int tracker_num=0;

typedef struct _nodeInfo nodeInfo;

/* A global list to save node's successor and predecessor */
struct _fingerList {
	nodeInfo suc1; // direct successor
	nodeInfo suc2; // successor's successor
	nodeInfo pre1; // direct predecessor
	nodeInfo pre2; // predecessor's predecessor
};
typedef struct _fingerList fingerlist;

/* information of each finger in finger table */
struct _finger {
	nodeInfo node;
	unsigned start;
};
typedef struct _finger finger;

nodeInfo find_suc(nodeInfo srcinfo, nodeInfo destinfo);
nodeInfo find_pre(nodeInfo srcinfo, nodeInfo destinfo);
nodeInfo close_pre_finger(nodeInfo srcinfo, nodeInfo destinfo);
nodeInfo my_suc(nodeInfo srcinfo);
nodeInfo my_pre(nodeInfo srcinfo);

void nodeCopy(nodeInfo a, nodeInfo b);
void stabilize();
void fix_fingers();
void notify(nodeInfo destinfo);
void backup();
void reshape();
void reshape_user(char * filename);

void *refresh(void* args);
void *timer(void* args);
void *command(void* args);
void *returntracker(void* args);

/* hash function to deal with ip, port and keyword*/
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

#endif /* CHORD_H_ */
