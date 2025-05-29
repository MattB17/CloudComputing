/**********************************
 * FILE NAME: Application.h
 *
 * DESCRIPTION: Header file of all classes pertaining to the Application Layer
 **********************************/

#ifndef _APPLICATION_H_
#define _APPLICATION_H_

#include "stdincludes.h"
#include "Config.h"
#include "Address.h"
#include "MP1Node.h"
#include "Log.h"
#include "Params.h"
#include "Member.h"
#include "EmulNet.h"
#include "Queue.h"
#include "MP2Node.h"
#include "Node.h"

/**
 * CLASS NAME: Application
 *
 * DESCRIPTION: Application layer of the distributed system
 */
class Application{
private:
  static size_t nodeCount;
	static const char alphanum[];

	// Address for introduction to the group
	// Coordinator Node
	char JOINADDR[30];
	std::shared_ptr<EmulNet> en;
	std::shared_ptr<EmulNet> en1;
  std::shared_ptr<Log> log;
	std::vector<std::unique_ptr<MP1Node>> mp1;
	std::vector<std::unique_ptr<MP2Node>> mp2;
	std::shared_ptr<Params> par;
	std::map<string, string> testKVPairs;
public:
	Application(char* inputFile, bool debugMode);
	virtual ~Application();
	Address getjoinaddr();
	void initTestKVPairs();
	int run();
	void mp1Run();
	void mp2Run();
	void insertTestKVPairs();
	int findARandomNodeThatIsAlive();
	void deleteTest();
	void readTest();
	void updateTest();
};

#endif /* _APPLICATION_H__ */
