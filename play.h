// common interface 

#ifndef PLAY_H
#define PLAY_H

#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <mqueue.h>

typedef int t_card;


#define SEATS       (10)
#define SUITS        (4)

#define VALUE(card)    (card&0x0F)
#define SUIT(card)     ((card&0xF0)/0x10)
#define ISEMPTY(card)  (VALUE(card)==EMPTYVALUE) 

#define MAKECARD(suit,value)   (suit*0x10+value)

#define SERVER_JOIN  (1)
#define CLIENT_JOIN  (2)
#define SERVID(id)    (3+2*id)
#define CLIENTID(id)  (4+3*id)

#define EMPTYVALUE 0
#define ACE 1
#define TEN 10
#define JACK 11
#define QUEEN 12
#define KING 13
#define VALUES 13

#define CLUBS     0
#define SPADES    1
#define HEARTS    2
#define DIAMONDS  3
// color non-zero on red 
#define COLOR(card)    (SUIT(card)&0x2)

#define PILES 7
#define HANDCOUNT 3
#define DEADLOCK 20
#define DECKCOUNT (VALUES*SUITS)
#define EMPTY  (0xF1)
#define OPEN   (0xF2)

typedef enum 
{
	idle,
	join,
	quit,
	newPlay,
	deadlock,
	echo
}t_play;

typedef struct
{
	int clientID;
	t_play pl;
	t_card cardToSend;
	int player;
}t_playerToServer;

typedef struct
{
	long  mtype;
	char  mtext[sizeof(t_playerToServer)/sizeof(char)];
}t_msqPlayerToServer;

typedef struct
{
	int  clientID;
	int  changeID;
	bool  accepted;
	int  numSeats;
	t_card acePile[SEATS][SUITS];
}t_serverToPlayer;


typedef struct
{
	long  mtype;
	char  mtext[sizeof(t_serverToPlayer)/sizeof(char)];
}t_msqServerToPlayer;

#endif
