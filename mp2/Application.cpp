/**********************************
 * FILE NAME: Application.cpp
 *
 * DESCRIPTION: Application layer class function definitions
 **********************************/

#include "Application.h"

void handler(int sig) {
	void *array[10];
	size_t size;

	// get void*'s for all entries on the stack
	size = backtrace(array, 10);

	// print out all the frames to stderr
	fprintf(stderr, "Error: signal %d:\n", sig);
	backtrace_symbols_fd(array, size, STDERR_FILENO);
	exit(1);
}

/**********************************
 * FUNCTION NAME: main
 *
 * DESCRIPTION: main function. Start from here
 **********************************/
int main(int argc, char *argv[]) {
  std::string usageString =
	  "Incorrect Usage. Correct form: ./Application <test_file> [--debug]";

	if (argc < 2 || argc > 4)
	{
		std::cout << usageString << std::endl;
		return FAILURE;
	}

	bool debugMode = false;
	if (argc == 3)
	{
		if (strcmp(argv[2], "--debug") == 0)
		{
			debugMode = true;
		}
		else
		{
			std::cout << usageString << std::endl;
			return FAILURE;
		}
	}

	// Create a new application object and run it.
	std::unique_ptr<Application> app = std::make_unique<Application>(
		argv[1], debugMode);
	app->run();

	return SUCCESS;
}

/**
 * Constructor of the Application class
 */
Application::Application(char *inputFile, bool debugMode) {
	int i;
	par = std::make_shared<Params>();
	srand (time(NULL));
	par->setparams(inputFile);
	log = std::make_shared<Log>(par, debugMode);
	en = std::make_shared<EmulNet>(par);
	en1 = std::make_shared<EmulNet>(par);
	mp1 = std::vector<std::unique_ptr<MP1Node>>(par->NUM_PEERS);
	mp2 = std::vector<std::unique_ptr<MP2Node>>(par->NUM_PEERS);

	/*
	 * Init all nodes
	 */
	for( i = 0; i < par->NUM_PEERS; i++ ) {
		std::shared_ptr<Member> memberNode = std::make_shared<Member>();
		memberNode->inited = false;
		Address addressOfMemberNode = en->ENinit();
		mp1[i] = std::make_unique<MP1Node>(
			memberNode, *par, en, log, addressOfMemberNode);
		mp2[i] = std::make_unique<MP2Node>(
			memberNode, *par, en1, log, addressOfMemberNode);
		log->LOG(&(mp1[i]->getMemberNode()->addr), "APP");
		log->LOG(&(mp2[i]->getMemberNode()->addr), "APP MP2");
	}
}

/**
 * Destructor
 */
Application::~Application() {}

/**
 * FUNCTION NAME: run
 *
 * DESCRIPTION: Main driver function of the Application layer
 */
int Application::run()
{
	int i;
	int timeWhenAllNodesHaveJoined = 0;
	// boolean indicating if all nodes have joined
	bool allNodesJoined = false;
	srand(time(NULL));

	// As time runs along
	for( par->globaltime = 0; par->globaltime < TOTAL_RUNNING_TIME; ++par->globaltime ) {
		// Run the membership protocol
		mp1Run();

		// Wait for all nodes to join
		if ( par->allNodesJoined == nodeCount && !allNodesJoined ) {
			timeWhenAllNodesHaveJoined = par->getcurrtime();
			allNodesJoined = true;
		}
		// We wait for a certain buffer period to allow all nodes to join.
		if ( par->getcurrtime() > timeWhenAllNodesHaveJoined + 50 ) {
			// Call the KV store functionalities
			mp2Run();
		}
	}

	// Clean up
	en->ENcleanup();
	en1->ENcleanup();

	for(i=0;i<=par->NUM_PEERS-1;i++) {
		 mp1[i]->finishUpThisNode();
	}

	return SUCCESS;
}

/**
 * FUNCTION NAME: mp1Run
 *
 * DESCRIPTION:	This function performs all the membership protocol functionalities
 */
void Application::mp1Run() {
	int i;

	// For all the nodes in the system
	for( i = 0; i <= par->NUM_PEERS-1; i++) {

		/*
		 * Receive messages from the network and queue them in the membership protocol queue
		 */
		if(par->getcurrtime() > (int)(par->STEP_RATE*i) &&
		   !(mp1[i]->getMemberNode()->failed) )
		{
			// Receive messages from the network and queue them
			mp1[i]->recvLoop();
		}
	}

	// For all the nodes in the system
	for(i = par->NUM_PEERS - 1; i >= 0; i--) {

		/*
		 * Introduce nodes into the distributed system
		 */
		if( par->getcurrtime() == (int)(par->STEP_RATE*i) ) {
			// introduce the ith node into the system at time STEPRATE*i
			mp1[i]->nodeStart(JOINADDR, par->PORTNUM);
			std::cout << i;
			std::cout << "-th introduced node is assigned with the address: ";
			std::cout << mp1[i]->getMemberNode()->addr.getAddress() << std::endl;
			nodeCount += i;
		}

		/*
		 * Handle all the messages in your queue and send heartbeats
		 */
		else if (par->getcurrtime() > (int)(par->STEP_RATE*i) &&
		         !(mp1[i]->getMemberNode()->failed))
		{
			// handle messages and send heartbeats
			mp1[i]->nodeLoop();
			if ((i == 0) && (par->globaltime % 500 == 0))
			{
				log->LOG(
					&mp1[i]->getMemberNode()->addr, "@@time=%d", par->getcurrtime());
			}
		}
	}
}

/**
 * FUNCTION NAME: mp2Run
 *
 * DESCRIPTION: This function performs all the key value store related functionalities
 * 				including:
 * 				1) Ring operations
 * 				2) CRUD operations
 */
void Application::mp2Run() {
	int i;

	// For all the nodes in the system
	for (i = 0; i <= par->NUM_PEERS-1; i++)
	{
		/*
		 * 1) Update the ring
		 * 2) Receive messages from the network and queue them in the KV store queue
		 */
		if (par->getcurrtime() > (int)(par->STEP_RATE*i) &&
		    !mp2[i]->getMemberNode()->failed)
		{
			if ( mp2[i]->getMemberNode()->inited && mp2[i]->getMemberNode()->inGroup)
			{
				// Step 1
				mp2[i]->updateRing();
			}
			// Step 2
			mp2[i]->recvLoop();
		}
	}

	/**
	 * Handle messages from the queue and update the DHT
	 */
	for (i = par->NUM_PEERS-1; i >= 0; i--) {
		if (par->getcurrtime() > (int)(par->STEP_RATE*i) &&
		    !mp2[i]->getMemberNode()->failed)
		{
			mp2[i]->checkMessages();
		}
	}

	/**
	 * Insert a set of test key value pairs into the system
	 */
	if (par->getcurrtime() == INSERT_TIME) {
		insertTestKVPairs();
	}

	/**
	 * Test CRUD operations
	 */
	if (par->getcurrtime() >= TEST_TIME)
	{
		/**************
		 * CREATE TEST
		 **************/
		/**
		 * TEST 1: Checks if there are RF * NUMBER_OF_INSERTS CREATE SUCCESS message are in the log
		 *
		 */
		if (par->getcurrtime() == TEST_TIME && CREATE_TEST == par->testType)
		{
			std::cout << std::endl << "Doing create test at time: ";
			std::cout << par->getcurrtime() << std::endl;
		} // End of create test

		/***************
		 * DELETE TESTS
		 ***************/
		/**
		 * TEST 1: NUMBER_OF_INSERTS/2 Key Value pair are deleted.
		 * 		   Check whether RF * NUMBER_OF_INSERTS/2 DELETE SUCCESS message are in the log
		 * TEST 2: Delete a non-existent key. Check for a DELETE FAIL message in the lgo
		 *
		 */
		else if (par->getcurrtime() == TEST_TIME && DELETE_TEST == par->testType)
		{
			deleteTest();
		} // End of delete test

		/*************
		 * READ TESTS
		 *************/
		/**
		 * TEST 1: Read a key. Check for correct value being read in quorum of replicas
		 *
		 * Wait for some time after TEST 1
		 *
		 * TEST 2: Fail a single replica of a key. Check for correct value of the key
		 * 		   being read in quorum of replicas
		 *
		 * Wait for STABILIZE_TIME after TEST 2 (stabilization protocol should ensure at least
		 * 3 replicas for all keys at all times)
		 *
		 * TEST 3 part 1: Fail two replicas of a key. Read the key and check for READ FAIL message in the log.
		 * 				  READ should fail because quorum replicas of the key are not up
		 *
		 * Wait for another STABILIZE_TIME after TEST 3 part 1 (stabilization protocol should ensure at least
		 * 3 replicas for all keys at all times)
		 *
		 * TEST 3 part 2: Read the same key as TEST 3 part 1. Check for correct value of the key
		 * 		  		  being read in quorum of replicas
		 *
		 * Wait for some time after TEST 3 part 2
		 *
		 * TEST 4: Fail a non-replica. Check for correct value of the key
		 * 		   being read in quorum of replicas
		 *
		 * TEST 5: Read a non-existent key. Check for a READ FAIL message in the log
		 *
		 */
		else if (par->getcurrtime() >= TEST_TIME && READ_TEST == par->testType)
		{
			readTest();
		} // end of read test

		/***************
		 * UPDATE TESTS
		 ***************/
		/**
		 * TEST 1: Update a key. Check for correct new value being updated in quorum of replicas
		 *
		 * Wait for some time after TEST 1
		 *
		 * TEST 2: Fail a single replica of a key. Update the key. Check for correct new value of the key
		 * 		   being updated in quorum of replicas
		 *
		 * Wait for STABILIZE_TIME after TEST 2 (stabilization protocol should ensure at least
		 * 3 replicas for all keys at all times)
		 *
		 * TEST 3 part 1: Fail two replicas of a key. Update the key and check for READ FAIL message in the log
		 * 				  UPDATE should fail because quorum replicas of the key are not up
		 *
		 * Wait for another STABILIZE_TIME after TEST 3 part 1 (stabilization protocol should ensure at least
		 * 3 replicas for all keys at all times)
		 *
		 * TEST 3 part 2: Update the same key as TEST 3 part 1. Check for correct new value of the key
		 * 		   		  being update in quorum of replicas
		 *
		 * Wait for some time after TEST 3 part 2
		 *
		 * TEST 4: Fail a non-replica. Check for correct new value of the key
		 * 		   being updated in quorum of replicas
		 *
		 * TEST 5: Update a non-existent key. Check for a UPDATE FAIL message in the log
		 *
		 */
		else if (par->getcurrtime() >= TEST_TIME && UPDATE_TEST == par->testType)
		{
			updateTest();
		} // End of update test

	} // end of if ( par->getcurrtime == TEST_TIME)
}

/**
 * FUNCTION NAME: getjoinaddr
 *
 * DESCRIPTION: This function returns the address of the coordinator
 */
Address Application::getjoinaddr(void)
{
	//trace.funcEntry("Application::getjoinaddr");
    Address joinaddr;
    joinaddr.init();
    *(int *)(&(joinaddr.addr))=1;
    *(short *)(&(joinaddr.addr[4]))=0;
    //trace.funcExit("Application::getjoinaddr", SUCCESS);
    return joinaddr;
}

/**
 * FUNCTION NAME: findARandomNodeThatIsAlive
 *
 * DESCRTPTION: Finds a random node in the ring that is alive
 */
int Application::findARandomNodeThatIsAlive() {
	int number;
	do
	{
		number = (rand()%par->NUM_PEERS);
	} while (mp2[number]->getMemberNode()->failed);
	return number;
}

/**
 * FUNCTION NAME: initTestKVPairs
 *
 * DESCRIPTION: Init NUMBER_OF_INSERTS test KV pairs in the map
 */
void Application::initTestKVPairs() {
	srand(time(NULL));
	int i;
	string key;
	key.clear();
	testKVPairs.clear();
	int alphanumLen = sizeof(alphanum) - 1;
	while (testKVPairs.size() != NUMBER_OF_INSERTS)
	{
		for (i = 0; i < KEY_LENGTH; i++)
		{
			key.push_back(alphanum[(rand()%alphanumLen)]);
		}
		string value = "value" + to_string((rand()%NUMBER_OF_INSERTS));
		testKVPairs[key] = value;
		key.clear();
	}
}

/**
 * FUNCTION NAME: insertTestKVPairs
 *
 * DESCRIPTION: This function inserts test KV pairs into the system
 */
void Application::insertTestKVPairs() {
	int number = 0;

	/*
	 * Init a few test key value pairs
	 */
	initTestKVPairs();

	for (map<string, string>::iterator it = testKVPairs.begin();
	     it != testKVPairs.end();
			 ++it )
	{
		// Step 1. Find a node that is alive
		number = findARandomNodeThatIsAlive();

		// Step 2. Issue a create operation
		log->LOG(&mp2[number]->getMemberNode()->addr,
		         "CREATE OPERATION KEY: %s VALUE: %s at time: %d",
						 it->first.c_str(),
						 it->second.c_str(),
						 par->getcurrtime());
		mp2[number]->clientCreate(it->first, it->second);
	}

	std::cout << std::endl;
	std::cout << "Sent " <<testKVPairs.size();
	std::cout << " create messages to the ring" << std::endl;
}

/**
 * FUNCTION NAME: deleteTest
 *
 * DESCRIPTION: Test the delete API of the KV store
 */
void Application::deleteTest() {
	int number;
	/**
	 * Test 1: Delete half the KV pairs
	 */
	std::cout << std::endl;
	std::cout << "Deleting " << testKVPairs.size()/2;
	std::cout << " valid keys.... ... .. . ." << std::endl;
	map<string, string>::iterator it = testKVPairs.begin();
	for (int i = 0; i < testKVPairs.size()/2; i++) {
    // Skip past the first node which is the coordinator.
		it++;

		// Step 1.a. Find a node that is alive
		number = findARandomNodeThatIsAlive();

		// Step 1.b. Issue a delete operation
		log->LOG(
			&mp2[number]->getMemberNode()->addr,
			"DELETE OPERATION KEY: %s VALUE: %s at time: %d",
			it->first.c_str(),
			it->second.c_str(),
			par->getcurrtime());
		mp2[number]->clientDelete(it->first);
	}

	/**
	 * Test 2: Delete a non-existent key
	 */
	std::cout << std::endl;
	std::cout << "Deleting an invalid key.... ... .. . ." << std::endl;
	string invalidKey = "invalidKey";
	// Step 2.a. Find a node that is alive
	number = findARandomNodeThatIsAlive();

	// Step 2.b. Issue a delete operation
	log->LOG(&mp2[number]->getMemberNode()->addr,
	         "DELETE OPERATION KEY: %s at time: %d",
					 invalidKey.c_str(),
					 par->getcurrtime());
	mp2[number]->clientDelete(invalidKey);
}

/**
 * FUNCTION NAME: readTest
 *
 * DESCRIPTION: Test the read API of the KV store
 */
void Application::readTest() {

	// Step 0. Key to be read
	// This key is used for all read tests
	map<string, string>::iterator it = testKVPairs.begin();
	int number;
	vector<Node> replicas;
	int replicaIdToFail = TERTIARY;
	int nodeToFail;
	bool failedOneNode = false;
	int targetTime = TEST_TIME;

	/**
 	 * Test 1: Test if value of a single read operation is read correctly in quorum number of nodes
 	 */
	if (par->getcurrtime() == targetTime)
	{
		// Step 1.a. Find a node that is alive
		number = findARandomNodeThatIsAlive();

		// Step 1.b Do a read operation
		std::cout << std::endl << "Reading a valid key.... ... .. . ." << std::endl;
		log->LOG(&mp2[number]->getMemberNode()->addr,
		         "READ OPERATION KEY: %s VALUE: %s at time: %d",
						 it->first.c_str(),
						 it->second.c_str(),
						 par->getcurrtime());
		mp2[number]->clientRead(it->first);
	}

	/** end of test1 **/

	/**
	 * Test 2: FAIL ONE REPLICA. Test if value is read correctly in quorum number of nodes after ONE OF THE REPLICAS IS FAILED
	 */
	targetTime += FIRST_FAIL_TIME;
	if (par->getcurrtime() == targetTime)
	{
		// Step 2.a Find a node that is alive and assign it as number
		number = findARandomNodeThatIsAlive();

		// Step 2.b Find the replicas of this key
		replicas.clear();
		replicas = mp2[number]->findNodes(it->first);
		// if less than quorum replicas are found then exit
		if (replicas.size() < (RF-1))
		{
			std::cout << std::endl;
			std::cout << "Could not find at least quorum replicas for this key. ";
			std::cout << "Exiting!!! size of replicas vector: ";
			std::cout << replicas.size() << std::endl;
			log->LOG(&mp2[number]->getMemberNode()->addr,
			         "Could not find at least quorum replicas for this key. Exiting!!! size of replicas vector: %d",
							 replicas.size());
			exit(1);
		}

		// Step 2.c Fail a replica
		for (int i = 0; i < par->NUM_PEERS; i++)
		{
			if (mp2[i]->getMemberNode()->addr.getAddress() ==
			    replicas.at(replicaIdToFail).getAddress()->getAddress())
			{
				if (!mp2[i]->getMemberNode()->failed)
				{
					nodeToFail = i;
					failedOneNode = true;
					break;
				}
				else
				{
					// Since we fail at most two nodes, one of the replicas must be alive
					if (replicaIdToFail > 0)
					{
						replicaIdToFail--;
					}
					else
					{
						failedOneNode = false;
					}
				}
			}
		}
		if (failedOneNode)
		{
			log->LOG(&mp2[nodeToFail]->getMemberNode()->addr,
			         "Node failed at time=%d",
							 par->getcurrtime());
			mp2[nodeToFail]->getMemberNode()->failed = true;
			mp1[nodeToFail]->getMemberNode()->failed = true;
			std::cout << std::endl << "Failed a replica node" << std::endl;
		}
		else
		{
			// The code can never reach here
			log->LOG(&mp2[number]->getMemberNode()->addr, "Could not fail a node");
			std::cout << "Could not fail a node. Exiting!!!" << std::endl;
			exit(1);
		}

		number = findARandomNodeThatIsAlive();

		// Step 2.d Issue a read
		std::cout << std::endl << "Reading a valid key.... ... .. . ." << std::endl;
		log->LOG(&mp2[number]->getMemberNode()->addr,
		         "READ OPERATION KEY: %s VALUE: %s at time: %d",
						 it->first.c_str(),
						 it->second.c_str(),
						 par->getcurrtime());
		mp2[number]->clientRead(it->first);

		failedOneNode = false;
	}

	/** end of test 2 **/

	/**
	 * Test 3 part 1: Fail two replicas. Test if value is read correctly in quorum number of nodes after TWO OF THE REPLICAS ARE FAILED
	 */
	// Wait for STABILIZE_TIME and fail two replicas
	targetTime += STABILIZE_TIME;
	if (par->getcurrtime() >= targetTime)
	{
		vector<int> nodesToFail;
		nodesToFail.clear();
		int count = 0;

		if (par->getcurrtime() == targetTime)
		{
			// Step 3.a. Find a node that is alive
			number = findARandomNodeThatIsAlive();

			// Get the keys replicas
			replicas.clear();
			replicas = mp2[number]->findNodes(it->first);

			// Step 3.b. Fail two replicas
			//cout<<"REPLICAS SIZE: "<<replicas.size();
			if (replicas.size() > 2)
			{
				replicaIdToFail = TERTIARY;
				while (count != 2)
				{
					int i = 0;
					while (i != par->NUM_PEERS)
					{
						if (mp2[i]->getMemberNode()->addr.getAddress() ==
						    replicas.at(replicaIdToFail).getAddress()->getAddress())
						{
							if (!mp2[i]->getMemberNode()->failed)
							{
								nodesToFail.emplace_back(i);
								replicaIdToFail--;
								count++;
								break;
							}
							else
							{
								// Since we fail at most two nodes, one of the replicas must be alive
								if (replicaIdToFail > 0)
								{
									replicaIdToFail--;
								}
							}
						}
						i++;
					}
				}
			}
			else
			{
				// If the code reaches here. Test your stabilization protocol
				std::cout << std::endl;
				std::cout << "Not enough replicas to fail two nodes. ";
				std::cout << "Number of replicas of this key: " <<replicas.size();
				std::cout << ". Exiting test case !!" << std::endl;
				exit(1);
			}
			if (count == 2)
			{
				for (int i = 0; i < nodesToFail.size(); i++)
				{
					// Fail a node
					log->LOG(&mp2[nodesToFail.at(i)]->getMemberNode()->addr,
					         "Node failed at time=%d",
									 par->getcurrtime());
					mp2[nodesToFail.at(i)]->getMemberNode()->failed = true;
					mp1[nodesToFail.at(i)]->getMemberNode()->failed = true;
					std::cout << std::endl << "Failed a replica node" << std::endl;
				}
			}
			else
			{
				// The code can never reach here
				log->LOG(&mp2[number]->getMemberNode()->addr,
				         "Could not fail two nodes");
				//cout<<"COUNT: " <<count;
				std::cout << "Could not fail two nodes. Exiting!!!";
				exit(1);
			}

			number = findARandomNodeThatIsAlive();

			// Step 3.c Issue a read
			std::cout << std::endl;
			std::cout << "Reading a valid key.... ... .. . ." << std::endl;
			log->LOG(&mp2[number]->getMemberNode()->addr,
			         "READ OPERATION KEY: %s VALUE: %s at time: %d",
							 it->first.c_str(),
							 it->second.c_str(),
							 par->getcurrtime());
			// This read should fail since at least quorum nodes are not alive
			mp2[number]->clientRead(it->first);
		}

		/**
		 * TEST 3 part 2: After failing two replicas and waiting for STABILIZE_TIME, issue a read
		 */
		// Step 3.d Wait for stabilization protocol to kick in
		targetTime += STABILIZE_TIME;
		if (par->getcurrtime() == targetTime)
		{
			number = findARandomNodeThatIsAlive();
			// Step 3.e Issue a read
			std::cout << std::endl;
			std::cout << "Reading a valid key.... ... .. . ." << std::endl;
			log->LOG(&mp2[number]->getMemberNode()->addr,
			         "READ OPERATION KEY: %s VALUE: %s at time: %d",
							 it->first.c_str(),
							 it->second.c_str(),
							 par->getcurrtime());
			// This read should be successful
			mp2[number]->clientRead(it->first);
		}
	}

	/** end of test 3 **/

	/**
	 * Test 4: FAIL A NON-REPLICA. Test if value is read correctly in quorum number of nodes after a NON-REPLICA IS FAILED
	 */
	targetTime += LAST_FAIL_TIME;
	if (par->getcurrtime() == targetTime)
	{
		// Step 4.a. Find a node that is alive
		number = findARandomNodeThatIsAlive();

		// Step 4.b Find a non - replica for this key
		replicas.clear();
		replicas = mp2[number]->findNodes(it->first);
		for ( int i = 0; i < par->NUM_PEERS; i++ )
		{
			if (!mp2[i]->getMemberNode()->failed)
			{
				std::string memberAddr = mp2[i]->getMemberNode()->addr.getAddress();
				if (memberAddr != replicas.at(PRIMARY).getAddress()->getAddress() &&
					 memberAddr != replicas.at(SECONDARY).getAddress()->getAddress() &&
					 memberAddr != replicas.at(TERTIARY).getAddress()->getAddress())
				{
					// Step 4.c Fail a non-replica node
					log->LOG(&mp2[i]->getMemberNode()->addr,
					         "Node failed at time=%d",
									 par->getcurrtime());
					mp2[i]->getMemberNode()->failed = true;
					mp1[i]->getMemberNode()->failed = true;
					failedOneNode = true;
					std::cout << std::endl << "Failed a non-replica node" << std::endl;
					break;
				}
			}
		}
		if (!failedOneNode) {
			// The code can never reach here
			log->LOG(&mp2[number]->getMemberNode()->addr,
			         "Could not fail a node(non-replica)");
			std::cout << "Could not fail a node(non-replica). Exiting!!!" << std::endl;
			exit(1);
		}

		number = findARandomNodeThatIsAlive();

		// Step 4.d Issue a read operation
		std::cout << endl << "Reading a valid key.... ... .. . ." << std::endl;
		log->LOG(&mp2[number]->getMemberNode()->addr,
		         "READ OPERATION KEY: %s VALUE: %s at time: %d",
						 it->first.c_str(),
						 it->second.c_str(),
						 par->getcurrtime());
		// This read should fail since at least quorum nodes are not alive
		mp2[number]->clientRead(it->first);
	}

	/** end of test 4 **/

	/**
	 * Test 5: Read a non-existent key.
	 */
	if (par->getcurrtime() == targetTime)
	{
		std::string invalidKey = "invalidKey";

		// Step 5.a Find a node that is alive
		number = findARandomNodeThatIsAlive();

		// Step 5.b Issue a read operation
		std::cout << std::endl;
		std::cout << "Reading an invalid key.... ... .. . ." << std::endl;
		log->LOG(&mp2[number]->getMemberNode()->addr,
		         "READ OPERATION KEY: %s at time: %d",
						 invalidKey.c_str(),
						 par->getcurrtime());
		// This read should fail since at least quorum nodes are not alive
		mp2[number]->clientRead(invalidKey);
	}
	/** end of test 5 **/
}

/**
 * FUNCTION NAME: updateTest
 *
 * DECRIPTION: This tests the update API of the KV Store
 */
void Application::updateTest() {
	// Step 0. Key to be updated
	// This key is used for all update tests
	map<string, string>::iterator it = testKVPairs.begin();
	it++;
	string newValue = "newValue";
	int number;
	vector<Node> replicas;
	int replicaIdToFail = TERTIARY;
	int nodeToFail;
	bool failedOneNode = false;

	/**
	 * Test 1: Test if value is updated correctly in quorum number of nodes
	 */
	int targetTime = TEST_TIME;
	if (par->getcurrtime() == targetTime)
	{
		// Step 1.a. Find a node that is alive
		number = findARandomNodeThatIsAlive();

		// Step 1.b Do a update operation
		std::cout << std::endl;
		std::cout << "Updating a valid key.... ... .. . ." << std::endl;
		log->LOG(&mp2[number]->getMemberNode()->addr,
		         "UPDATE OPERATION KEY: %s VALUE: %s at time: %d",
						 it->first.c_str(),
						 newValue.c_str(),
						 par->getcurrtime());
		mp2[number]->clientUpdate(it->first, newValue);
	}

	/** end of test 1 **/

	/**
	 * Test 2: FAIL ONE REPLICA. Test if value is updated correctly in quorum number of nodes after ONE OF THE REPLICAS IS FAILED
	 */
	targetTime += FIRST_FAIL_TIME;
	if (par->getcurrtime() == targetTime)
	{
		// Step 2.a Find a node that is alive and assign it as number
		number = findARandomNodeThatIsAlive();

		// Step 2.b Find the replicas of this key
		replicas.clear();
		replicas = mp2[number]->findNodes(it->first);
		// if quorum replicas are not found then exit
		if (replicas.size() < RF-1)
		{
			log->LOG(&mp2[number]->getMemberNode()->addr,
			         "Could not find at least quorum replicas for this key. Exiting!!! size of replicas vector: %d",
							 replicas.size());
			std::cout << std::endl;
			std::cout << "Could not find at least quorum replicas for this key. ";
			std::cout << "Exiting!!! size of replicas vector: ";
			std::cout << replicas.size() << std::endl;
			exit(1);
		}

		// Step 2.c Fail a replica
		for (int i = 0; i < par->NUM_PEERS; i++)
		{
			if (mp2[i]->getMemberNode()->addr.getAddress() ==
			    replicas.at(replicaIdToFail).getAddress()->getAddress())
			{
				if (!mp2[i]->getMemberNode()->failed)
				{
					nodeToFail = i;
					failedOneNode = true;
					break;
				}
				else
				{
					// Since we fail at most two nodes, one of the replicas must be alive
					if (replicaIdToFail > 0)
					{
						replicaIdToFail--;
					}
					else
					{
						failedOneNode = false;
					}
				}
			}
		}
		if (failedOneNode)
		{
			log->LOG(&mp2[nodeToFail]->getMemberNode()->addr,
			         "Node failed at time=%d",
							 par->getcurrtime());
			mp2[nodeToFail]->getMemberNode()->failed = true;
			mp1[nodeToFail]->getMemberNode()->failed = true;
			std::cout << std::endl << "Failed a replica node" << std::endl;
		}
		else
		{
			// The code can never reach here
			log->LOG(&mp2[number]->getMemberNode()->addr, "Could not fail a node");
			std::cout << "Could not fail a node. Exiting!!!" << std::endl;
			exit(1);
		}

		number = findARandomNodeThatIsAlive();

		// Step 2.d Issue a update
		std::cout << std::endl;
		std::cout << "Updating a valid key.... ... .. . ." << std::endl;
		log->LOG(&mp2[number]->getMemberNode()->addr,
		         "UPDATE OPERATION KEY: %s VALUE: %s at time: %d",
						 it->first.c_str(),
						 newValue.c_str(),
						 par->getcurrtime());
		mp2[number]->clientUpdate(it->first, newValue);

		failedOneNode = false;
	}

	/** end of test 2 **/

	/**
	 * Test 3 part 1: Fail two replicas. Test if value is updated correctly in quorum number of nodes after TWO OF THE REPLICAS ARE FAILED
	 */
	targetTime += STABILIZE_TIME;
	if (par->getcurrtime() >= targetTime)
	{
		vector<int> nodesToFail;
		nodesToFail.clear();
		int count = 0;

		if (par->getcurrtime() == targetTime) {
			// Step 3.a. Find a node that is alive
			number = findARandomNodeThatIsAlive();

			// Get the key's replicas
			replicas.clear();
			replicas = mp2[number]->findNodes(it->first);

			// Step 3.b. Fail two replicas
			if (replicas.size() > 2)
			{
				replicaIdToFail = TERTIARY;
				// So fail the tertiary and secondary replicas.
				while (count != 2)
				{
					int i = 0;
					while (i != par->NUM_PEERS)
					{
						if (mp2[i]->getMemberNode()->addr.getAddress() ==
						    replicas.at(replicaIdToFail).getAddress()->getAddress())
						{
							if (!mp2[i]->getMemberNode()->failed)
							{
								nodesToFail.emplace_back(i);
								replicaIdToFail--;
								count++;
								break;
							}
							else
							{
								// Since we fail at most two nodes, one of the replicas must be alive
								if (replicaIdToFail > 0) {
									replicaIdToFail--;
								}
							}
						}
						i++;
					}
				}
			}
			else
			{
				// If the code reaches here. Test your stabilization protocol
				std::cout << std::endl;
				std::cout << "Not enough replicas to fail two nodes. ";
				std::cout << "Exiting test case !! " << std::endl;
			}
			if (count == 2)
			{
				for (int i = 0; i < nodesToFail.size(); i++)
				{
					// Fail a node
					log->LOG(&mp2[nodesToFail.at(i)]->getMemberNode()->addr,
					         "Node failed at time=%d",
									 par->getcurrtime());
					mp2[nodesToFail.at(i)]->getMemberNode()->failed = true;
					mp1[nodesToFail.at(i)]->getMemberNode()->failed = true;
					std::cout << std::endl << "Failed a replica node" << std::endl;
				}
			}
			else
			{
				// The code can never reach here
				log->LOG(&mp2[number]->getMemberNode()->addr,
				         "Could not fail two nodes");
				std::cout << "Could not fail two nodes. Exiting!!!" << std::endl;
				exit(1);
			}

			number = findARandomNodeThatIsAlive();

			// Step 3.c Issue an update
			std::cout << std::endl;
			std::cout << "Updating a valid key.... ... .. . ." << std::endl;
			log->LOG(&mp2[number]->getMemberNode()->addr,
			         "UPDATE OPERATION KEY: %s VALUE: %s at time: %d",
							 it->first.c_str(),
							 newValue.c_str(),
							 par->getcurrtime());
			// This update should fail since at least quorum nodes are not alive
			mp2[number]->clientUpdate(it->first, newValue);
		}

		/**
		 * TEST 3 part 2: After failing two replicas and waiting for STABILIZE_TIME, issue an update
		 */
		// Step 3.d Wait for stabilization protocol to kick in
		targetTime += STABILIZE_TIME;
		if (par->getcurrtime() == targetTime)
		{
			number = findARandomNodeThatIsAlive();
			// Step 3.e Issue a update
			std::cout << std::endl;
			std::cout << "Updating a valid key.... ... .. . ." << std::endl;
			log->LOG(&mp2[number]->getMemberNode()->addr,
			         "UPDATE OPERATION KEY: %s VALUE: %s at time: %d",
							 it->first.c_str(),
							 newValue.c_str(),
							 par->getcurrtime());
			// This update should be successful
			mp2[number]->clientUpdate(it->first, newValue);
		}
	}

	/** end of test 3 **/

	/**
	 * Test 4: FAIL A NON-REPLICA. Test if value is read correctly in quorum number of nodes after a NON-REPLICA IS FAILED
	 */
	targetTime += LAST_FAIL_TIME;
	if (par->getcurrtime() == targetTime)
	{
		// Step 4.a. Find a node that is alive
		number = findARandomNodeThatIsAlive();

		// Step 4.b Find a non - replica for this key
		replicas.clear();
		replicas = mp2[number]->findNodes(it->first);
		for (int i = 0; i < par->NUM_PEERS; i++)
		{
			if (!mp2[i]->getMemberNode()->failed)
			{
				std::string memberAddr = mp2[i]->getMemberNode()->addr.getAddress();
				if (memberAddr != replicas.at(PRIMARY).getAddress()->getAddress() &&
					  memberAddr != replicas.at(SECONDARY).getAddress()->getAddress() &&
					  memberAddr != replicas.at(TERTIARY).getAddress()->getAddress())
				{
					// Step 4.c Fail a non-replica node
					log->LOG(&mp2[i]->getMemberNode()->addr,
					         "Node failed at time=%d",
									 par->getcurrtime());
					mp2[i]->getMemberNode()->failed = true;
					mp1[i]->getMemberNode()->failed = true;
					failedOneNode = true;
					std::cout << std::endl << "Failed a non-replica node" << std::endl;
					break;
				}
			}
		}

		if (!failedOneNode)
		{
			// The code can never reach here
			log->LOG(&mp2[number]->getMemberNode()->addr,
			         "Could not fail a node(non-replica)");
			std::cout << "Could not fail a node(non-replica). Exiting!!!";
			std::cout << std::endl;
			exit(1);
		}

		number = findARandomNodeThatIsAlive();

		// Step 4.d Issue a update operation
		std::cout << std::endl;
		std::cout << "Updating a valid key.... ... .. . ." << std::endl;
		log->LOG(&mp2[number]->getMemberNode()->addr,
		         "UPDATE OPERATION KEY: %s VALUE: %s at time: %d",
						 it->first.c_str(),
						 newValue.c_str(),
						 par->getcurrtime());
		// This read should fail since at least quorum nodes are not alive
		mp2[number]->clientUpdate(it->first, newValue);
	}

	/** end of test 4 **/

	/**
	 * Test 5: Udpate a non-existent key.
	 */
	if (par->getcurrtime() == targetTime)
	{
		string invalidKey = "invalidKey";
		string invalidValue = "invalidValue";

		// Step 5.a Find a node that is alive
		number = findARandomNodeThatIsAlive();

		// Step 5.b Issue a read operation
		std::cout << std::endl;
		std::cout << "Updating a valid key.... ... .. . ." << std::endl;
		log->LOG(&mp2[number]->getMemberNode()->addr,
		         "UPDATE OPERATION KEY: %s VALUE: %s at time: %d",
						 invalidKey.c_str(),
						 invalidValue.c_str(),
						 par->getcurrtime());
		// This read should fail since at least quorum nodes are not alive
		mp2[number]->clientUpdate(invalidKey, invalidValue);
	}
	/** end of test 5 **/
}
