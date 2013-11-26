//============================================================================
// Name        : play.cpp
// Author      : John Miller
// Version     : 1.0.0
// Copyright   : -none-
// Description : solitare playing client 
//============================================================================

#include <iostream>
using namespace std;

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>

#include "play.h"
#include "../common/commonCode.cpp"




typedef struct
{
  t_card hidden[PILES][PILES];
  t_card shown[PILES][VALUES];
  t_card hand[DECKCOUNT];
  t_card undealt[(DECKCOUNT)+1];
  int undealtCount;
  int handCount;
  int handSelected;
  int remaining;
}t_localState;

t_card deal(void);
void perror(const char *errString);
int randomNumber(void);
void init();
void printCard(t_card card);
void printLocalState();

t_card currentHand();
t_card nextHand();
t_card useHand();
bool flipHidden(int pile);



bool hToA();
bool pToA();
bool hToP();
bool pToP();

bool aceMatch(t_card cardToMatch);

t_localState l;
int deadlockCount = 0;






char v[VALUES+1+1] = {"-A23456789TJQK"};
char s[SUITS+1] =  {"HDCS"};

typedef int t_rnsize;

FILE *fp;


int msqid_unconfigured;
int msqid_server;

bool useServer = false;
int myID = 0;
bool respondedToServer = false;

mqd_t mqdToClient = EMPTY_MQD;
mqd_t mqdFromClient = EMPTY_MQD;

unsigned int priority = 10;
unsigned int readPriority =0;
char recBuffer  [MSGSIZE_MAX];



bool getFromServer();
bool sendToServer();	


bool getFromServer()
{
	int bytesRead = 0;
	bool bReturn = false;
	unsigned int readPriority = 0;


	bytesRead = mq_receive( mqdToClient , recBuffer , sizeof(recBuffer) , &readPriority );
	

	// See that we got the complete message 	
	if ( bytesRead == sizeof(fromServer) ) // we got it all
	{
		bReturn = true;
		memcpy( &fromServer , recBuffer , sizeof(fromServer) );
	}
	else if ( bytesRead >= 0 ) 
	{ 
		perror("Incomplete transfer "); 
	}
	else if ( bytesRead == -1 )
	{
		if ( errno != ENOMSG ) { perror("unexpected errorcode for msgrcv "); }
	}	

	return bReturn;
}



bool sendToServer()
{
	int bytesSent = 0;
	bool bReturn = false;
	priority = 10;
	
    // compose message to send

	bytesSent = mq_send( mqdFromClient , (char *) &toServer , sizeof(toServer) , priority );
	if ( bytesSent == 0 )
	{
		bReturn = true;
	}
	else { perror(" message sending error \n"); exit(1); }
	
	return bReturn;

}



int main(int argc,char *argv[])
{
	mqd_t servReq_mqd =  EMPTY_MQD;
	mqd_t servAssign_mqd = EMPTY_MQD;
	int bytesRec = -22;
	
	
	printf("starting \n");
	

	// See if we can use remote server
	

	int changeID = 0;
	
	useServer = false;
	servReq_mqd = mq_open( MQ_REQNAME , O_RDWR  );

	if ( servReq_mqd != EMPTY_MQD )
	{
		printf(" Server was found \n");
		useServer = true; // we found the server
		// we expect that the assignment queue will be there too...
		servAssign_mqd = mq_open( MQ_ASSIGN_NAME , O_RDWR );
		if ( servAssign_mqd == EMPTY_MQD )
		{
			perror(" bad server configuration - req queue without assignment queue ");
			exit(1);
		}
		// send out notify
		memcpy(recBuffer,MQ_REQTEXT,sizeof(MQ_REQTEXT)+1);
		// send out message -priority does not matter
		mq_send( servReq_mqd , recBuffer , sizeof(MQ_REQTEXT) , priority );
		// wait for response 
		bytesRec = mq_receive(servAssign_mqd , recBuffer , sizeof(recBuffer) , &readPriority);
		if ( bytesRec == sizeof(myID) )
		{
			memcpy( &myID , recBuffer, sizeof(myID) );
		}
		else
		{
			perror(" Bad assignment resonse " );
			exit(1);
		}
		// note: need large buffer with larger buffer size 
		printf(" ASERVER AREPONDED %d:%s \n ",bytesRec,recBuffer
		);
	
		// now get our queues
		mqdToClient = mq_open( toClient(myID) , O_RDWR );
		if ( mqdToClient == EMPTY_MQD )
		{
			perror(" cannot open toClient name");
			exit(1);
		}
		mqdFromClient = mq_open( fromClient(myID) , O_RDWR );
		if ( mqdFromClient == EMPTY_MQD )
		{
			perror(" cannot open fromClient name");
			exit(1);
		}
		 
			
	}
		
		


      

	fp = fopen("/dev/urandom","rb");
	


   do
   {
	printf(" ******************* START NEW GAME ******************** \n");

	init();
	if ( false == useServer )
	{
		fromServer.numSeats = 1;
		initAcepile(); 
	}
	else
	{
		toServer.cardToSend = EMPTY;
		toServer.pl = idle;
		sendToServer();
		getFromServer();
	}
	printLocalState();



	
	do
	{
		toServer.cardToSend = EMPTY;
		toServer.pl = idle;
		respondedToServer = false;
		
		if (useServer )
		{
			if ( changeID != fromServer.changeID ) 
			{	 
				deadlockCount = 0; 
				changeID = fromServer.changeID;
			}
		}
		if ( hToA() )  { deadlockCount=0; }
		else if ( pToA() ) { deadlockCount=0; }
		else if ( hToP() ) { deadlockCount=0; }
		else if ( pToP() ) { deadlockCount=0; }
		else nextHand();
		
		if ( deadlockCount > (l.handCount+1) ) toServer.pl = deadlock;
		deadlockCount++;
		
		if ( (useServer ) && (false == respondedToServer ) )
		{
			sendToServer();
			getFromServer();
			respondedToServer = true;
		}

		printLocalState();
		
		
	}while( ( ( l.remaining > 0 ) ) &&
	         ( useServer || ( toServer.pl!=deadlock) )
	         );
	         
	         
  }while( l.remaining > 0 );

  
	// Let server know that I am done
	if ( useServer )
	{
		toServer.pl=quit;
		msgsnd(msqid_server, &toServer,  sizeof(toServer), 0);
		mq_close(servReq_mqd);
		mq_close(servAssign_mqd);  
		mq_close(mqdFromClient);
		mq_close(mqdToClient);
	}

	
	return 0;
}



int randomNumber()
{
    int retVal = 0;
    fread( &retVal , sizeof(retVal) , 1 , fp );
	return retVal&0xFFFFFF;
}




bool hToA()
{
	bool bReturn = false;
	if ( l.handCount > 0 )
	{
	if ( aceMatch(currentHand()) )
	{
		useHand();
	}
	}
	return bReturn;
}

bool aceMatch( t_card cardToMatch)
{
	bool bMatched = false;
	bool bAcceptedFromServer = false;
	t_card aceCard;
	int suitToMatch = SUIT(cardToMatch);
	int i;
	int iFound;
	for ( i=0; (i<fromServer.numSeats) && !(bMatched) ;i++)
	{
		aceCard = fromServer.acePile[i][suitToMatch];
		// empty is value one less then ace so we can just use the value match below
		if ( VALUE(cardToMatch) == VALUE(aceCard)+1 )
		{
			    iFound = i;
				bMatched = true;
		}
		if ( bMatched ) // setup and send card
		{
			toServer.clientID = myID;
		    toServer.cardToSend = cardToMatch;
		    toServer.pl =         newPlay;
		    toServer.player =     iFound;  
		    if ( useServer )
		    {
				sendToServer();
				getFromServer();
/*********************************************************************************
***********************************************************************************
// 
//   Client waits for response from server 
//
***********************************************************************************
**********************************************************************************/
			}
			if ( ( fromServer.accepted ) || (!useServer) )
			{
				if (!useServer) // do it ourselves
				{
				   fromServer.acePile[iFound][suitToMatch] = cardToMatch;
			    }
				bAcceptedFromServer = true;
				l.remaining--;
			}
				
		}

	}
	
	return bAcceptedFromServer;
}

bool pToA()
{
	bool bReturn = false;
	int i, k;
	for (i = 0; (i < PILES) && !bReturn; i++)
	{
		if (l.shown[i][0] != EMPTY)
		{
			// find top card
			for (k = 0; l.shown[i][k + 1] != EMPTY; k++)
			{
			}
			// See if match
			if (aceMatch(l.shown[i][k]))
			{
				// clear it out
				l.shown[i][k] = EMPTY;
				bReturn = true;
				// flip hidden card if needed
				if (l.shown[i][0] == EMPTY )
				{
					flipHidden(i);
				}
			}
		}
	}
 return bReturn;
}

bool pToP()
{
	bool bReturn = false;
	int i,j,k,m;  // i == fixed(top)  j == moving(bottom)
	              // k == index to find top
	              // m == index to move card pile
	t_card fixedCard;
	t_card movingCard;
	int iFound,jFound;
	for(i=0;(i<PILES) && !bReturn ;i++)
	{
	// find fixed top card
	for(k=0;l.shown[i][k+1]!=EMPTY;k++) {}
 	fixedCard = l.shown[i][k];
	for(j=0;(j<PILES) && !bReturn ;j++)
	{
		movingCard = l.shown[j][0];
		if ( movingCard != EMPTY )
	    {
			if (i!=j)
			{
			    // is fixed empty
				if ( fixedCard == EMPTY )
				{
				   // only move if hidden cards underneath moving pile
				   // else we will be moving back and forth and not deadlock
				   if ( l.hidden[j][0] != EMPTY )
				   {
					   if ( VALUE(movingCard) == KING )
						   {
						     iFound = i;
						     jFound = j;
						     k=0;
						     bReturn = true;
						   }
				   }
				}
				else
				{
					// future smart enhancement - if no hidden card only move if king available
					// do value/color match if cards in fixed and moving
					if( ( VALUE(movingCard)+1 == VALUE(fixedCard) )
					     &&
					     ( COLOR(movingCard) != COLOR(fixedCard) ))
					     {
						     iFound = i;
						     jFound = j;
						     k++; // move to empty space
							 bReturn = true;
						 }
				 }
			 }
		 }
	 }
   }
   if ( bReturn == true ) // we have work to do
   {
	   m=0;
	   // move that pile
	   while( l.shown[jFound][m] != EMPTY )
	   {
		   l.shown[iFound][k+m] = l.shown[jFound][m];
		   l.shown[jFound][m] = EMPTY;
		   m++;
	   }
	   // flip hidden
	   flipHidden(jFound);
   }
   return bReturn;
}


bool hToP()
{
	bool bReturn = false;
	int i,k;  // i == fixed(top)  j == moving(bottom)
	              // k == index to find top
	              // m == index to move card pile
	t_card fixedCard;
	t_card movingCard;
	int iFound;
	if ( l.handCount > 0 )
	{
	movingCard = currentHand();
	for(i=0;(i<PILES) && !bReturn ;i++)
	{
		// find fixed top card
		for(k=0;l.shown[i][k+1]!=EMPTY;k++) {}
		fixedCard = l.shown[i][k];
	    // is fixed empty
		if ( fixedCard == EMPTY )
		{
		   if ( VALUE(movingCard) == KING )
			   {
			     iFound = i;
			     k=0;
			     bReturn = true;
			   }
		}
		else
		{
			if( ( VALUE(movingCard)+1 == VALUE(fixedCard) )
				     &&
				     ( COLOR(movingCard) != COLOR(fixedCard) ))
			     {
				   iFound = i;
				   k++;
				   bReturn = true;
				 }
		 }
	}
   if ( bReturn == true ) // we have work to do
   {
	   // move card from hand to pile
	   l.shown[iFound][k] = movingCard;
	   useHand();
   }
	}

	return bReturn;
}




bool flipHidden(int pile)
{
	bool bReturn = false;
	int j;
	if ( l.hidden[pile][0] != EMPTY )
	{
		// Flip to shown
		l.shown[pile][0] = l.hidden[pile][0];
		// shift hidden pile stack
		for(j=0;j<PILES-1;j++) l.hidden[pile][j] = l.hidden[pile][j+1];
		// indicate success
		bReturn = true;
	}
	return bReturn;
}




void printCard(t_card card)
{
	if ( (VALUE(card)<=KING ) 
	     &&
	(SUIT(card)<SUITS)
	    )
	{
		printf("%c%c ",v[VALUE(card)],s[SUIT(card)]);
	}
	else
	{
		
	  switch(card)
	  {
		  case EMPTY: printf("-- "); break;
	 	  case OPEN: printf("Op"); break;
		  default: printf("??"); break;
			break;
	  }
  }
}



void init()
{
  int i = 0;
  int j = 0;
  
  for(i=0;i<SEATS;i++)
  {
	  for(j=0;j<SUITS;j++)
	  {
		  fromServer.acePile[i][j] = MAKECARD(j,EMPTYVALUE);
	  }
  }
  
  for(i=0;i<DECKCOUNT;i++) l.undealt[i] = i;
  l.undealt[DECKCOUNT]=EMPTY;
  l.undealtCount = DECKCOUNT;
  
  
  // Initialize localstate
  for(i=0;i<PILES;i++)
  {
   for(j=0;j<PILES;j++) l.hidden[i][j] = EMPTY;
   for(j=0;j<VALUES;j++) l.shown[i][j] = EMPTY;
  }
  for(i=0;i<DECKCOUNT;i++) l.hand[i] = EMPTY;
  
  // initialize the deck
  l.undealtCount=0;
  for(i=0;i<VALUES;i++) l.undealt[l.undealtCount++] = MAKECARD(CLUBS,ACE+i);
  for(i=0;i<VALUES;i++) l.undealt[l.undealtCount++] = MAKECARD(SPADES,ACE+i);
  for(i=0;i<VALUES;i++) l.undealt[l.undealtCount++] = MAKECARD(HEARTS,ACE+i);
  for(i=0;i<VALUES;i++) l.undealt[l.undealtCount++] = MAKECARD(DIAMONDS,ACE+i);

  for(i=0;i<PILES;i++)
  {
      for(j=0;j<i;j++) l.hidden[i][j] = deal();
      l.shown[i][0] = deal();
  }
  i=0;
  while(  l.undealtCount > 0 )
  {
    l.hand[i++] = deal();
  }
  l.handCount = i;
  l.handSelected = HANDCOUNT-1;
  l.remaining= DECKCOUNT;

  return;
}




t_card deal()
{
  int newCardIndex;
  t_card newCard;
  int i;

  if ( l.undealtCount == 0 ) perror("NO MORE CARD TO DEAL");

  newCardIndex = randomNumber() % l.undealtCount;
  newCard = l.undealt[newCardIndex];
//  printf(" new card index = %d \n",newCardIndex );

  // now shift
  for(i= newCardIndex;i< l.undealtCount;i++) l.undealt[i]=l.undealt[i+1];
  l.undealtCount--;

 //printCard(newCard);

  return newCard;
}

t_card currentHand()
{
	return l.hand[l.handSelected];
}

bool atEnd()
{
	bool bReturn = false;
	if ( l.hand[l.handSelected+1] == EMPTY ) bReturn = true;
	return bReturn;
}

t_card nextHand()
{
	t_card cardReturn = EMPTY;
	int i;
	if ( currentHand() != EMPTY )
	{
		if ( atEnd() )
		{
			l.handSelected = 0;
		}
		for(i=0;(i<HANDCOUNT) && !atEnd();i++)
		{
			l.handSelected++;
		}
		cardReturn = currentHand();
	}
	return cardReturn;
}



t_card useHand()
{
  int i;
  t_card cardReturned = currentHand();
  if ( l.handCount > 0 )
  {
	  // shift and adjust count
	  for(i=l.handSelected;i<l.handCount;i++)
	  {
		  l.hand[i] = l.hand[i+1];
	  }
	  l.handCount--;
	  if ( l.handSelected == 0 )
	  {
 		for(i=1;(i<HANDCOUNT) && !atEnd();i++)
		{}
	  }
	  else
	  {
		  l.handSelected--;
	  }
  }
  return cardReturned;


}




void printLocalState()
{
	int i,j;
	printf("\n");
	for(i=0;i<PILES;i++)
	{
		for(j=0;j<PILES;j++) printCard( l.hidden[i][j] );
		printf("|");
		for(j=0;j<VALUES;j++) printCard( l.shown[i][j] );
		printf("\n");
	}
	printf("HAND:");
	for(i=0;i<l.handCount;i++) printCard( l.hand[i] );
	printf("\n SELECTED ");
	printCard(l.hand[l.handSelected]);
	printf("\n");
	printf("ACE:");
	for(i=0;i<SUITS;i++)
		{
		 printCard( fromServer.acePile[0][i]);
		}
	printf("\n");
	printf("Remaining count = %d \n",l.remaining);
	printf("\n\n");

}











