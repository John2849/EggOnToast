#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>

// common elements - eventually may be separate *.o to link
#include "../play/play.h"
#include "../common/commonCode.cpp"


int  numberOfPlayingClients = 0;
int  numberOfWaitingClients = 0;
bool clientResponded[SEATS];
int  priority = 10;



bool handleJoin();
void handleClient();
bool  playing = true;
int   totalDeadlockCounter = 0;


int Client[SEATS];

		




char recBuffer[MSGSIZE_MAX];

int main(void)
{
	int seatID;
	int waitcount = 0;
	
	

	initAcepile();
	numberOfPlayingClients = 0;
	numberOfWaitingClients = 0;


	msqid = msgget( ftok("/tmp", 'B'+ee)) , 0664 | IPC_CREAT );
	
	
	if ( msqid == -1 )
	{
		perror(" msgget error ");
		exit(2);
	}


	for(seatID=0;seatID<SEATS;seatID++) 
	{
		Client[ee] = -1;
	}
	
	
	playing = true;
	while(playing) // do server loop
	{
		if ( numberOfPlayingClients == 0 ) 
		{
			sleep(5);
			printf(" ... Try waiting - %d \n",waitcount++);
		}
		else
		{
			printf(" %d players \n",numberOfPlayingClients);
			sleep(1);
		}
		handleJoin(); //  clients joining and quitting
		for(seatID=0;seatID<SEATS;seatID++) 
		{
			// Round-robin through all clients
			if ( Client[seatID] != -1)
			{
				handleClient( Client[seatID] );
			}
		}
	}
	
	return 0;
}

// return true if there was some activity 
bool handleJoin()
{
	int seatID;
	bool bReturn = false;
	static bool bNonEmptyAdminQ =  false;
	if ( numberOfPlayingClients < SEATS )
	{
		// find first empty seat
		for(seatID=0;(client[seatID])&&(seatID<SEATS);seatID++) {}
		if ( seatID == SEATS ) { 
			perror(" seat accounting error "); return(1);
		}
		// prime message queue with echo message 
		// so that we will not be blocked
		toSender.pl = echo;
		if ( bNonEmptyAdminQ == false ) sendSender(SERVER_ADMIN_ID);
		bNonEmptyAdminQ = true;
		do
		{
			recSender(SERVER_ADMIN_ID);
			switch(toSender.pl)
			{
				case join:				
				    playerID++;
					toPlayer.clientID = seatID;
					client[seatID] = true;
					sendPlayer(CLIENT_ADMIN_ID);
					numberOfPlayingClients++;
					bReturn = true;
					break;
				case echo:
					bNonEmptyAdminQ = false;
					break;
				default:
					break;
			}
		} while( ( numberOfPlayingClients<SEATS ) && ( bNonEmptyAdminQ ) );
	}
	return bReturn;
}
					
		

void handleClient(int activeID)
{
	static int activeID = -1;
	unsigned int dummyPriority  = 0;

	if ( numberOfPlayingClients > 0 )
	{
		// Round-robin to next active client
		activeID++;
		if ( activeID >= SEATS ) activeID = 0;
		while( EMPTY_MQD == mqdFromClient[activeID] ) 
		{
			activeID++;
			if ( activeID >= SEATS ) activeID = 0;
		}
		
		// Wait/get response from the active client
		mq_receive( mqdFromClient[activeID] , (char *) &toServer , sizeof(toServer) , &dummyPriority );
/*********************************************************************************************/
/*********************************************************************************************/
//                                   
//                            PROCESS WAIT    
//
/*********************************************************************************************/
/*********************************************************************************************/
		
		switch( toServer.pl )
		{
			case newPlay:
			fromServer.accepted = addToAcepile(toServer.cardToSend,toServer.player);
			if ( fromServer.accepted )
			{
				fromServer.changeID++;
			}
			totalDeadlockCounter = 0;
			break;
			
			case idle:
			totalDeadlockCounter = 0;
			break;
			
			case quit:
			break;

			
			default:
			break;
			
		}
			
		if ( toServer.pl == quit )
		{
			// If client just quit, do not talk to them anymore
			// and remove them from the active list. 
			mq_close( mqdFromClient[activeID] );
			mq_close( mqdToClient[activeID] );
			mq_unlink( fromClient(activeID) );
			mq_unlink( toClient(activeID) );
			numberOfPlayingClients--;
		}
		else
		{
			// If client is still active then respond to them to unblock them
			mq_send( mqdToClient[activeID] , (char *) &fromServer , sizeof(fromServer) , priority );
					
		}
	}
}

