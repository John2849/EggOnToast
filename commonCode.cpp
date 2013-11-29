

#include <stdio.h>
#include <string.h>



bool addToAcepile(const t_card cardToAdd,const int playerIndex,const int suitIndex);

int initMessaging(const bool create);
void initAcepile();

/*******************  MESSAGING COMMON CODE ********************************/


t_serverToPlayer fromServer;
t_playerToServer toServer;

typedef struct 
{
	long mtype;
	char buf[1000];
}t_anymessage;

t_anymessage anyMessage;

	



int msqid;

int  initMessaging(const bool bCreate)
{
	int msqreturn = -1;
	key_t myTok;
	
	myTok = ftok("/tmp",'B'+1);
	
	if ( bCreate )
	{
		msqreturn = msgget( myTok , 0666 | IPC_CREAT );
		if ( msqreturn == -1 )
		{
			printf(" Try removing queue \n");
			
		}
		if ( msgctl( msqreturn , IPC_RMID , NULL ) == 0 )
		{
			printf(" Queue removed \n");
		}
		msqreturn = msgget( myTok , 0666 | IPC_CREAT );
		if ( msqreturn == -1 )
		{
			perror(" message creation error ");
			exit(1);
		}
			
		
	}
	else
	{
		msqreturn = msgget( myTok  , 0666  );
		if ( msqreturn == -1 )
		{
			perror(" message int error ");
			exit(2);
		}
	}
	printf(" new msqid == %d \n",msqreturn);
	return msqreturn;
}
	
	
bool sendMsg(long msgID,char *msg,int msgSize);
bool recMsg(long msgID, char *msg,int msgSize,int flag);	



bool sendMsg(long msgID,char *msg,int msgSize)
{
	bool bSuccess = false;
	anyMessage.mtype = msgID;
	memcpy( anyMessage.buf , msg , msgSize );
	if ( msgsnd( msqid , &anyMessage , msgSize , 0 ) != -1 )
	{
		bSuccess = true;
	}
	
	return bSuccess;
}


bool recMsg(long msgID, char *msg,int msgSize,int iFlag)
{
	bool bReturn = false;
	if ( msgrcv( msqid , &anyMessage , msgSize , msgID , iFlag ) != -1 )
	{
		printf("reeived message \n");
		memcpy( msg , &anyMessage.buf[0] , msgSize );
		bReturn = true;
	}
	return bReturn;
}
	     
	

		




/******************* CARD RELATED COMMON CODE ********************************/

void initAcepile()
{
	int iSeats;
	int iSuit;
	fromServer.changeID = 0;
	fromServer.clientID = -1;
	fromServer.accepted = false;

	for(iSeats=0;iSeats<SEATS;iSeats++)
	{
		for(iSuit=0;iSuit<SUITS;iSuit++)
		{
			fromServer.acePile[iSeats][iSuit] = MAKECARD(iSuit,EMPTYVALUE);
		}
	}
}


bool addToAcepile(const t_card cardToAdd,const int playerIndex)
{
	bool bAccepted = false;
	t_card cardAtAcepile = OPEN;

	int suitIndex = SUIT(cardToAdd);
	cardAtAcepile = fromServer.acePile[playerIndex][suitIndex];
	
	if ( VALUE(cardToAdd) == VALUE(cardAtAcepile)+1 )
	{
		bAccepted = true;
		fromServer.acePile[playerIndex][suitIndex] = cardToAdd;
	}
	else
	{
		printf(" ************************************** CARD NOT ACCEPTED \n");
		printf(" Client id = %d \n", toServer.clientID );
		printf(" PLAYER INDEX %d \n",playerIndex);
		printf(" CARD TO ADD %d  SUIT %d VALUE %d \n",cardToAdd , SUIT(cardToAdd) , VALUE(cardToAdd) );
		exit(1);
	}
	
	return bAccepted;
}
	
	
