/**********************************
 * FILE NAME: Params.cpp
 *
 * DESCRIPTION: Definition of Parameter class
 **********************************/

#include "Params.h"

/**
 * Constructor
 */
Params::Params(): PORTNUM(8001) {}

/**
 * FUNCTION NAME: setparams
 *
 * DESCRIPTION: Set the parameters for this test case
 */
void Params::setparams(char *config_file) {
	//trace.funcEntry("Params::setparams");
	char CRUD[10];
	FILE *fp = fopen(config_file,"r");

	fscanf(fp,"NODES: %d", &MAX_NUM_NEIGHBOURS);
	fscanf(fp,"\nCRUD_TEST: %s", CRUD);

	if (0 == strcmp(CRUD, "CREATE"))
	{
		this->testType = CREATE_TEST;
	}
	else if (0 == strcmp(CRUD, "READ"))
	{
		this->testType = READ_TEST;
	}
	else if (0 == strcmp(CRUD, "UPDATE"))
	{
		this->testType = UPDATE_TEST;
	}
	else if (0 == strcmp(CRUD, "DELETE"))
	{
		this->testType = DELETE_TEST;
	}

  NUM_PEERS = MAX_NUM_NEIGHBOURS;
	STEP_RATE=.25;
	MAX_MSG_SIZE = 4000;
	globaltime = 0;
	allNodesJoined = 0;
	for (unsigned int i = 0; i < NUM_PEERS; i++)
	{
		allNodesJoined += i;
	}
	fclose(fp);
	//trace.funcExit("Params::setparams", SUCCESS);
	return;
}

/**
 * FUNCTION NAME: getcurrtime
 *
 * DESCRIPTION: Return time since start of program, in time units.
 * 				For a 'real' implementation, this return time would be the UTC time.
 */
int Params::getcurrtime() const {
    return globaltime;
}
