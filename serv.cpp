#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>

// common elements - eventually may be separate *.o to link
#include "play.h"
#include "commonCode.cpp"


int  numberOfPlayingClients = 0;
int  numberOfWaitingClients = 0;
bool clientResponded[SEATS];
int  priority = 10;



bool handleJoin();
void handleClient(int activeID);
bool  playing = true;
int   totalDeadlockCounter = 0;


int Client[SEATS];

		

int main(void)
{
	int seatID;
	int waitcount = 0;
	
	

	initAcepile();
	numberOfPlayingClients = 0;
	numberOfWaitingClients = 0;


	msqid = initMessaging(true);
	
	
	if ( msqid == -1 )
	{
		perror(" msgget error ");
		exit(2);
	}


	for(seatID=0;seatID<SEATS;seatID++) 
	{
		Client[seatID] = false;
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
		for(seatID=0;(Client[seatID])&&(seatID<SEATS);seatID++) {}
		if ( seatID == SEATS ) { 
			perror(" seat accounting error "); return(1);
		}
		// prime message queue with echo message 
		// so that we will not be blocked
		toServer.pl = echo;

		if ( bNonEmptyAdminQ == false ) 
		{
			sendMsg(SERVER_JOIN);
		}
		bNonEmptyAdminQ = true;
		do
		{
			recMsg(SERVER_JOIN);
			
			switch(toServer.pl)
			{
				case join:				
				    sendMsg(CLIENT_JOIN);
					Client[seatID] = true;
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

/********************************************************************************************/
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
			
	
}

