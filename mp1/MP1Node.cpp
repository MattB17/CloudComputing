/**********************************
 * FILE NAME: MP1Node.cpp
 *
 * DESCRIPTION: Membership protocol run by this Node.
 * 				Definition of MP1Node class functions.
 **********************************/

#include "MP1Node.h"

/*
 * Note: You can change/add any functions in MP1Node.{h,cpp}
 */

/**
 * JoinMessage constructor.
 * Initializes `msgSize` and allocates space for `msg`.
 */
JoinMessage::JoinMessage(Address* fromAddr,
	                       MsgTypes&& joinType,
												 long* heartbeat)
{
	// So a message is a messageHdr, followed by the to address,
	// followed by 1 byte, followed by a long representing the heartbeat.
	msgSize = sizeof(MessageHdr) + (6 * sizeof(char)) + 1 + sizeof(long);
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
JoinMessage::~JoinMessage() {
	free(msg);
}

/**
 * FUNCTION NAME: getMessage
 *
 * DESCRIPTION: Builds the join message based on the address `fromAddr`, the
 * join type `joinType` and the provided heartbeat `heartbeat`.
 */
char* JoinMessage::getMessage()
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
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "init_thisnode failed. Exit.");
#endif
        exit(1);
    }

    if( !introduceSelfToGroup(&joinaddr) ) {
        finishUpThisNode();
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Unable to join self to group. Exiting.");
#endif
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
#ifdef DEBUGLOG
    static char s[1024];
#endif

    if ( 0 == memcmp((char *)&(memberNode->addr.addr), (char *)&(joinaddr->addr), sizeof(memberNode->addr.addr))) {
        // I am the group booter (first process to join the group). Boot up the group
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Starting up group...");
#endif
        memberNode->inGroup = true;
    }
    else {
			JoinMessage joinMsg = JoinMessage(
				&memberNode->addr, JOINREQ, &memberNode->heartbeat);
			memberNode->heartbeat++;

#ifdef DEBUGLOG
        sprintf(s, "Trying to join...");
        log->LOG(&memberNode->addr, s);
#endif

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
	#ifdef DEBUGLOG
	    static char logMsg[1024];
	#endif

	MessageHdr *msgHeader = (MessageHdr *)(data);
	Address *senderAddr = (Address *)(data + sizeof(MessageHdr));
	long *senderHeartbeat = (long *)(data + sizeof(MessageHdr) + sizeof(Address) + 1);

	if (msgHeader->msgType == JOINREP)
	{
		// We received a reply to our join request, so we are now in the group.
		memberNode->inGroup = true;

		#ifdef DEBUGLOG
		    sprintf(logMsg, "Received reply from %d.%d.%d.%d:%d for join request",
			           senderAddr->addr[0],
							   senderAddr->addr[1],
							   senderAddr->addr[2],
							   senderAddr->addr[3],
								 addressHandler->portFromAddress(senderAddr));
				log->LOG(&memberNode->addr, logMsg);
		#endif

		addMembershipEntry(senderAddr, *senderHeartbeat);

	}
	else if (msgHeader->msgType == JOINREQ)
	{
		// Received a JOINREQ so need to send a JOINREP as the response.
		JoinMessage joinMsg = JoinMessage(
			&memberNode->addr, JOINREP, &memberNode->heartbeat);
		memberNode->heartbeat++;

		emulNet->ENsend(&(memberNode->addr), senderAddr,
		                joinMsg.getMessage(), joinMsg.getMessageSize());

		#ifdef DEBUGLOG
		    sprintf(logMsg,
					"Sending reply message for join request to %d.%d.%d.%d:%d",
			    senderAddr->addr[0],
					senderAddr->addr[1],
					senderAddr->addr[2],
					senderAddr->addr[3],
					addressHandler->portFromAddress(senderAddr));
				log->LOG(&memberNode->addr, logMsg);
		#endif

		addMembershipEntry(senderAddr, *senderHeartbeat);
	}
	// TODO: handle gossip message
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
	std::vector<MemberListEntry> cleanedMemberList;
	for (int mleIdx = 0; mleIdx < memberNode->memberList.size(); mleIdx++) {
		MemberListEntry *mle = &memberNode->memberList[mleIdx];
		if (par->getcurrtime() - mle->gettimestamp() < TCLEANUP) {
			cleanedMemberList.push_back(*mle);
		}
		else {
			Address entryAddr;
			addressHandler->addressFromIdAndPort(
				&entryAddr, mle->getid(), mle->getport());
			log->logNodeRemove(&memberNode->addr, &entryAddr);
		}
	}
	memberNode->memberList = cleanedMemberList;

	// Propagate the membership list if it's time to gossip again.
	if (memberNode->pingCounter > 0)
	{
		memberNode->pingCounter--;
	}
  else
	{
		// Time to gossip again.
		// Start by updating your own heartbeat.
		memberNode->myPos->setheartbeat(memberNode->heartbeat);
		memberNode->heartbeat++;
		// Next construct the gossip message.

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
	memberNode->myPos = memberNode->memberList.begin();
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
	memberNode->memberList.push_back(mle);
	log->logNodeAdd(&memberNode->addr, newAddr);
}
