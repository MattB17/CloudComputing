/**********************************
 * FILE NAME: MP1Node.cpp
 *
 * DESCRIPTION: Membership protocol run by this Node.
 * 				Definition of MP1Node class functions.
 **********************************/

#include "MP1Node.h"
#include <random>

/*
 * Note: You can change/add any functions in MP1Node.{h,cpp}
 */

/**
 * JoinMessage constructor.
 *
 * DESCRIPTION: Builds the message based on the source address `fromAddr`, the
 * heartbeat `heartbeat`, and the join message type `joinType`
 */
JoinMessage::JoinMessage(Address* fromAddr,
	                       MsgTypes&& joinType,
												 long* heartbeat)
{
	// So a message is a messageHdr, followed by the to address,
	// followed by 1 byte, followed by a long representing the heartbeat.
	msgSize = sizeof(MessageHdr) + sizeof(fromAddr->addr) + 1 + sizeof(long);
	msg = (MessageHdr *) malloc(msgSize * sizeof(char));
	msg->msgType = joinType;
	memcpy((char *)(msg + 1), &fromAddr->addr, sizeof(fromAddr->addr));
	memcpy(
		(char *)(msg + 1) + 1 + sizeof(fromAddr->addr), heartbeat, sizeof(long));
}

/**
 * JoinMessage destructor.
 * Frees the allocated `msg`.
 */
JoinMessage::~JoinMessage()
{
	free(msg);
}

/**
 * FUNCTION NAME: getMessage
 *
 * DESCRIPTION: Returns the message.
 */
char* JoinMessage::getMessage()
{
	return (char *) msg;
}

/**
 * Constructor for a GossipMessage.
 *
 * The gossip message is built from the Address `fromAddr` and the active nodes
 * in `memTable`.
 */
GossipMessage::GossipMessage(Address* fromAddr,
													   std::vector<MemberListEntry>& memTable)
{
  long numEntries = memTable.size();

	// Will have message header, followed by source address, 1 bit for null
	// terminator, and then the number of entries in the membership table.
	msgSize = sizeof(MessageHdr) + sizeof(fromAddr->addr) + 1 + sizeof(long);
	// For each active entry in the membership table we will send its id, port,
	// and heartbeat (we don't need to send the timestamp as that is local time
  // and won't be used by the receiving process)
	msgSize += (numEntries * (sizeof(long) + sizeof(short) + sizeof(long)));

  // Allocate space for the message and set the message type, from address,
	// and size of membership table.
	msg = (MessageHdr *) malloc(msgSize * sizeof(char));
	msg->msgType = GOSSIP;
  memcpy((char *)(msg + 1), &fromAddr->addr, sizeof(fromAddr->addr));
	memcpy(
		(char *)(msg + 1) + sizeof(fromAddr->addr) + 1, &numEntries, sizeof(long));

	// Now fill in all the member entries.
	size_t offset = sizeof(fromAddr->addr) + 1 + sizeof(long);
	for (auto itr = memTable.begin(); itr != memTable.end(); itr++) {
		int id = itr->getid();
		short port = itr->getport();
		long heartbeat = itr->getheartbeat();
		memcpy((char *)(msg+1) + offset, &id, sizeof(int));
		offset += sizeof(int);
		memcpy((char *)(msg+1) + offset, &port, sizeof(short));
		offset += sizeof(short);
		memcpy((char *)(msg+1) + offset, &heartbeat, sizeof(long));
		offset += sizeof(long);
	}
}

/**
 * GossipMessage destructor.
 * Frees the allocated `msg`.
 */
GossipMessage::~GossipMessage()
{
	free(msg);
}

/**
 * FUNCTION NAME: getMessage
 *
 * DESCRIPTION: Returns the message.
 */
char* GossipMessage::getMessage()
{
	return (char *) msg;
}

/**
 * FUNCTION NAME: addressFromIdAndPort
 *
 * DESCRIPTION: builds Address `addr` from the supplied `id` and `port`.
 */
void AddressHandler::addressFromIdAndPort(Address* addr, int id, short port)
{
	*(int *)(addr->addr) = id;
	*(short *)(&addr->addr[4]) = port;
	return;
}

/**
 * FUNCTION NAME: idFromAddress
 *
 * DESCRIPTION: extracts the id portion of `addr`.
 */
int AddressHandler::idFromAddress(Address *addr)
{
	return *(int *)(addr->addr);
}

/**
 * FUNCTION NAME: portFromAddress
 *
 * DESCRIPTION: extracts the port portion of `addr`.
 */
short AddressHandler::portFromAddress(Address *addr)
{
	return *(short *)(&addr->addr[4]);
}

// TODO: implement GossipMessage

/**
 * Overloaded Constructor of the MP1Node class
 * You can add new members to the class if you think it
 * is necessary for your logic to work
 */
MP1Node::MP1Node(Member *member, Params *params, EmulNet *emul, Log *log, Address *address) {
	for( int i = 0; i < 6; i++ ) {
		NULLADDR[i] = 0;
	}
	this->memberNode = member;
	this->emulNet = emul;
	this->log = log;
	this->par = params;
	this->memberNode->addr = *address;
	this->addressHandler = new AddressHandler();
	this->gossipProp = 0.5;
	this->addrStr = std::string(address->addr);
}

/**
 * Destructor of the MP1Node class
 */
MP1Node::~MP1Node() {}

/**
 * FUNCTION NAME: recvLoop
 *
 * DESCRIPTION: This function receives message from the network and pushes into the queue
 * 				This function is called by a node to receive messages currently waiting for it
 */
int MP1Node::recvLoop() {
    if ( memberNode->bFailed ) {
    	return false;
    }
    else {
    	return emulNet->ENrecv(
				&(memberNode->addr), enqueueWrapper, NULL, 1, &(memberNode->mp1q));
    }
}

/**
 * FUNCTION NAME: enqueueWrapper
 *
 * DESCRIPTION: Enqueue the message from Emulnet into the queue
 */
int MP1Node::enqueueWrapper(void *env, char *buff, int size) {
	Queue q;
	return q.enqueue((queue<q_elt> *)env, (void *)buff, size);
}

/**
 * FUNCTION NAME: nodeStart
 *
 * DESCRIPTION: This function bootstraps the node
 * 				All initializations routines for a member.
 * 				Called by the application layer.
 */
void MP1Node::nodeStart(char *servaddrstr, short servport) {
  Address joinaddr;
  joinaddr = getJoinAddress();

  // Self booting routines
  if( initThisNode(&joinaddr) == -1 ) {
		logMsg("init_thisnode failed. Exiting.");
    exit(1);
  }

  if( !introduceSelfToGroup(&joinaddr) ) {
    finishUpThisNode();
		logMsg("Unable to join self to group. Exiting.");
    exit(1);
  }

  return;
}

/**
 * FUNCTION NAME: initThisNode
 *
 * DESCRIPTION: Find out who I am and start up
 */
int MP1Node::initThisNode(Address *joinaddr) {
	memberNode->bFailed = false;
	memberNode->inited = true;
	memberNode->inGroup = false;
    // node is up!
	memberNode->nnb = 0;
	memberNode->heartbeat = 0;
	memberNode->pingCounter = TGOSSIP;
	memberNode->timeOutCounter = -1;
  initMemberListTable(memberNode);

  return 0;
}

/**
 * FUNCTION NAME: introduceSelfToGroup
 *
 * DESCRIPTION: Join the distributed system
 */
int MP1Node::introduceSelfToGroup(Address *joinaddr) {
	MessageHdr *msg;

  if ( 0 == memcmp((char *)&(memberNode->addr.addr), (char *)&(joinaddr->addr), sizeof(memberNode->addr.addr))) {
    // I am the group booter (first process to join the group). Boot up the group
		logMsg("Starting up group...");
    memberNode->inGroup = true;
  }
  else {
		JoinMessage joinMsg = JoinMessage(
			&memberNode->addr, JOINREQ, &memberNode->heartbeat);
		logMsg("Trying to join...");

    // send JOINREQ message to introducer member
		// you send from your own address to the joinaddr, specifying the msg
		// and its size
    emulNet->ENsend(&memberNode->addr, joinaddr,
					          joinMsg.getMessage(), joinMsg.getMessageSize());
  }

  return 1;
}

/**
 * FUNCTION NAME: finishUpThisNode
 *
 * DESCRIPTION: Wind up this node and clean up state
 */
int MP1Node::finishUpThisNode(){
	 this->memberNode->inGroup = false;
   this->memberNode = nullptr;
	 this->emulNet->ENcleanup();
	 this->emulNet = nullptr;
	 this->log = nullptr;
	 this->par = nullptr;
	 delete this->addressHandler;

	 return 0;
}

/**
 * FUNCTION NAME: nodeLoop
 *
 * DESCRIPTION: Executed periodically at each member
 * 				Check your messages in queue and perform membership protocol duties
 */
void MP1Node::nodeLoop() {
    if (memberNode->bFailed) {
    	return;
    }

    // Check my messages
    checkMessages();

    // Wait until you're in the group...
    if( !memberNode->inGroup ) {
    	return;
    }

    // ...then jump in and share your responsibilites!
    nodeLoopOps();

    return;
}

/**
 * FUNCTION NAME: checkMessages
 *
 * DESCRIPTION: Check messages in the queue and call the respective message handler
 */
void MP1Node::checkMessages() {
    void *ptr;
    int size;

    // Pop waiting messages from memberNode's mp1q
    while ( !memberNode->mp1q.empty() ) {
    	ptr = memberNode->mp1q.front().elt;
    	size = memberNode->mp1q.front().size;
    	memberNode->mp1q.pop();
    	recvCallBack((void *)memberNode, (char *)ptr, size);
    }
    return;
}

/**
 * FUNCTION NAME: recvCallBack
 *
 * DESCRIPTION: Message handler for different message types
 */
bool MP1Node::recvCallBack(void *env, char *data, int size ) {
  // Extract the message header and the address of the sender.
	MessageHdr *msgHeader = (MessageHdr *)(data);
	Address *senderAddr = (Address *)(data + sizeof(MessageHdr));

  if (msgHeader->msgType == GOSSIP)
	{
		// If it's a gossip message then the next long is the size of the
		// sender's member table.
		long memTableSize = *(long *)(
			data + sizeof(MessageHdr) + sizeof(senderAddr->addr) + 1);
		logEvent("Received gossip message from %d.%d.%d.%d:%d", senderAddr);

		handleGossipMessage(data, memTableSize, senderAddr);
	}
	else
	{
		// Otherwise, it's a join message (request or reply) and the payload is only
		// one long representing the sender's heartbeat.
		long senderHeartbeat = *(long *)(
			data + sizeof(MessageHdr) + sizeof(senderAddr->addr) + 1);

	  if (msgHeader->msgType == JOINREP)
	  {
		  // We received a reply to our join request, so we are now in the group.
		  memberNode->inGroup = true;
		  logEvent(
				"Received reply from %d.%d.%d.%d:%d for join request", senderAddr);

		  addMembershipEntry(senderAddr, senderHeartbeat);

	  }
	  else if (msgHeader->msgType == JOINREQ)
	  {
		  // Received a JOINREQ so need to send a JOINREP as the response.
			incrementHeartbeat();
		  JoinMessage joinMsg = JoinMessage(
			  &memberNode->addr, JOINREP, &memberNode->heartbeat);

		  emulNet->ENsend(&(memberNode->addr), senderAddr,
		                  joinMsg.getMessage(), joinMsg.getMessageSize());
		  logEvent(
			  "Sending reply message for join request to %d.%d.%d.%d:%d", senderAddr);

		  addMembershipEntry(senderAddr, senderHeartbeat);
	  }
	}
	return true;
}

/**
 * FUNCTION NAME: nodeLoopOps
 *
 * DESCRIPTION: Check if any node hasn't responded within a timeout period
 *              and then delete the nodes.
 * 				      Propagate your membership list
 */
void MP1Node::nodeLoopOps() {
	// Remove any nodes that have not been updated in the last TCLEANUP
	// time steps.
	cleanMemberList();

	// Propagate the membership list if it's time to gossip again.
	if (memberNode->pingCounter > 0)
	{
		memberNode->pingCounter--;
	}
  else
	{
		// Time to gossip again.
		// Start by updating your own heartbeat.
		incrementHeartbeat();
		printMemberTable();

		// We want to send only the active nodes.
		std::vector<MemberListEntry> activeNodes = getActiveNodes();

		// Next construct the gossip message.
		GossipMessage gossipMsg = GossipMessage(&memberNode->addr, activeNodes);

		// Send to a random subset of active neighbours
		sendGossip(activeNodes, gossipMsg);

    // Reset the ping counter.
		memberNode->pingCounter = TGOSSIP;
	}
	return;
}

/**
 * FUNCTION NAME: isNullAddress
 *
 * DESCRIPTION: Function checks if the address is NULL
 */
int MP1Node::isNullAddress(Address *addr) {
	return (memcmp(addr->addr, NULLADDR, 6) == 0 ? 1 : 0);
}

/**
 * FUNCTION NAME: getJoinAddress
 *
 * DESCRIPTION: Returns the Address of the coordinator
 */
Address MP1Node::getJoinAddress() {
    Address joinaddr;

    memset(&joinaddr, 0, sizeof(Address));
		addressHandler->addressFromIdAndPort(&joinaddr, 1, 0);

    return joinaddr;
}

/**
 * FUNCTION NAME: initMemberListTable
 *
 * DESCRIPTION: Initialize the membership list
 */
void MP1Node::initMemberListTable(Member *memberNode) {
	memberNode->memberList.clear();
	// Add self to the table
	addMembershipEntry(&memberNode->addr, memberNode->heartbeat);
}

/**
 * FUNCTION NAME: printAddress
 *
 * DESCRIPTION: Print the Address
 */
void MP1Node::printAddress(Address *addr)
{
    printf("%d.%d.%d.%d:%d \n",  addr->addr[0],addr->addr[1],addr->addr[2],
           addr->addr[3], addressHandler->portFromAddress(addr));
}

void MP1Node::addMembershipEntry(Address* newAddr, long newHeartbeat)
{
	int id = addressHandler->idFromAddress(newAddr);
	short port = addressHandler->portFromAddress(newAddr);
	MemberListEntry mle = MemberListEntry(
		id, port, newHeartbeat, par->getcurrtime());

	std::string newAddrStr(newAddr->addr);
	memTableIdx[newAddrStr] = memberNode->memberList.size();
	memberNode->memberList.push_back(mle);
	log->logNodeAdd(&memberNode->addr, newAddr);
  memberNode->nnb++;
}

/**
 * FUNCTION NAME: logEvent
 *
 * DESCRIPTION: Logs an event that occurred at this node involving Address
 *              `addr` amd the message `eventMsg`.
 */
void MP1Node::logEvent(const char* eventMsg, Address* addr) {
	#ifdef DEBUGLOG
	  char logMsg[1024];
	  sprintf(logMsg, eventMsg,
					  addr->addr[0],
					  addr->addr[1],
					  addr->addr[2],
					  addr->addr[3],
					  addressHandler->portFromAddress(addr));
	  log->LOG(&memberNode->addr, logMsg);
	#endif
}

/**
 * FUNCTION NAME: logMsg
 *
 * DESCRIPTION: logs a simple message given by `msg`.
 */
void MP1Node::logMsg(const char* msg) {
	#ifdef DEBUGLOG
	  log->LOG(&memberNode->addr, msg);
	#endif
}

/**
 * FUNCTION NAME: cleanMemberList
 *
 * DESCRIPTION: cleans the member list by removing any entries for nodes that
 *              have been inactive for TCLEANUP time.
 *              The nodes removal is logged.
 */
void MP1Node::cleanMemberList() {
	std::vector<MemberListEntry> cleanedMemberList;
	for (int mleIdx = 0; mleIdx < memberNode->memberList.size(); mleIdx++) {
		MemberListEntry *mle = &memberNode->memberList[mleIdx];

		Address entryAddr;
		addressHandler->addressFromIdAndPort(
			&entryAddr, mle->getid(), mle->getport());
		std::string entryAddrStr(entryAddr.addr);

		if (par->getcurrtime() - mle->gettimestamp() <= TCLEANUP ||
	      entryAddr == memberNode->addr)
		{
			memTableIdx[entryAddrStr] = cleanedMemberList.size();
			cleanedMemberList.push_back(*mle);
		}
		else
		{
			log->logNodeRemove(&memberNode->addr, &entryAddr);
			memTableIdx.erase(entryAddrStr);
			memberNode->nnb--;
		}
	}

	memberNode->memberList = cleanedMemberList;
}

/**
 * FUNCTION NAME: getActiveNodes
 *
 * DESCRIPTION: Returns the subset of the member table corresponding to entry
 *              for nodes that have not failed.
 */
std::vector<MemberListEntry> MP1Node::getActiveNodes() {
	std::vector<MemberListEntry> activeNodes;
	for (auto itr = memberNode->memberList.begin();
			 itr != memberNode->memberList.end();
			 itr++)
	{
		if (par->getcurrtime() - itr->gettimestamp() <= TFAIL) {
			activeNodes.push_back(*itr);
		}
	}
	return activeNodes;
}

/**
 * FUNCTION NAME: sendGossip
 *
 * DESCRIPTION: Sends the gossip message `gossipMsg` to a random subset of the
 *              active members given by `activeNodes`.
 */
void MP1Node::sendGossip(std::vector<MemberListEntry>& activeNodes,
                         GossipMessage& gossipMsg)
{
	std::random_device rd;
	std::mt19937 g(rd());
	std::shuffle(activeNodes.begin(), activeNodes.end(), g);
	int neighborsInGossip = (int)(gossipProp * activeNodes.size());

  char* msg = gossipMsg.getMessage();

	for (int i = 0; i < neighborsInGossip; i++) {
		Address destAddr;
		addressHandler->addressFromIdAndPort(
			&destAddr, activeNodes[i].getid(), activeNodes[i].getport());
		if (destAddr == memberNode->addr) {
			continue;
		}
		emulNet->ENsend(&memberNode->addr, &destAddr,
										msg, gossipMsg.getMessageSize());
		logEvent("Sending gossip message to %d.%d.%d.%d:%d", &destAddr);
	}
}

void MP1Node::handleGossipMessage(char* gossipData,
	                                long numGossipEntries,
																	Address* senderAddr)
{
	size_t offset = (
		sizeof(MessageHdr) + sizeof(senderAddr->addr) + 1 + sizeof(long));
	for (int i = 0; i < numGossipEntries; i++) {
		// Parse data for this gossip entry.
		int currId = *(int *)(gossipData + offset);
		offset += sizeof(int);
		short currPort = *(short *)(gossipData + offset);
		offset += sizeof(short);
		long currHeartbeat = *(long *)(gossipData + offset);
		offset += sizeof(long);

		Address currAddress;
		addressHandler->addressFromIdAndPort(&currAddress, currId, currPort);
		if (currAddress == memberNode->addr) {
			continue;
		}

    logEvent("Handling member table entry for %d.%d.%d.%d:%d", &currAddress);

		std::string currAddressStr(currAddress.addr);
		auto currAddrIdx = memTableIdx.find(currAddressStr);
		if (currAddrIdx == memTableIdx.end())
		{
			addMembershipEntry(&currAddress, currHeartbeat);
		}
		else
		{
			MemberListEntry* currMle = &memberNode->memberList[currAddrIdx->second];
			if (par->getcurrtime() - currMle->gettimestamp() <= TFAIL &&
			    currHeartbeat > currMle->getheartbeat())
			{
				logEvent("Updating heartbeat for %d.%d.%d.%d:%d", &currAddress);
				currMle->setheartbeat(currHeartbeat);
				currMle->settimestamp(par->getcurrtime());
			}
		}
	}
	printMemberTable();
}

void MP1Node::printMemberTable()
{
	logMsg("Printing member list table:");
	for (int i = 0; i < memberNode->memberList.size(); i++) {
		MemberListEntry* mle = &memberNode->memberList[i];
		Address mleAddress;
		addressHandler->addressFromIdAndPort(
			&mleAddress, mle->getid(), mle->getport());
		#ifdef DEBUGLOG
		  static char logMsg[1024];
			sprintf(
				logMsg,
				"Entry for %d.%d.%d.%d:%d with heartbeat %ld, last updated at %ld",
				mleAddress.addr[0],
				mleAddress.addr[1],
				mleAddress.addr[2],
				mleAddress.addr[3],
				addressHandler->portFromAddress(&mleAddress),
			  mle->getheartbeat(),
			  mle->gettimestamp());
			log->LOG(&memberNode->addr, logMsg);
		#endif
	}
}

void MP1Node::incrementHeartbeat()
{
	memberNode->heartbeat++;
	auto itr = memTableIdx.find(addrStr);
	if (itr == memTableIdx.end()) {
		logMsg("Something has gone wrong, cannot find self!");
		exit(1);
	}
	memberNode->memberList[itr->second].setheartbeat(memberNode->heartbeat);
	memberNode->memberList[itr->second].settimestamp(par->getcurrtime());
}
