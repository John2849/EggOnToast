





bool addToAcepile(const t_card cardToAdd,const int playerIndex,const int suitIndex);

void initMessaging(const bool create);
void initAcepile();

/*******************  MESSAGING COMMON CODE ********************************/


t_serverToPlayer fromServer;
t_playerToServer toServer;
int msqid;

bool initMessaging(const bool bCreate)
{
	bReturn = false;
	if ( bCreate )
	{
		msqid = msgget( ftok("/tmp", 'B'+ee)) , 0664 | IPC_CREAT );
		if ( msqid == -1 )
		{
			perror(" message init/create error ");
			exit(2);
		}
		else bReturn = true;
	}
	else
	{
		msqid = msgget( ftok("/tmp", 'B'+ee)) , 0664  );
		if ( msqid == -1 )
		{
			perror(" message int error ");
			exit(2);
		}
		else bReturn = true;
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
	fromServer.numSeats = SEATS;
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
	
	return bAccepted;
}
	
	
