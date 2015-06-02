#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>

#define DEBUG

#ifdef  DEBUG
 #define PRINT(log, args...)	do{fprintf(stdout, log, ##args);}while(0)
#else    
 #define PRINT(log, args...)
#endif

#define SERV_PORT 			6000
#define PLAYER_NAME 		"Jovec"
#define MAXLINE				2048
#define MAXOFFSET			2048
#define PARSER_MAXARGS	200
#define MAX_PNAME_SIZE		20

#define K_UNKNOWN		0x00
#define K_seat			0x01
#define K_seat_e			0x81
#define K_gameover		0x02
#define K_blind			0x03
#define K_blind_e		0x83
#define K_hold			0x04
#define K_hold_e			0x84
#define K_inquire			0x05
#define K_inquire_e		0x85
#define K_flop			0x06
#define K_flop_e			0x86
#define K_turn			0x07
#define K_turn_e			0x87
#define K_river			0x08
#define K_river_e			0x88

char buffer[MAXLINE * 2];
pthread_t read_pid;
int client_fd;
int my_pid;;

typedef struct
{
	int pid;
	char pname[MAX_PNAME_SIZE + 1];
	int jetton;
	int money;
	int button;
	char action[20];
	int bet;
}Player_t;

typedef struct
{
	char color[20];
	char point;
}Card_t;

typedef struct
{
	Player_t *member;
	Card_t piv_cards[2];
	Card_t pub_cards[5];
	int total_pot;
	int count;
	int out_count;
	char roll[10];
	int act;
}Players_t;

Players_t players;

void error_quit(const char *str)    
{    
	fprintf(stderr, "%s", str); 
	if( errno != 0 )
		fprintf(stderr, " : %s", strerror(errno));
	printf("\n");
	exit(1);    
}

//pname		jetton		money		button		action		bet
void PRINT_HEAD()
{
	PRINT("%-20s%-22s%-20s%-10s%-10s%-10s\n", 
		"pname", "jetton", "money", "button", "action", "bet");
}

void do_action(int fd)
{
	int num;
	char ch;
	char act_str[20];
	int i;

	for(i = 0; i < players.count; i++)
	{
		//players.member;
	}
	
	printf("1.check | 2.call | 3.raise num | 4.all_in | 5.fold\n");
	//check | call | raise num | all_in | fold eol

	printf("Action: \33[K");
	fflush(stdout);
	
	//scanf("%[^\n]", act_str);
	fgets(act_str, sizeof(act_str), stdin);
	ch = act_str[0];
	//PRINT("input:%s\n", act_str);


	if(ch == '1')
	{
		strcpy(buffer, "check \n");
		PRINT("Send> %s", buffer);
	}
	else if(ch == '2')
	{
		strcpy(buffer, "call \n");
		PRINT("Send> %s", buffer);
	}
	else if(ch == '3')
	{
		sscanf(act_str, "3 %d\n", &num);
		sprintf(buffer, "raise %d \n", num);
		PRINT("Send> %s", buffer);
	}
	else if(ch == '4')
	{
		strcpy(buffer, "all_in \n");
		PRINT("Send> %s", buffer);
	}
	else if(ch == '5')
	{
		strcpy(buffer, "fold \n");
		PRINT("Send> %s", buffer);
	}

	write(fd, buffer, strlen(buffer) + 1);
	
	fflush(stdout);
}

void update_screen()
{
	int i = 0;

	//printf("\033[?25l\033[0;0H");
	
	//system("clear");
	PRINT_HEAD();
	
	for(i = 0; i < players.count; i++)
	{
		printf("%-20d%-22d%-20d%-10c%-10s%-10d\n", 
												players.member[i].pid,
												players.member[i].jetton,
												players.member[i].money,
												players.member[i].button == 1 ? '*': ' ',
												players.member[i].action,
												players.member[i].bet);
	}

	printf("Total pot: %d\t\tRoll: %s\33[K\n", players.total_pot, players.roll);

	printf("Private cards: \33[K\n");
	printf("\33[K%s-%c\t%s-%c\n",
		players.piv_cards[0].color,
		players.piv_cards[0].point,
		players.piv_cards[1].color,
		players.piv_cards[1].point);
	
	printf("Public cards:\33[K\n");
	printf("\33[K%s-%c\t%s-%c\t%s-%c\t%s-%c\t%s-%c\n",
		players.pub_cards[0].color,
		players.pub_cards[0].point,
		players.pub_cards[1].color,
		players.pub_cards[1].point,
		players.pub_cards[2].color,
		players.pub_cards[2].point,
		players.pub_cards[3].color,
		players.pub_cards[3].point,
		players.pub_cards[4].color,
		players.pub_cards[4].point);

	if(players.out_count > 0)
	{
		printf("\033[%dB", players.out_count);
		for(i = 0; i < players.out_count; i++)
		{
			printf("\33[K\033[1A");
			fflush(stdout);
		}
	}

	if(players.act == 1)
	{
		do_action(client_fd);
	}
	
	/*printf("Action:\33[K");
	printf("\33[?25h");
	fflush(stdout);*/
}

int lookup_keyword(const char *s)
{
	if (!strcmp(s, "seat/ ")) return K_seat;
	if (!strcmp(s, "/seat ")) return K_seat_e;
	if (!strcmp(s, "game-over ")) return K_gameover;
	if (!strcmp(s, "blind/ ")) return K_blind;
	if (!strcmp(s, "/blind ")) return K_blind_e;
	if (!strcmp(s, "hold/ ")) return K_hold;
	if (!strcmp(s, "/hold ")) return K_hold_e;
	if (!strcmp(s, "inquire/ ")) return K_inquire;
	if (!strcmp(s, "/inquire ")) return K_inquire_e;
	if (!strcmp(s, "flop/ ")) return K_flop;
	if (!strcmp(s, "/flop ")) return K_flop_e;
	if (!strcmp(s, "turn/ ")) return K_turn;
	if (!strcmp(s, "/turn ")) return K_turn_e;	
	if (!strcmp(s, "river/ ")) return K_river;
	if (!strcmp(s, "/river ")) return K_river_e;	
  
	return K_UNKNOWN;
}

int do_msg(char *args[], int nargs)
{
	int kw;//, kw2;
	int i;
	
	kw = lookup_keyword(args[0]);
	//kw2 = lookup_keyword(args[nargs -1]);

	//PRINT("---------kw  = %d------------\n", kw);

	/*if((kw |0x80) != kw2)
		return -1;*/
	
	if(kw == K_seat)
	{
		PRINT("K_seat\n");
		/*
		seat/ eol
		button: pid jetton money eol
		small blind: pid jetton money eol
		(big blind: pid jetton money eol)0-1
		(pid jetton money eol)0-5
		/seat eol
		*/
		players.out_count = players.count - nargs + 2; 
		players.count = nargs - 2;
		if(players.member == NULL)
			players.member = (Player_t *)malloc(sizeof(Player_t) * players.count);		

		//memset(players.member, 0, sizeof(Player_t) * players.count);
		//memset(players.pub_cards[0].color, 0, sizeof(Player_t) * players.count);
		for(i = 0; i < 2; i++)
		{
			players.piv_cards[i].color[0] = 0;
			players.piv_cards[i].point = 0;
		}
		for(i = 0; i < 5; i++)
		{
			players.pub_cards[i].color[0] = 0;
			players.pub_cards[i].point = 0;
		}
		
		sscanf(args[1], "button: %d %d %d", 
						&players.member[0].pid, 
						&players.member[0].jetton, 
						&players.member[0].money);
		players.member[0].button = 1;
			
		sscanf(args[2], "small blind: %d %d %d", 
						&players.member[1].pid,
						&players.member[1].jetton, 
						&players.member[1].money);
		players.member[1].button = 2;
		
		if(nargs > 4 )
		{
			sscanf(args[3], "big blind: %d %d %d", 
						&players.member[2].pid, 
						&players.member[2].jetton, 
						&players.member[2].money);
			players.member[2].button = 3;
		}
		
		for(i = 4; i < nargs - 2; i++)
		{
			sscanf(args[i], "%d %d %d", 
								&players.member[i-1].pid, 
								&players.member[i-1].jetton, 
								&players.member[i-1].money);
			players.member[i-1].button = 0;
		}

		strcpy(players.roll, "seat");
		//update_screen();
		//sleep(2);
	}
	else if(kw == K_gameover)
	{
		PRINT("K_gameover\n");
		exit(1);
	}
	else if(kw == K_blind)
	{
		PRINT("K_blind\n");
		/*
		blind/ eol
		(pid: bet eol)1-2
		/blind eol
		*/
		int pid, bet, pid2 = -1, bet2 = -1;
		sscanf(args[1], "%d: %d", &pid, &bet);
		
		if(nargs == 4)
			sscanf(args[2], "%d: %d", &pid2, &bet2);
		for(i = 0; i < players.count; i++)
		{
			if(players.member[i].pid == pid)
			{
				players.member[i].bet = bet;
				//break;
			}
			else if(players.member[i].pid == pid2)
			{
				players.member[i].bet = bet2;
				//break;
			}
			players.member[i].bet = 0;
		}

		strcpy(players.roll, "blind");

		//update_screen();
		//sleep(2);
	}
	else if(kw == K_hold)
	{
		PRINT("K_hold\n");
		/*
		hold/ eol
		color point eol
		color point eol
		/hold eol
		*/
		sscanf(args[1], "%s %c", players.piv_cards[0].color, &players.piv_cards[0].point);
		sscanf(args[2], "%s %c",  players.piv_cards[1].color, &players.piv_cards[1].point);

		strcpy(players.roll, "hold");
	}
	else if(kw == K_inquire)
	{
		PRINT("K_inquire\n");
		/*
		inquire/ eol
		(pid jetton money bet blind | check | call | raise | all_in | fold eol)1-8
		total pot: num eol
		/inquire eol
		*/
		for(i = 0; i < nargs - 3; i++)
		{
			sscanf(args[i + 1], "%d %d %d %d %s", 
					&players.member[i].pid, 
					&players.member[i].jetton, 
					&players.member[i].money,
					&players.member[i].bet,
					players.member[i].action);
		}
		sscanf(args[i+1], "total pot: %d", &players.total_pot);
		players.act =1;
		
		/* check */
		strcpy(buffer, "check \n");
		//write(client_fd, buffer, strlen(buffer) + 1);
		do_action(client_fd);
		//return 1;
		//update_screen();
	}
	else if(kw == K_flop)
	{
		PRINT("K_flop\n");
		/*
		flop/ eol
		color point eol
		color point eol
		color point eol
		/flop eol
		*/
		sscanf(args[1], "%s %c", players.pub_cards[0].color, &players.pub_cards[0].point);
		sscanf(args[2], "%s %c",  players.pub_cards[1].color, &players.pub_cards[1].point);
		sscanf(args[3], "%s %c",  players.pub_cards[2].color, &players.pub_cards[2].point);
	}
	else if(kw == K_turn)
	{
		PRINT("K_turn\n");
		/*
		turn/ eol
		color point eol
		/turn eol
		*/
		sscanf(args[1], "%s %c", players.pub_cards[3].color, &players.pub_cards[3].point);
		strcpy(players.roll, "turn");
	}
	else if(kw == K_river)
	{
		PRINT("K_river\n");
		/*
		river/ eol
		color point eol
		/river eol
		*/
		sscanf(args[1], "%s %c", players.pub_cards[4].color, &players.pub_cards[4].point);
		strcpy(players.roll, "river");
	}
	//update_screen();
	//sleep(1);
	return 0;
}

int parse_msg(char *msg, int count)
{
	int i;
	int kw;
	int nargs = 0;	
	static int tag_start = 0;
	static int tag_end = 0;
	static char *args[PARSER_MAXARGS];
	static int fragment = 0;
	char *buffer = msg;

	//printf("fragment1 = %d\n", fragment);	
	
	msg -= fragment;
	count += fragment;
	args[0] = msg;
	for(i = 0; i < count; i++,msg++)
	{
		if(*msg == '\n')
		{
			*msg = '\0';
			nargs++;
			if(nargs >= PARSER_MAXARGS)
				return -1;

			args[nargs] = msg + 1; 
		}
		else if(*msg == 0)
		{
			args[nargs] = msg + 1;
		}
	}	

	msg--;
	if(*msg != '\0')
	{
		fragment = msg - args[nargs] + 1;
		//PRINT("fragment2 = %d\n", fragment);
		//PRINT("%s\n", args[nargs]);
		if(fragment > MAXOFFSET)
			return -1;
		memcpy(buffer - fragment, args[nargs], fragment);
	}
	else
	{
		fragment = 0;
	}
#if 1
	PRINT("nargs = %d\n", nargs);
	for(i = 0; i < nargs; i++)
		PRINT("%2d# %s\n", i, args[i]);
	sleep(1);
#endif

	for(i = 0; i < nargs; i++)
	{
		kw = lookup_keyword(args[i]);
		if(kw == K_UNKNOWN)
			continue;
			
		if(kw == K_gameover)
			exit(0);

		if(kw & 0x80)
		{
			tag_end = i;
			PRINT("\n------------tag_start = %d------------\n" \
			"%s\n------------tag_end = %d------------\n\n", tag_start, args[tag_start], tag_end);
			do_msg(args + tag_start, tag_end - tag_start + 1);
		}
		else
		{
			tag_start = i;			
		}
	}
	return 0;
}

void read_thread(int fd)
{
	char buffer[MAXLINE + MAXOFFSET];
	char *buffer2 = buffer + MAXOFFSET;
	int res;
	
	do
	{
		res = read(fd, buffer2, MAXLINE);
		//printf("res = %d\n", res);
		if( res < 0 )
			error_quit("read error");
		if(res > 0)
		{
			res = parse_msg(buffer2, res);
			//if(res == 1)
				//do_action(fd);
		}
	}while(1);
}

int main(int argc, char **argv)     
{          
	int res; 
	socklen_t len;
	int server_port;

	if( argc != 6 )
	{
		printf("Usage: %s server_ip server_port client_ip client_port play_id\n", argv[0]);
		return -1;
	}

	/* create client fd */
	client_fd = socket(AF_INET, SOCK_STREAM, 0); 
	int on = 1;
	res = setsockopt(client_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	if(res < 0)
		error_quit("setsockopt error");
	
	struct sockaddr_in client_addr;
	int client_port;
	memset(&client_addr, 0, sizeof(client_addr));      
	client_addr.sin_family = AF_INET;      
	client_addr.sin_addr.s_addr = htonl(INADDR_ANY);  
	sscanf(argv[4], "%d", &client_port);    
	client_addr.sin_port = htons(client_port);      
	inet_pton(AF_INET, argv[3], &client_addr.sin_addr);
	
	res = bind(client_fd, (struct sockaddr *)&client_addr, sizeof(struct sockaddr)); 
	if(res < 0)
	{
		error_quit("bind error");
	}

	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));      
	server_addr.sin_family = AF_INET;      
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);  
	sscanf(argv[2], "%d", &server_port);    
	server_addr.sin_port = htons(server_port);      
	inet_pton(AF_INET, argv[1], &server_addr.sin_addr);

	int error_count = 0;
	do
	{
		res = connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)); 
		usleep(50000);
	}while(++error_count < 10 && res < 0);
	if( res < 0 )
			error_quit("connect error");

	/* register
	 * reg: pid pname need_notify eol
	 */
	//sprintf(buffer, "reg: %s %s need_notify \n", argv[5], PLAYER_NAME);
	my_pid = atoi(argv[5]);
	sprintf(buffer, "reg: %d %s \n", my_pid, PLAYER_NAME);
	res = write(client_fd, buffer, strlen(buffer) + 1);
	if(res < 0)
		error_quit("write");	
	
	system("clear");
	//PRINT_HEAD();
	 //printf("\033[s");
	
	res = pthread_create(&read_pid, NULL, (void *)read_thread, (void *)client_fd);
	if(res < 0)
		error_quit("pthread_create");
	
	pthread_join(read_pid, NULL);	
	
	close(client_fd);
	
	return 0;
}
