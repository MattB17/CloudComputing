/**********************************
 * FILE NAME: Params.h
 *
 * DESCRIPTION: Header file of Parameter class
 **********************************/

#ifndef _PARAMS_H_
#define _PARAMS_H_

#include "stdincludes.h"
#include "Params.h"
#include "Address.h"
#include "Member.h"

enum TestType
{
	CREATE_TEST,
	READ_TEST,
	UPDATE_TEST,
	DELETE_TEST
};

/**
 * CLASS NAME: Params
 *
 * DESCRIPTION: Params class describing the test cases
 */
class Params{
public:
	int MAX_NUM_NEIGHBOURS;                // max number of neighbors
	double STEP_RATE;		                   // dictates the rate of insertion
	int NUM_PEERS;			                   // actual number of peers
	int MAX_MSG_SIZE;
	int globaltime;
	int allNodesJoined;
	short PORTNUM;
	TestType testType;
	Params();
	void setparams(char *);
	int getcurrtime() const;
};

#endif /* _PARAMS_H_ */
