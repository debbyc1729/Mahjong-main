#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "MahjongCard.h"

#define PORT 5555

//message codes
//server -> client
#define GAMEOVER	0 //server -> client
#define KONG		1 //server -> client
#define SEE		4 //server -> client
#define ASK		5 //server -> client
#define GIVE		7 //server -> client
#define CHI		9 //server -> client
#define PONG		11 //server -> client
#define KONG_PONG	12 // GAME?
#define RON		13 // GAME?
#define WINNER		14 // GAME?

//client -> server
#define NICKNAME	2 //client -> server
#define PLAY		6 //client -> server
#define EMPTY		8 //client -> server
#define SCORE		10 //client -> server

//common
#define DISCONNECT	3 //either way

#define MAX_PLAYERS 4
#define NAMESIZE 20
#define MESSAGE_SIZE 82


typedef struct mathopt
{
	int oprate;
	float value1;
	float value2;
}mopt;

typedef struct player
{
	int order;
	int order_now;
	
	int sign;
	char msg[256];
	
	int Hand[CARD_HAND];
	int HandNow;
	
	int discard;
	int Hand_len;
	
	int num_pong;
	int num_chi;
	int num_kong;
	
	int pong[4][3];
	int chi[4][3];
	int kong[4][4];
	
	int temp[20];
	
	int ready_hand[5];
}player;


void send_msg(int clientsocket, char* buf, int len_buf){
	int error = send(clientsocket, buf, len_buf,0);
	if(error < 0){
		perror("send error");
		//break;
	}
}




int main(int argc, char *argv[])//int main()
{//printf("aaa!!!!");
	struct sockaddr_in serverAddr;
	int clientSocket;
	char sendbuf[200];
	char command[20];
	int dis_card;
	if((clientSocket=socket(AF_INET, SOCK_STREAM,0)) < 0)
	{
		perror( "error: create socket");
		return -1;
	}
	serverAddr.sin_family=AF_INET;
	serverAddr.sin_port=htons(PORT);
	serverAddr.sin_addr.s_addr=inet_addr(argv[1]);
	if(connect(clientSocket,(struct sockaddr * )&serverAddr,sizeof(serverAddr)) < 0)
	{
		perror("error: connect remote server");
		exit(1);
	}
	else{
		char recvbuf[1024];
		printf("infor: connect with destinationhost........[ok]\n");
		int error = recv(clientSocket,recvbuf,1024,0);
		if(error < 0){
			printf("recv error");
			//break;
		}
		printf("%s\n", recvbuf);
		char buf[100];
		int Size = sizeof(struct player);
		while(1){ ////Game Start!!!
			//printf("------------------------------------------------\n");
			char pack[1024];
			
			int error = recv(clientSocket,pack,1024,0);
			if(error < 0){
				perror("	recv error");
				//break;
			}
			//printf("	recv end\n");
			
			
			struct player *pMp = (struct player*)pack;
			
			
			int ss = pMp->sign ;
			printf("ss: %d", ss);
			if(ss == GAMEOVER){
				
				
				printf("Game Over!!\n");
				break;
			
			
			}
			if(ss == ASK){
				printf("------------------------------------------------\n");
				printf(" your card: \n");
				for(int i=0;i<pMp->Hand_len;i++){
					char *tt = get_card_name(pMp->Hand[i]);
					printf("%s,", tt);
				}
				if((pMp->HandNow)!=-1){
					printf(" %s", get_card_name(pMp->HandNow));
				}
				printf("	");
				if((pMp->num_pong)>0){
					for(int i=0;i<pMp->num_pong;i++){
						if(pMp->pong[i][0] != -1)
							printf(" %s %s %s", get_card_name(pMp->pong[i][0])
									  , get_card_name(pMp->pong[i][1])
									  , get_card_name(pMp->pong[i][2]));
					}
				}
				if((pMp->num_chi)>0){
					for(int i=0;i<pMp->num_chi;i++){
						printf(" %s %s %s", get_card_name(pMp->chi[i][0])
								  , get_card_name(pMp->chi[i][1])
								  , get_card_name(pMp->chi[i][2]));
					}
				}
				
				if((pMp->num_kong)>0){
					for(int i=0;i<pMp->num_kong;i++){
						printf(" %s %s %s %s", get_card_name(pMp->kong[i][0])
									, get_card_name(pMp->kong[i][1])
									, get_card_name(pMp->kong[i][2])
									, get_card_name(pMp->kong[i][3]));
					}
				}
				printf("\n");
			
				char cardbuf[1024];
				char buf[100];
				
				printf("%s(0~%d)", pMp->msg, pMp->Hand_len);
				//scanf("%d",&dis_card);
				while(scanf("%d", &dis_card)){
					if(dis_card<0 || dis_card>pMp->Hand_len){
						printf("Please enter 0 ~ %d: ",pMp->Hand_len);
					}
					else{
						//printf("yes");
						break;
					}
				}
				printf("You Discarded: (%d)%s\n", dis_card, get_card_name(pMp->Hand[dis_card]));
				sprintf(buf,"%d",dis_card);
				send_msg(clientSocket,buf,sizeof(buf));
				//printf("	send end\n");
			
			
			}
			if(ss == SEE){
				
				
				printf(" %s.\n", pMp->msg);
			
			
			}
			if (ss == CHI){
				printf("Your Card: \n");
				for(int i=0;i<pMp->Hand_len;i++){
					char *tt = get_card_name(pMp->Hand[i]);
					printf("%s,", tt);
				}
				if((pMp->HandNow)!=-1){
					printf(" %s", get_card_name(pMp->HandNow));
				}
				printf("	");
				if((pMp->num_pong)>0){
					for(int i=0;i<pMp->num_pong;i++){
						if(pMp->pong[i][0] != -1)
							printf(" %s %s %s", get_card_name(pMp->pong[i][0])
									  , get_card_name(pMp->pong[i][1])
									  , get_card_name(pMp->pong[i][2]));
					}
				}
				if((pMp->num_chi)>0){
					for(int i=0;i<pMp->num_chi;i++){
						printf(" %s %s %s", get_card_name(pMp->chi[i][0])
								  , get_card_name(pMp->chi[i][1])
								  , get_card_name(pMp->chi[i][2]));
					}
				}
				
				if((pMp->num_kong)>0){
					for(int i=0;i<pMp->num_kong;i++){
						printf(" %s %s %s %s", get_card_name(pMp->kong[i][0])
									, get_card_name(pMp->kong[i][1])
									, get_card_name(pMp->kong[i][2])
									, get_card_name(pMp->kong[i][3]));
					}
				}
				printf("\n");
				for(int i=0;pMp->temp[i]!=-1;i++){
					printf(" %s",get_card_name(pMp->Hand[pMp->temp[i]]));
					if(i%2==1){
						printf(", ");
					}
				}
				printf("\n");
				
				printf("%s.\n", pMp->msg);
				char request[20];
				//scanf("%s",command);
				while(scanf("%s",command)){
				printf("	command: %c\n",command[0]);
					if(strncmp(command,"Y",1) == 0|| strncmp(command,"N",1) == 0){
						break;
					}
					else{
						//printf("yes");
						printf("Please enter Y or N: ");
					}
					scanf("Which?(1/2/3)%s",command);
				}
				char yes[]="Y";
				printf("	strncmp: %d\n",strncmp(command,yes,1));
				printf("	yes: %c\n",yes[0]);
				sprintf(request,"%s",command);
				int number = 0;
				if(strncmp(command,yes,1) == 0){
					printf("Which?(1/2/3)");
					//scanf("Which?(1/2/3)%s",command);
					//scanf("%s",command);
					while(scanf("%d",number)){
						printf("number : %d \n",number);
						if(number < pMp->num_chi){
							//printf("yes");
							break;
						}
						else{
							printf("Please enter 1 ~ %d: ", pMp->num_chi - 1);
						}
						//scanf("Which?(1/2/3)%s",command);
					}
					sprintf(request,"%s %d",request, number);
				}
				printf("	send ing\n");
				perror("	sprintf error");
				send_msg(clientSocket,request,sizeof(request));
				printf("	send end\n");
			}
			if (ss == PONG){
				printf(" your card: \n");
				for(int i=0;i<pMp->Hand_len;i++){
					char *tt = get_card_name(pMp->Hand[i]);
					printf("%s,", tt);
				}
				if((pMp->HandNow)!=-1){
					printf(" %s", get_card_name(pMp->HandNow));
				}
				printf("	");
				if((pMp->num_pong)>0){
					for(int i=0;i<pMp->num_pong;i++){
						if(pMp->pong[i][0] != -1)
							printf(" %s %s %s", get_card_name(pMp->pong[i][0])
									  , get_card_name(pMp->pong[i][1])
									  , get_card_name(pMp->pong[i][2]));
					}
				}
				if((pMp->num_chi)>0){
					for(int i=0;i<pMp->num_chi;i++){
						printf(" %s %s %s", get_card_name(pMp->chi[i][0])
								  , get_card_name(pMp->chi[i][1])
								  , get_card_name(pMp->chi[i][2]));
					}
				}
				
				if((pMp->num_kong)>0){
					for(int i=0;i<pMp->num_kong;i++){
						printf(" %s %s %s %s", get_card_name(pMp->kong[i][0])
									, get_card_name(pMp->kong[i][1])
									, get_card_name(pMp->kong[i][2])
									, get_card_name(pMp->kong[i][3]));
					}
				}
				printf("\n");
				for(int i=0;pMp->temp[i]!=-1;i++){
					printf(" %s",get_card_name(pMp->Hand[pMp->temp[i]]));
					if(i%2==1){
						printf(", ");
					}
				}
				printf("\n");
				
				printf("%s.\n", pMp->msg);
				char request[20];
				//scanf("%s",command);
				while(scanf("%s",command)){
				printf("	command: %c\n",command[0]);
					if(strncmp(command,"Y",1) == 0|| strncmp(command,"N",1) == 0){
						//printf("yes");
						break;
					}
					else{
						printf("Please enter Y or N: ");
					}
					scanf("Which?(1/2/3)%s",command);
				}
				char yes[]="Y";
				printf("	strncmp: %d\n",strncmp(command,yes,1));
				printf("	yes: %c\n",yes[0]);
				sprintf(request,"%s",command);
				printf("	send ing\n");
				perror("	sprintf error");
				send_msg(clientSocket,request,sizeof(request));
				printf("	send end\n");
			}
			if (ss == KONG_PONG){
				printf(" your card: \n");
				for(int i=0;i<pMp->Hand_len;i++){
					char *tt = get_card_name(pMp->Hand[i]);
					printf("%s,", tt);
				}
				if((pMp->HandNow)!=-1){
					printf(" %s", get_card_name(pMp->HandNow));
				}
				printf("	");
				if((pMp->num_pong)>0){
					for(int i=0;i<pMp->num_pong;i++){
						if(pMp->pong[i][0] != -1)
							printf(" %s %s %s", get_card_name(pMp->pong[i][0])
									  , get_card_name(pMp->pong[i][1])
									  , get_card_name(pMp->pong[i][2]));
					}
				}
				if((pMp->num_chi)>0){
					for(int i=0;i<pMp->num_chi;i++){
						printf(" %s %s %s", get_card_name(pMp->chi[i][0])
								  , get_card_name(pMp->chi[i][1])
								  , get_card_name(pMp->chi[i][2]));
					}
				}
				
				if((pMp->num_kong)>0){
					for(int i=0;i<pMp->num_kong;i++){
						printf(" %s %s %s %s", get_card_name(pMp->kong[i][0])
									, get_card_name(pMp->kong[i][1])
									, get_card_name(pMp->kong[i][2])
									, get_card_name(pMp->kong[i][3]));
					}
				}
				printf("\n");
				for(int i=0;pMp->temp[i]!=-1;i++){
					printf(" %s",get_card_name(pMp->Hand[pMp->temp[i]]));
					if(i%2==1){
						printf(", ");
					}
				}
				printf("\n");
				
				printf("%s.\n", pMp->msg);
				char request[20];
				//scanf("%s",command);
				while(scanf("%s",command)){
				printf("	command: %c\n",command[0]);
					if(strncmp(command,"Y",1) == 0|| strncmp(command,"N",1) == 0){
						//printf("yes");
						break;
					}
					else{
						printf("Please enter Y or N: ");
					}
					scanf("Which?(1/2/3)%s",command);
				}
				char yes[]="Y";
				printf("	strncmp: %d\n",strncmp(command,yes,1));
				printf("	yes: %c\n",yes[0]);
				sprintf(request,"%s",command);
				if(strncmp(command,yes,1) == 0){
					printf("Which?(1:PONG/2:KONG)");
					scanf("Which?(1/2/3)%s",command);
					//scanf("%s",command);
					while(scanf("%s",command)){
						if(strncmp(command,"Y",1) == 0|| strncmp(command,"N",1) == 0){
							//printf("yes");
							break;
						}
						else{
							printf("Please enter Y or N: ");
						}
						scanf("Which?(1/2/3)%s",command);
					}
					printf("command : %s\n",command);
					sprintf(request,"%s", command);
				}
				printf("	send ing\n");
				perror("	sprintf error");
				send_msg(clientSocket,request,sizeof(request));
				printf("	send end\n");
			}
			if (ss == KONG){
				printf(" your card: \n");
				for(int i=0;i<pMp->Hand_len;i++){
					char *tt = get_card_name(pMp->Hand[i]);
					printf("%s,", tt);
				}
				if((pMp->HandNow)!=-1){
					printf(" %s", get_card_name(pMp->HandNow));
				}
				printf("	");
				if((pMp->num_pong)>0){
					for(int i=0;i<pMp->num_pong;i++){
						if(pMp->pong[i][0] != -1)
							printf(" %s %s %s", get_card_name(pMp->pong[i][0])
									  , get_card_name(pMp->pong[i][1])
									  , get_card_name(pMp->pong[i][2]));
					}
				}
				if((pMp->num_chi)>0){
					for(int i=0;i<pMp->num_chi;i++){
						printf(" %s %s %s", get_card_name(pMp->chi[i][0])
								  , get_card_name(pMp->chi[i][1])
								  , get_card_name(pMp->chi[i][2]));
					}
				}
				
				if((pMp->num_kong)>0){
					for(int i=0;i<pMp->num_kong;i++){
						printf(" %s %s %s %s", get_card_name(pMp->kong[i][0])
									, get_card_name(pMp->kong[i][1])
									, get_card_name(pMp->kong[i][2])
									, get_card_name(pMp->kong[i][3]));
					}
				}
				printf("\n");
				for(int i=0;pMp->temp[i]!=-1;i++){
					printf(" %s",get_card_name(pMp->Hand[pMp->temp[i]]));
					if(i%2==1){
						printf(", ");
					}
				}
				printf("\n");
				
				printf("%s.\n", pMp->msg);
				char request[20];
				//scanf("%s",command);
				while(scanf("%s",command)){
				printf("	command: %c\n",command[0]);
					if(strncmp(command,"Y",1) == 0|| strncmp(command,"N",1) == 0){
						//printf("yes");
						break;
					}
					else{
						printf("Please enter Y or N: ");
					}
					scanf("Which?(1/2/3)%s",command);
				}
				char yes[]="Y";
				printf("	strncmp: %d\n",strncmp(command,yes,1));
				printf("	yes: %c\n",yes[0]);
				sprintf(request,"%s",command);
				printf("	send ing\n");
				perror("	sprintf error");
				send_msg(clientSocket,request,sizeof(request));
				printf("	send end\n");
			}
			if (ss == RON){
				printf(" your card: \n");
				for(int i=0;i<pMp->Hand_len;i++){
					char *tt = get_card_name(pMp->Hand[i]);
					printf("%s,", tt);
				}
				if((pMp->HandNow)!=-1){
					printf(" %s", get_card_name(pMp->HandNow));
				}
				printf("	");
				if((pMp->num_pong)>0){
					for(int i=0;i<pMp->num_pong;i++){
						if(pMp->pong[i][0] != -1)
							printf(" %s %s %s", get_card_name(pMp->pong[i][0])
									  , get_card_name(pMp->pong[i][1])
									  , get_card_name(pMp->pong[i][2]));
					}
				}
				if((pMp->num_chi)>0){
					for(int i=0;i<pMp->num_chi;i++){
						printf(" %s %s %s", get_card_name(pMp->chi[i][0])
								  , get_card_name(pMp->chi[i][1])
								  , get_card_name(pMp->chi[i][2]));
					}
				}
				
				if((pMp->num_kong)>0){
					for(int i=0;i<pMp->num_kong;i++){
						printf(" %s %s %s %s", get_card_name(pMp->kong[i][0])
									, get_card_name(pMp->kong[i][1])
									, get_card_name(pMp->kong[i][2])
									, get_card_name(pMp->kong[i][3]));
					}
				}
				printf("\n");
				for(int i=0;pMp->temp[i]!=-1;i++){
					printf(" %s",get_card_name(pMp->Hand[pMp->temp[i]]));
					if(i%2==1){
						printf(", ");
					}
				}
				printf("\n");
				
				printf("%s.\n", pMp->msg);
				char request[20];
				//scanf("%s",command);
				while(scanf("%s",command)){
					
				printf("	command: %c\n",command[0]);
					if(strncmp(command,"Y",1) == 0|| strncmp(command,"N",1) == 0){
						//printf("yes");
						break;
					}
					else{
						printf("Please enter Y or N: ");
					}
					scanf("Which?(1/2/3)%s",command);
				}
				char yes[]="Y";
				printf("	strncmp: %d\n",strncmp(command,yes,1));
				printf("	yes: %c\n",yes[0]);
				sprintf(request,"%s",command);
				printf("	send ing\n");
				perror("	sprintf error");
				send_msg(clientSocket,request,sizeof(request));
				printf("	send end\n");
			}
			if(ss == GIVE){
				printf(" your card: ");
				for(int i=0;i<CARD_HAND;i++){
					char *tt = get_card_name(pMp->Hand[i]);
					printf("%s,", tt);
				}
				printf("\n\n");
				
			
			
			}
			if(ss == WINNER){
				
				printf(" %s.\n", pMp->msg);
			
			
			}
		}
	}
	close(clientSocket);
	return 0;
}
