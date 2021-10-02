#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <stdbool.h>
#include <unistd.h>

#include "MahjongCard.h"

#define SERVER_PORT 5555

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

bool card_names_check[CARD_MAX]={false};

int Desk[4][34];
int Player_desk[4]={0};

int CHECK_ANY(struct player *pl, int *p,int clientsocket[]);



int cmpfunc (const void * a, const void * b)
{
   return ( *(int*)a - *(int*)b );
}


void send_msg(int clientsocket, char* buf, int len_buf){
	int error = send(clientsocket, buf, len_buf,0);
	if(error < 0){
		perror("send error");
	}
}

void swap(int* a, int* b){
	int c;
	c = *a; 
	*a = *b; 
	*b = c; 
}
void find_card_in_hand(int *card, int *hand, int *hand_len, int *HandNow){
	for(int i=0;i<*hand_len;i++){
		if(*(hand+i) == *card){
			swap(hand+i,HandNow);
			break;
		}
		
	}
}

//struct player *player, int dis_card, int clientsocket
void Ask_player_discard(int num, struct player *player,int clientsocket, int flag){///////////////////////////////////////////
	char dis_card[20];
	player->sign = ASK;
	char buf[1024] = "Input you want to discard Card:>";
	strcpy(player->msg, buf);
	
	send_msg(clientsocket,(char *)player,sizeof(*player));
	
	int error = recv(clientsocket,dis_card,1024,0);
	if(error < 0){
		perror("recv error");
		//break;
	}
	int dd = atoi(dis_card);
	
	//printf("discard: %s,%d,%s,%d\n", dis_card,dd, get_card_name(player->Hand[dd]), player->Hand_len);
	printf("Player %d Discard: %s\n", num, get_card_name(player->Hand[dd]));
	
	if(dd != (player->Hand_len)){
		//printf("find_card_in_hand start\n");
		if(flag == 1){
			find_card_in_hand(&player->Hand[dd], player->Hand
					, &player->Hand_len, &player->HandNow);
			qsort(player->Hand, player->Hand_len, sizeof(int), cmpfunc);
		}
		if(flag == 2){
			find_card_in_hand(&player->Hand[dd], player->Hand
					, &player->Hand_len, &player->Hand[player->Hand_len-1]);
			player->HandNow = player->Hand[player->Hand_len-1];
			player->discard = player->Hand[player->Hand_len-1];
			player->Hand[player->Hand_len-1]=-1;
			player->Hand_len-=1;
			qsort(player->Hand, player->Hand_len, sizeof(int), cmpfunc);
		}
		//printf("find_card_in_hand end: %s\n",get_card_name(player->HandNow));
	}

}/**/



int find(int *head,int *tail,int value){
	int *t;
	t = head;
	for(int i=0;head!=tail;head++,i++){
		if((*head)/4 == value){
			return i;
		}
	}
	return -1;
}

int Sequence(int discard,struct player *player, int *sq){ ////////////////////////////////////Debug
	int len = player->Hand_len;
	int *head = player-> Hand;
	int i,j;
	int sq_inx=0, f=0;
	
	if(discard>=28){
		int max=0,min=0;
		if(discard>=28 && discard<64){
			min=28,max=64;
		}
		if(discard>=64 && discard<100){
			min=64,max=100;
		}
		if(discard>=100 && discard<136){
			min=100,max=136;
		}
		if((i=find(head,head+len,discard/4+1))!=-1 && (j=find(head,head+len,discard/4+2))!=-1){
			if(player-> Hand[i]>=min && player-> Hand[i]<max && player-> Hand[j]>=min && player-> Hand[j]<max){
				*(sq+sq_inx) = i;
				sq_inx++;
				*(sq+sq_inx) = j;
				sq_inx++;
				f=1;
			}
		} 
		if((i=find(head,head+len,discard/4-1))!=-1 && (j=find(head,head+len,discard/4+1))!=-1){
			if(player-> Hand[i]>=min && player-> Hand[i]<max && player-> Hand[j]>=min && player-> Hand[j]<max){
				*(sq+sq_inx) = i;
				sq_inx++;
				*(sq+sq_inx) = j;
				sq_inx++;
				f=1;
			}
		} 
		if((i=find(head,head+len,discard/4-1))!=-1 && (j=find(head,head+len,discard/4-2))!=-1){
			if(player-> Hand[i]>=min && player-> Hand[i]<max && player-> Hand[j]>=min && player-> Hand[j]<max){
				*(sq+sq_inx) = i;
				sq_inx++;
				*(sq+sq_inx) = j;
				sq_inx++;
				f=1;
			}
		}
		if(f==1){
			*(sq+sq_inx) = -1;
			return 1;
		}
		//}
	}
	return 0;
}

int CHECK_CHI(struct player *player, int dis_card, int clientsocket){
	usleep(1);
	char buf[200];
	
	//吃牌
	//printf("player.order_now: %d,player.order: %d\n", player->order_now,player->order);
	//if(player->order_now == player->order-1){//上家
		int sq[6]={-1};
		//printf("CHECK CHI!!!!!! \n");
		if(Sequence(player->discard, player, player->temp)){//順子
			player->sign = CHI;
			sprintf(buf, "CHI ?(Y/N)");
			
			strcpy(player->msg, buf);
			send_msg(clientsocket,(char *)player,sizeof(*player));
			printf("	CHI send end\n");
			
			char message[20];
			int error = recv(clientsocket,message,20,0);
			if(error < 0){
				perror("recv error");
				//break;
			}
			printf("	CHI recv end\n");
			printf("	message: %s\n", message);
			if(strncmp(message,"Y 1",3) == 0){//吃牌
				printf("	CHI Y 1\n");
				player->chi[player->num_chi][0]=player->Hand[player->temp[0]];
				player->chi[player->num_chi][1]=player->Hand[player->temp[1]];
				player->chi[player->num_chi][2]=player->discard;
				
				printf("CHI:%d %d %d\n"
					,player->chi[player->num_chi][0]
					,player->chi[player->num_chi][1]
					,player->chi[player->num_chi][2]);
				player->num_chi++;
				
				swap(&player->Hand[player->temp[0]],&player->Hand[player->Hand_len-1]);
				swap(&player->Hand[player->temp[1]],&player->Hand[player->Hand_len-2]);
				player->discard=-1;
				player->Hand_len-=2;
				qsort(player->Hand, player->Hand_len, sizeof(int), cmpfunc);
				
				return 1;
			}
			if(strncmp(message,"Y 2",3) == 0){//吃牌
				printf("	CHI Y 2\n");
				player->chi[player->num_chi][0]=player->Hand[player->temp[2]];
				player->chi[player->num_chi][1]=player->Hand[player->temp[3]];
				player->chi[player->num_chi][2]=player->discard;
				
				printf("CHI:%d %d %d\n"
					,player->chi[player->num_chi][0]
					,player->chi[player->num_chi][1]
					,player->chi[player->num_chi][2]);
				player->num_chi++;
				
				swap(&player->Hand[player->temp[2]],&player->Hand[player->Hand_len-1]);
				swap(&player->Hand[player->temp[3]],&player->Hand[player->Hand_len-2]);
				player->discard=-1;
				player->Hand_len-=2;
				qsort(player->Hand, player->Hand_len, sizeof(int), cmpfunc);
				
				return 1;
			}
			if(strncmp(message,"Y 3",3) == 0){//吃牌
				printf("	CHI Y 3\n");
				player->chi[player->num_chi][0]=player->Hand[player->temp[4]];
				player->chi[player->num_chi][1]=player->Hand[player->temp[5]];
				player->chi[player->num_chi][2]=player->discard;
				
				printf("CHI:%d %d %d\n"
					,player->chi[player->num_chi][0]
					,player->chi[player->num_chi][1]
					,player->chi[player->num_chi][2]);
				player->num_chi++;
				
				swap(&player->Hand[player->temp[4]],&player->Hand[player->Hand_len-1]);
				swap(&player->Hand[player->temp[5]],&player->Hand[player->Hand_len-2]);
				player->discard=-1;
				player->Hand_len-=2;
				qsort(player->Hand, player->Hand_len, sizeof(int), cmpfunc);
				
				return 1;
			}
		}
	//}
	return 0;
}
//struct player *player, int dis_card, int clientsocket
int CHECK_PONG(struct player *player, int dis_card, int clientsocket){
	usleep(1);
	int len = player -> Hand_len;
	int *head = player -> Hand;
	int i, j;
	char buf[200];

        if((i = find(head, head+len, dis_card/4))!=-1){
		if((player -> Hand[i])/4 == (player -> Hand[i+1])/4){
			if((player -> Hand[i+1])/4 == (player -> Hand[i+2]/4)){//槓 KONG_PONG
				player->sign = KONG_PONG;
				player->temp[0]=i;
				player->temp[1]=i+1;
				player->temp[2]=i+2;
				player->temp[3]=-1;
				sprintf(buf, "PONG or KONG ?(Y/N)");
				
				strcpy(player->msg, buf);
				send_msg(clientsocket,(char *)player,sizeof(*player));
				printf("	KONG_PONG send end\n");
				
				char message[20];
				int error = recv(clientsocket,message,20,0);
				if(error < 0){
					perror("recv error");
					//break;
				}
				//printf("	KONG_PONG recv end\n");
				printf("	message: %s\n", message);
				int dd = atoi(message);
				if(dd == 1){//碰牌(1)
					printf("	PONG(KONG_PONG) Y\n");
					player->pong[player->num_pong][0]=player->Hand[player->temp[0]];////
					player->pong[player->num_pong][1]=player->Hand[player->temp[1]];
					player->pong[player->num_pong][2]=player->discard;
					
					printf("PONG:%d %d %d\n"
						,player->pong[player->num_pong][0]
						,player->pong[player->num_pong][1]
						,player->pong[player->num_pong][2]);
					player->num_pong++;
					
					swap(&player->Hand[player->temp[0]],&player->Hand[player->Hand_len-1]);
					swap(&player->Hand[player->temp[1]],&player->Hand[player->Hand_len-2]);
					player->discard=-1;
					player->Hand_len-=2;
					qsort(player->Hand, player->Hand_len, sizeof(int), cmpfunc);
					
					return 1;
				}
				if(dd == 2){//槓牌(2)
				
					printf("	KONG Y\n");
					player->kong[player->num_kong][0]=player->Hand[player->temp[0]];////
					player->kong[player->num_kong][1]=player->Hand[player->temp[1]];
					player->kong[player->num_kong][2]=player->Hand[player->temp[2]];
					player->kong[player->num_kong][3]=player->discard;
					
					printf("KONG:%d %d %d %d\n"
						,player->kong[player->num_kong][0]
						,player->kong[player->num_kong][1]
						,player->kong[player->num_kong][2]
						,player->kong[player->num_kong][3]);
					player->num_kong++;
					
					swap(&player->Hand[player->temp[0]],&player->Hand[player->Hand_len-1]);
					swap(&player->Hand[player->temp[1]],&player->Hand[player->Hand_len-2]);
					swap(&player->Hand[player->temp[2]],&player->Hand[player->Hand_len-3]);
					player->discard=-1;
					player->Hand_len-=2;
					qsort(player->Hand, player->Hand_len, sizeof(int), cmpfunc);
					
					//摸牌
					int n = rand() % CARD_MAX;
					while(card_names_check[n]){
						n = rand() % CARD_MAX;////
						
					}
					player->Hand[player->Hand_len-1] = n;
					card_names_check[n] = true;
					
                    			return 1;
				}
                    	}
                    	else{
				//執行碰動作 
				player->sign = PONG;
				player->temp[0]=i;
				player->temp[1]=i+1;
				player->temp[2]=-1;
				sprintf(buf, "PONG ?(Y/N)");
				
				strcpy(player->msg, buf);
				send_msg(clientsocket,(char *)player,sizeof(*player));
				//printf("	PONG send end\n");
				
				char message[20];
				int error = recv(clientsocket,message,20,0);
				if(error < 0){
					perror("recv error");
					//break;
				}
				//printf("	PONG recv end\n");
				//printf("	message: %s\n", message);
				if(strncmp(message,"Y",1) == 0){//碰牌(1/2)
					//printf("	PONG Y\n");
					player->pong[player->num_pong][0]=player->Hand[player->temp[0]];////
					player->pong[player->num_pong][1]=player->Hand[player->temp[1]];
					player->pong[player->num_pong][2]=player->discard;
					
					printf("PONG:%d %d %d\n"
						,player->pong[player->num_pong][0]
						,player->pong[player->num_pong][1]
						,player->pong[player->num_pong][2]);
					player->num_pong++;
					
					swap(&player->Hand[player->temp[0]],&player->Hand[player->Hand_len-1]);
					swap(&player->Hand[player->temp[1]],&player->Hand[player->Hand_len-2]);
					player->discard=-1;
					player->Hand_len-=2;
					qsort(player->Hand, player->Hand_len, sizeof(int), cmpfunc);
					
		        		return 1;
				}
                	}
		}
                
	}

	return 0;
}

int find_Sq(int (*pong)[3], int num_pong, int target){//pong num_pong
	for(int i=0;i<num_pong;i++){
		if(pong[i][0]/4 == target/4){
			return i;
		}
	
	}
	return -1;
}

int CHECK_KONG(struct player *player, int dis_card, int clientsocket){
	usleep(1);
	int len = player -> Hand_len;
	int *head = player -> Hand;
	int i, j;
	char buf[200];

	//printf("CHECK KONG!!!!!! \n");
        if((i = find(head, head+len, dis_card/4))!=-1 || find_Sq(player->pong, player->num_pong, dis_card)!=-1){ //no加槓
        	
		if(find_Sq(player->pong, player->num_pong, dis_card)!=-1 || 
		   (player -> Hand[i])/4 == (player -> Hand[i+1])/4){
        		//printf("	2\n");
        		int n=-1;
			if((n = find_Sq(player->pong, player->num_pong, dis_card))!=-1 || 
			   (player -> Hand[i+1])/4 == (player -> Hand[i+2]/4)){//槓 KONG
        			printf("	KONG start\n");
				player->sign = KONG;
				if(n != -1){
					player->temp[0]=player->pong[n][0];
					player->temp[1]=player->pong[n][1];
					player->temp[2]=player->pong[n][2];
					//player->temp[3]=player->pong[n][2];
					player->temp[3]=-1;
				}
				else{
					player->temp[0]=i;
					player->temp[1]=i+1;
					player->temp[2]=i+2;
					player->temp[3]=-1;
				}
				sprintf(buf, "KONG ?(Y/N)");
				
				strcpy(player->msg, buf);
				send_msg(clientsocket,(char *)player,sizeof(*player));
				printf("	KONG send end\n");
				
				char message[20];
				int error = recv(clientsocket,message,20,0);
				if(error < 0){
					perror("recv error");
					//break;
				}
				printf("	KONG recv end\n");
				printf("	message: %s\n", message);
				
				if(strncmp(message,"Y",1) == 0){//槓牌
				
					printf("	KONG Y\n");
					player->kong[player->num_kong][0]=player->Hand[player->temp[0]];////
					player->kong[player->num_kong][1]=player->Hand[player->temp[1]];
					player->kong[player->num_kong][2]=player->Hand[player->temp[2]];
					player->kong[player->num_kong][3]=player->HandNow;
					
					printf("KONG:%d %d %d %d\n"
						,player->kong[player->num_kong][0]
						,player->kong[player->num_kong][1]
						,player->kong[player->num_kong][2]
						,player->kong[player->num_kong][3]);
					player->num_kong++;
					
					swap(&player->Hand[player->temp[0]],&player->Hand[player->Hand_len-1]);
					swap(&player->Hand[player->temp[1]],&player->Hand[player->Hand_len-2]);
					swap(&player->Hand[player->temp[2]],&player->Hand[player->Hand_len-3]);
					player->HandNow=-1;
					player->Hand_len-=2;
					qsort(player->Hand, player->Hand_len, sizeof(int), cmpfunc);
					
					if(n != -1){
						player->pong[n][0] = -1;
						player->pong[n][1] = -1;
						player->pong[n][2] = -1;
					}
					
					//摸牌
					int n = rand() % CARD_MAX;
					while(card_names_check[n]){
						n = rand() % CARD_MAX;////
						
					}
					player->Hand[player->Hand_len-1] = n;
					card_names_check[n] = true;
					
                    			return 1;
				}
                    	}
                    	
		}
                
	}

	return 0;
}

int CHECK_RON(struct player *player, int key){//和牌：key 
	//printf("	CHECK_RON start\n");
	/*int hand[13];
	int count = 0;
	for(int i = 0; i < len(player.Hand); i++, count++)
		hand[count] = player.Hand[i];
	for(int i = 0; i < len(player.Hand); i++, count++)
		hand[count] = player.chi[i];
	for(int i = 0; i < len(player.Hand); i++, count++)
		hand[count] = player.Hand[i];*/
	//手牌 
	int lengh = CARD_HAND+1;//len(player.Hand)+1;////
	int temp[lengh];
	for(int i = 0, j=0; i < lengh-1; i++,j++){
		/*if(player.Hand[i] == 200){
			
		}
		else if(player.pong[4][3]){
			
		}*/
		temp[i] = player->Hand[i]/4;
	}
	//printf("	1\n");
	temp[lengh] = key/4;
	qsort(temp, lengh, sizeof(int), cmpfunc);
	//printf("	2\n");
	//一般胡牌 
	int flag = 1;//1：胡牌、0：沒有胡牌 
	int eye = 0;
	for(int i = 0; i < lengh; ){
		//printf("	3\n");
		if(temp[i] + 1 == temp[i+1]){
			//printf("	4\n");
			if(temp[i+1] + 1 == temp[i+2]){
				//printf("	5\n");
				i += 3;
			}
			else{
				i += 1;
			}
		}
		else if(temp[i] == temp[i+1]){
			//printf("	6\n");
			if(temp[i+1] == temp[i+2]){//三個一對 
				//printf("	7\n");
				i += 3;
			}
			else{//眼睛 
				//printf("	8\n");
				eye++;
				if(eye > 1)//眼睛超過一對不到胡牌條件 
					flag = 0;
				i += 2;
			}
		}
		else{
				//printf("	9\n");
			flag = 0;
			i += 1; 
		}   
	}
	//printf("	10\n");
	if(flag == 1)
		return 1;//胡牌
	else
		flag = 1; 
    
	//printf("	11\n");
	//特殊牌型(七對子)
	if(lengh == 13){
				//printf("	12\n");
		int three = 0;
		for(int i = 0; i < 13; i++){
				//printf("	13\n");
			if(temp[i] == temp[i+1]){
				//printf("	14\n");
				if(temp[i+1] == temp[i+2]){
				//printf("	15\n");
					three++;
					if(three > 1)
						flag = 0;
					i += 3;
				}
				else{
				//printf("	16\n");
					i += 2;
				}
			}
		}
				//printf("	17\n");
		if(flag == 1)
			return 1;//胡牌
		else
			flag = 1;
	}
    return 0;
	//特殊牌型(國士無雙)
	//((’_?`)) 
}

void display(struct player *player,int play, int dis_card, int clientsocket){
	usleep(1);
	char buf[100];
	player->sign = SEE;
	sprintf(buf,"Player %d Discard: %s",play, get_card_name(dis_card));
	
	
	strcpy(player->msg, buf);
	send_msg(clientsocket,(char *)player,sizeof(*player));
}

void winner(struct player *player,int play, int clientsocket, int flag){
	usleep(1);
	char buf[100];
	player->sign = WINNER;
	if(flag == 1){
		sprintf(buf,"Player %d WIN!!",play);
	}
	else{
		sprintf(buf,"YOU WIN!!");
	}
	
	
	strcpy(player->msg, buf);
	send_msg(clientsocket,(char *)player,sizeof(*player));

}

int CHECK_ANY(struct player *pl, int *p,int clientsocket[]){
	int index=(*p)%4;
	char buf[20];
	for(int i=0;i<4;i++) ////show other player discard 
	{
		if(index != i){
			//printf("show other player discard \n");
			//printf("	1 \n");
			display(&pl[i], index, pl[index].discard, clientsocket[i]);
		
			//printf("pl[index].discard: %d\n", pl[index].discard);
		}
		
		
	}
	//胡牌
	for(int i=0;i<4;i++){
		if(index != i){
			if(CHECK_RON(&pl[index], pl[index].HandNow) == 1){
				//printf("CHECK_RON (2) == 1\n");
				//Ask ron?
				pl[index].sign = RON;
				sprintf(buf, "RON ?(Y/N)");
				
				strcpy(pl[index].msg, buf);
				send_msg(clientsocket[index],(char *)&pl[index],sizeof(pl[index]));
				///printf("	RON send end\n");
				
				char message[20];
				int error = recv(clientsocket[index],message,20,0);
				if(error < 0){
					perror("recv error");
					//break;
				}
				//printf("	RON recv end\n");
				//printf("	message: %s\n", message);
				if(strncmp(message,"Y",1) == 0){//胡牌
					for(int i=0;i<4;i++) ////
					{
						if(index != i){
							winner(&pl[i], index, clientsocket[i], 1);
						}
						else{
							winner(&pl[i], index, clientsocket[i], 2);
							
						}
						printf("Player %d WIN!!\n", index);
						
						
					}
					
					return 1;
				}
			}
		}
	}
	//碰牌
	for(int i=0;i<4;i++) ////
	{
		if(index != i){
			if(CHECK_PONG(&pl[i], pl[index].discard, clientsocket[i]) == 1){
				*p=i;
				//printf("*p: %d pl[index].discard: %d\n", *p, pl[index].discard);
				Ask_player_discard(*p, &pl[*p], clientsocket[*p], 2);
				
					pl[0].discard=pl[*p].HandNow;
					pl[1].discard=pl[*p].HandNow;
					pl[2].discard=pl[*p].HandNow;
					pl[3].discard=pl[*p].HandNow;
				
				pl[index].HandNow = -1;
				
				
				if(CHECK_ANY(pl, p, clientsocket) == 1){
					return 1;
				}
				
				return 0;
			}
			
		
		}
	}
	//吃牌
	int a=(index==3)?-3:1;
	if(CHECK_CHI(&pl[index+a], pl[index].discard, clientsocket[index+a])){
		*p=index+a;
		//printf("*p: %d pl[index].discard: %d\n", *p, pl[index].discard);
		Ask_player_discard(*p, &pl[*p], clientsocket[*p], 2);
		
			pl[0].discard=pl[*p].HandNow;
			pl[1].discard=pl[*p].HandNow;
			pl[2].discard=pl[*p].HandNow;
			pl[3].discard=pl[*p].HandNow;
		
		pl[index].HandNow = -1;
		
		
		if(CHECK_ANY(pl, p, clientsocket) == 1){
			return 1;
		}
	}
	
	//printf("pl[index].discard: %d\n", pl[index].discard);
	return 0;

}

int game_play(struct player *pl, int *p,int clientsocket[]){
	printf("------------------------------------------------\n");
	int index=(*p)%4;
	//perror("	game_play error");
	//printf("\n\n\n");
	char dis_card[20];
	char buf[20];
	//摸牌
	int n = rand() % CARD_MAX;
	while(card_names_check[n]){
		n = rand() % CARD_MAX;////
		
	}
	pl[index].HandNow = n;
	card_names_check[n] = true;
	if(CHECK_RON(&pl[index], pl[index].HandNow) == 1){
		printf("	CHECK_RON (1) == 1\n");
		//Ask ron?
		pl[index].sign = RON;
		sprintf(buf, "RON ?(Y/N)");
		
		strcpy(pl[index].msg, buf);
		send_msg(clientsocket[index],(char *)&pl[index],sizeof(pl[index]));
		printf("	RON send end\n");
		
		char message[20];
		int error = recv(clientsocket[index],message,20,0);
		if(error < 0){
			perror("recv error");
			//break;
		}
		printf("	RON recv end\n");
		printf("	message: %s\n", message);
		if(strncmp(message,"Y",1) == 0){//胡牌
			for(int i=0;i<4;i++) ////
			{
				if(index != i){
					winner(&pl[i], index, clientsocket[i], 1);
				}
				else{
					winner(&pl[i], index, clientsocket[i], 2);
				
				}
				printf("Player WIN: %d\n", index);
				
				
			}
			
			return 1;
		}
	}
	//printf("game_play CHECK_KONG start: %d\n", index);
	if(CHECK_KONG(&pl[index], pl[index].HandNow, clientsocket[index]) == 1){
		//printf("CHECK_KONG == 1\n");
	}
	else{
		Ask_player_discard(index, &pl[index], clientsocket[index], 1);
	
	}
	//printf("game_play CHECK_KONG end\n");
		
		pl[0].discard=pl[index].HandNow;
		pl[1].discard=pl[index].HandNow;
		pl[2].discard=pl[index].HandNow;
		pl[3].discard=pl[index].HandNow;
	
	pl[index].HandNow = -1;
	
	
	if(CHECK_ANY(pl, p, clientsocket) == 1){
		
		return 1;
	}
	
	return 0;
}

int main()
{
	int serverSocket;
	struct sockaddr_in server_addr;
	struct sockaddr_in clientAddr;
	int addr_len = sizeof(clientAddr);
	printf("1\n");
	if((serverSocket = socket(AF_INET,SOCK_STREAM,0)) < 0)
	{
		perror( "error: create server socket");
		exit(1);
	}
	printf("2\n");
	bzero(&server_addr,sizeof(server_addr));
	server_addr.sin_family =AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if(bind(serverSocket,(struct sockaddr *)&server_addr,sizeof(server_addr)) < 0)
	{
		perror("error: bind address");
		exit(1);
	}
	printf("3\n");
	if(listen(serverSocket,5)<0)
	{
		perror("error: listen");
		exit(1);
	}
	printf("4\n");
	int clientsocket[4];
	for(int i=0;i<4;i++) //// 4people connect
	{
		clientsocket[i] = accept(serverSocket,(struct sockaddr *)&clientAddr,(socklen_t*)& addr_len);
		if(clientsocket[i] < 0)
		{
			perror("error: accept client socket");
			continue;
		}
		else{
			char buf[100];
			printf("clientsocket[%d]\n", i);
		}
		
	}
	
	printf("ready\n");
	
	for(int i=0;i<4;i++) //// 4people connect
	{
		char buf[100];
		sprintf(buf,"%s","Game Start!!");
		printf("send\n");
		send_msg(clientsocket[i],buf,sizeof(buf));
	}
	sleep(1);
	struct player pl[4];
	
	for(int i=0;i<4;i++) ////  發牌 CARD_HAND
	{
		usleep(1);
		srand( time(NULL) );
		char buf[1024];
		//printf("send發牌\n");
		pl[i].order=i;
		pl[i].num_chi=0;
		pl[i].num_pong=0;
		pl[i].num_kong=0;
		pl[i].HandNow=-1;
		/* tesing data
		pl[0].Hand[0]=28;
		pl[1].Hand[0]=32;
		pl[1].Hand[1]=36;
		card_names_check[28] = true;
		card_names_check[32] = true;
		card_names_check[36] = true;*/
		
		
		for(int j=0;j<CARD_HAND;j++){
			/*if((i==0&&j==0)||(i==1&&j==0)||(i==1&&j==1)){
				printf("tesing data");
				continue;
			}*/
			int n = rand() % CARD_MAX;////
			while(card_names_check[n]){
				n = rand() % CARD_MAX;////
				
			
			}
			pl[i].Hand[j] = n;
			card_names_check[n] = true;
			
		}
		
		//printf("發牌end\n");
		pl[i].Hand_len=CARD_HAND;
		pl[i].sign = GIVE;
		qsort(pl[i].Hand, pl[i].Hand_len, sizeof(int), cmpfunc);
		
		perror("發牌 error");
		
		send_msg(clientsocket[i],(char *)&pl[i],sizeof(pl[i]));
	}
	
	int play=0;
	int Mahjongcard[4][13];
	char recvbuf[1024];
	int c=0;
	while(1) ////game start
	{
		if(c==136)
			break;
		//printf("yaaaaaaaa!!!!\n");
		//printf("------------------------------------------------\n");
		sleep(2);
		
		pl[0].order_now=play%4;
		pl[1].order_now=play%4;
		pl[2].order_now=play%4;
		pl[3].order_now=play%4;
		
		//printf("play: %d\n", play);
		if(play%4 == 0){//東、
			
			//printf("\n\n\n");
			if(game_play(pl, &play, clientsocket) == 1){
				break;
			}
			
			
			
			
		}
		else if(play%4 == 1){//南、
		
			if(game_play(pl, &play, clientsocket) == 1){
				break;
			}
			
		}
		else if(play%4 == 2){//西、
		
			if(game_play(pl, &play, clientsocket) == 1){
				break;
			}
		}
		else if(play%4 == 3){//北
		
			if(game_play(pl, &play, clientsocket) == 1){
				break;
			}
			
		}
		
		Desk[play%4][Player_desk[play%4]] = pl[play%4].discard;
			
		printf("	player: %d Player_desk: %s\n", play%4,
			get_card_name(Desk[play%4][Player_desk[play%4]]));
		
		Player_desk[play%4]++;
		play++;
		c++;
	}
	for(int i=0;i<4;i++) ////GAMEOVER
	{
		usleep(1);
		
		pl[i].sign = GAMEOVER;
		
		send_msg(clientsocket[i],(char *)&pl[i],sizeof(pl[i]));
	}
	pclose(serverSocket);
}
	
