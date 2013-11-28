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
int  numberOfDeadlockedClients = 0;
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
		numberOfDeadlockedClients = 0;
		for(seatID=0;seatID<SEATS;seatID++) 
		{
			// Round-robin through all clients
			if ( Client[seatID] )
			{
				handleClient( seatID );
			}
		}
	}
	
	return 0;
}

// return true if there was some activity 
bool handleJoin()
{
	int dummyID;
	int seatID;
	bool bReturn = false;
	if ( numberOfPlayingClients < SEATS )
	{
		// find first empty seat
		for(seatID=0;(Client[seatID])&&(seatID<SEATS);seatID++) {}
		if ( seatID == SEATS ) { 
			perror(" seat accounting error "); return(1);
		}
		while ( ( numberOfPlayingClients < SEATS ) &&
		       ( recMsg(SERVER_JOIN, (char *)&dummyID , sizeof(dummyID) ,  IPC_NOWAIT ) ) )
		{
		    sendMsg(CLIENT_JOIN , (char *) &seatID , sizeof(seatID) );
			Client[seatID] = true;
			numberOfPlayingClients++;
			bReturn = true;
		}
	}
	return bReturn;
}
					
		

void handleClient(int seatID )
{
	recMsg( SERVID(seatID) , (char *) &toServer , sizeof(toServer) ,  0 );	
	switch( toServer.pl )
	{
		case newPlay:
		fromServer.accepted = addToAcepile(toServer.cardToSend,toServer.player);
		if ( fromServer.accepted )
		{
			fromServer.changeID++;
		}
		break;

		case deadlock:
		numberOfDeadlockedClients++;
		break;
			
		case quit:
		Client[seatID] = false;
		break;

		default:
		break;
			
	}
	
	if ( Client[seatID] ) 
	{
		sendMsg( CLIENTID(seatID) , (char *) &fromServer , sizeof(fromServer) );
	}
		
	
}

