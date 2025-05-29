/**********************************
 * FILE NAME: MP1Node.cpp
 *
 * DESCRIPTION: Membership protocol run by this Node.
 * 				Definition of MP1Node class functions.
 **********************************/

#include "MP1Node.h"
#include <random>

const short MP1Node::tCleanup = 20;
const short MP1Node::tFail = 10;
const short MP1Node::tGossip = 2;

/**
 * Overloaded Constructor of the MP1Node class
 * You can add new members to the class if you think it
 * is necessary for your logic to work
 */
MP1Node::MP1Node(std::shared_ptr<Member> member,
	               const Params &params,
								 std::shared_ptr<EmulNet> emul,
								 std::shared_ptr<Log> log,
								 Address address): par(params)
{
	for( int i = 0; i < 6; i++ ) {
		NULLADDR[i] = 0;
	}
	this->memberNode = member;
	this->emulNet = emul;
	this->log = log;
	this->memberNode->addr = address;
	this->addressHandler = std::make_unique<AddressHandler>();
	this->addrStr = std::string(address.addr);
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
int MP1Node::recvLoop()
{
    if (memberNode->failed)
		{
    	return false;
    }
    else
		{
    	return emulNet->ENrecv(
				&(memberNode->addr), enqueueWrapper, NULL, 1, &(memberNode->mp1q));
    }
}

/**
 * FUNCTION NAME: enqueueWrapper
 *
 * DESCRIPTION: Enqueue the message from Emulnet into the queue
 */
int MP1Node::enqueueWrapper(void *env, char *buff, int size)
{
	return Queue::enqueue((queue<q_elt> *)env, (void *)buff, size);
}

/**
 * FUNCTION NAME: nodeStart
 *
 * DESCRIPTION: This function bootstraps the node
 * 				All initializations routines for a member.
 * 				Called by the application layer.
 */
void MP1Node::nodeStart(char *servaddrstr, short servport)
{
	initThisNode();

  Address joinaddr = getJoinAddress();
  if(!introduceSelfToGroup(joinaddr)) {
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
void MP1Node::initThisNode()
{
	memberNode->failed = false;
	memberNode->inited = true;
	memberNode->inGroup = false;
    // node is up!
	memberNode->numNeighbours = 0;
	memberNode->heartbeat = 0;
	memberNode->pingCounter = MP1Node::tGossip;
  initMemberListTable();
}

/**
 * FUNCTION NAME: introduceSelfToGroup
 *
 * DESCRIPTION: Join the distributed system
 */
int MP1Node::introduceSelfToGroup(Address& joinaddr)
{
  if (memberNode->addr == joinaddr)
	{
    // I am the group booter (first process to join the group). Boot up the group
		logMsg("Starting up group...");
    memberNode->inGroup = true;
  }
  else
	{
		JoinMessage joinMsg = JoinMessage(&memberNode->addr,
			                                MembershipMessageType::JOIN_REQUEST,
																			&memberNode->heartbeat);
		logMsg("Trying to join...");

    // send JOIN_REQUEST message to introducer member
		// you send from your own address to the joinaddr, specifying the msg
		// and its size
    emulNet->ENsend(&memberNode->addr, &joinaddr,
					          joinMsg.getMessage(), joinMsg.getMessageSize());
  }

	return 1;
}

/**
 * FUNCTION NAME: finishUpThisNode
 *
 * DESCRIPTION: Wind up this node and clean up state
 */
int MP1Node::finishUpThisNode()
{
	 this->memberNode->inGroup = false;
	 this->emulNet->ENcleanup();

	 return 0;
}

/**
 * FUNCTION NAME: nodeLoop
 *
 * DESCRIPTION: Executed periodically at each member
 * 				Check your messages in queue and perform membership protocol duties
 */
void MP1Node::nodeLoop()
{
    if (memberNode->failed)
		{
    	return;
    }

    // Check my messages
    checkMessages();

    // Wait until you're in the group...
    if(!memberNode->inGroup) {
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
void MP1Node::checkMessages()
{
    void *ptr;
    int size;

    // Pop waiting messages from memberNode's mp1q
    while (!memberNode->mp1q.empty())
		{
    	ptr = memberNode->mp1q.front().elt;
    	size = memberNode->mp1q.front().size;
    	memberNode->mp1q.pop();
    	recvCallBack((char *)ptr, size);
    }
    return;
}

/**
 * FUNCTION NAME: recvCallBack
 *
 * DESCRIPTION: Message handler for different message types
 */
bool MP1Node::recvCallBack(char *data, int size)
{
  // Extract the message header and the address of the sender.
	// The *(T *) converts to a pointer to T then dereferences the pointer to
	// get an element of type T.
	MessageHdr msgHeader = *(MessageHdr *)(data);
	Address senderAddr = *(Address *)(data + sizeof(MessageHdr));

  if (msgHeader.msgType == GOSSIP)
	{
		// If it's a gossip message then the next long is the size of the
		// sender's member table.
		long memTableSize = *(long *)(
			data + sizeof(MessageHdr) + sizeof(senderAddr.addr) + 1);
		logEvent("Received gossip message from %d.%d.%d.%d:%d", senderAddr);

		handleGossipMessage(data, memTableSize, senderAddr);
	}
	else
	{
		// Otherwise, it's a join message (request or reply) and the payload is only
		// one long representing the sender's heartbeat.
		long senderHeartbeat = *(long *)(
			data + sizeof(MessageHdr) + sizeof(senderAddr.addr) + 1);

	  if (msgHeader.msgType == MembershipMessageType::JOIN_REPLY)
	  {
		  // We received a reply to our join request, so we are now in the group.
		  memberNode->inGroup = true;
		  logEvent(
				"Received reply from %d.%d.%d.%d:%d for join request", senderAddr);

		  addMembershipEntry(senderAddr, senderHeartbeat);
	  }
	  else if (msgHeader.msgType == MembershipMessageType::JOIN_REQUEST)
	  {
		  // Received a JOIN_REQUEST so need to send a JOIN_REPLY as the response.
			incrementHeartbeat();
		  JoinMessage joinMsg = JoinMessage(&memberNode->addr,
				                                MembershipMessageType::JOIN_REPLY,
																				&memberNode->heartbeat);

		  emulNet->ENsend(&(memberNode->addr), &senderAddr,
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
void MP1Node::nodeLoopOps()
{
	// Propagate the membership list if it's time to gossip again.
	if (memberNode->pingCounter == 0)
	{
		// Time to gossip again.
		// Start by updating your own heartbeat.
		incrementHeartbeat();

		// We want to send only the active nodes.
		std::vector<MemberListEntry> activeNodes = getActiveNodes();

		// Next construct the gossip message.
		std::unique_ptr<GossipMessage> gossipMsg = std::make_unique<GossipMessage>(
			memberNode->addr, activeNodes);

		// Send to a random subset of active neighbours
		sendGossip(activeNodes, std::move(gossipMsg));

		// Reset the ping counter.
		memberNode->pingCounter = MP1Node::tGossip;
	}
  else
	{
		memberNode->pingCounter--;
	}

	// Remove any nodes that have not been updated recently.
	cleanMemberList();

	return;
}

/**
 * FUNCTION NAME: getJoinAddress
 *
 * DESCRIPTION: Returns the Address of the coordinator
 */
Address MP1Node::getJoinAddress()
{
	return addressHandler->addressFromIdAndPort(1, 0);
}

/**
 * FUNCTION NAME: initMemberListTable
 *
 * DESCRIPTION: Initialize the membership list
 */
void MP1Node::initMemberListTable()
{
	memberNode->memberList.clear();
	// Add self to the table
	addMembershipEntry(memberNode->addr, memberNode->heartbeat);
}

/**
 * FUNCTION NAME: printAddress
 *
 * DESCRIPTION: Print the Address
 *
 * Currently not used but helpful for debugging.
 */
void MP1Node::printAddress(const Address& addr)
{
    printf("%d.%d.%d.%d:%d \n",  addr.addr[0],addr.addr[1],addr.addr[2],
           addr.addr[3], addressHandler->portFromAddress(addr));
}

/**
 * FUNCTION NAME: addMembershipEntry
 *
 * DESCRIPTION: Adds a membership entry to the member table for `newAddr`
 *              with heartbeat `newHeartbeat`.
 *
 * This method is only called for addresses that are not currently in the
 * membership list table.
 */
void MP1Node::addMembershipEntry(Address& newAddr, long newHeartbeat)
{
	std::string newAddrStr(newAddr.addr);
	auto mleItr = memTableIdx.find(newAddrStr);
	if (mleItr != memTableIdx.end()) {
		return;
	}

	int id = addressHandler->idFromAddress(newAddr);
	short port = addressHandler->portFromAddress(newAddr);
	MemberListEntry mle = MemberListEntry(
		id, port, newHeartbeat, par.getcurrtime());

	memTableIdx[newAddrStr] = memberNode->memberList.size();
	memberNode->memberList.push_back(mle);
	log->logNodeAdd(&memberNode->addr, &newAddr);
	if (memberNode->addr != newAddr)
	{
    memberNode->numNeighbours++;
  }
}

/**
 * FUNCTION NAME: logEvent
 *
 * DESCRIPTION: Logs an event that occurred at this node involving Address
 *              `addr` amd the message `eventMsg`.
 */
void MP1Node::logEvent(const char* eventMsg, const Address& addr)
{
	char logMsg[1024];
	snprintf(logMsg, sizeof(logMsg), eventMsg,
					 addr.addr[0],
					 addr.addr[1],
					 addr.addr[2],
					 addr.addr[3],
					 addressHandler->portFromAddress(addr));
	log->logDebug(&memberNode->addr, logMsg);
}

/**
 * FUNCTION NAME: logMsg
 *
 * DESCRIPTION: logs a simple message given by `msg`.
 */
void MP1Node::logMsg(const char* msg)
{
	log->logDebug(&memberNode->addr, msg);
}

/**
 * FUNCTION NAME: cleanMemberList
 *
 * DESCRIPTION: cleans the member list by removing any entries for nodes that
 *              have been inactive for a while.
 *              The nodes' removal is logged.
 */
void MP1Node::cleanMemberList()
{
	std::vector<MemberListEntry> cleanedMemberList;
	for (auto itr = memberNode->memberList.begin();
       itr != memberNode->memberList.end();
		   itr++)
	{
		Address entryAddr = addressHandler->addressFromIdAndPort(
			itr->getid(),
			itr->getport());
		std::string entryAddrStr(entryAddr.addr);

		if (par.getcurrtime() - itr->gettimestamp() <= MP1Node::tCleanup ||
	      entryAddr == memberNode->addr)
		{
			memTableIdx[entryAddrStr] = cleanedMemberList.size();
			cleanedMemberList.emplace_back(*itr);
		}
		else
		{
			log->logNodeRemove(&memberNode->addr, &entryAddr);
			memTableIdx.erase(entryAddrStr);
			memberNode->numNeighbours--;
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
std::vector<MemberListEntry> MP1Node::getActiveNodes()
{
	std::vector<MemberListEntry> activeNodes;
	for (auto itr = memberNode->memberList.begin();
			 itr != memberNode->memberList.end();
			 itr++)
	{
		if ((par.getcurrtime() - itr->gettimestamp()) <= MP1Node::tFail) {
			activeNodes.emplace_back(*itr);
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
                         std::unique_ptr<GossipMessage> gossipMsg)
{
	std::random_device rd;
	std::mt19937 g(rd());
	std::shuffle(activeNodes.begin(), activeNodes.end(), g);
	int neighborsInGossip = (int)(Config::gossipProportion * activeNodes.size());

  char* msg = gossipMsg->getMessage();

	for (int i = 0; i < neighborsInGossip; i++)
	{
		Address destAddr = addressHandler->addressFromIdAndPort(
			activeNodes[i].getid(), activeNodes[i].getport());
		if (destAddr == memberNode->addr)
		{
			continue;
		}
		emulNet->ENsend(&memberNode->addr, &destAddr,
										msg, gossipMsg->getMessageSize());
		logEvent("Sending gossip message to %d.%d.%d.%d:%d", destAddr);
	}
}

void MP1Node::handleGossipMessage(char* gossipData,
	                                long numGossipEntries,
																	const Address& senderAddr)
{
	size_t offset = (
		sizeof(MessageHdr) + sizeof(senderAddr.addr) + 1 + sizeof(long));
	for (int i = 0; i < numGossipEntries; i++)
	{
		// Parse data for this gossip entry.
		int currId = *(int *)(gossipData + offset);
		offset += sizeof(int);
		short currPort = *(short *)(gossipData + offset);
		offset += sizeof(short);
		long currHeartbeat = *(long *)(gossipData + offset);
		offset += sizeof(long);

		Address currAddress = addressHandler->addressFromIdAndPort(
			currId, currPort);
		logEvent("Received gossip message from %d.%d.%d.%d:%d", currAddress);
		if (currAddress == memberNode->addr)
		{
			continue;
		}

		std::string currAddressStr(currAddress.addr);
		auto currAddrIdx = memTableIdx.find(currAddressStr);
		if (currAddrIdx == memTableIdx.end())
		{
			addMembershipEntry(currAddress, currHeartbeat);
		}
		else
		{
			MemberListEntry* currMle = &memberNode->memberList[currAddrIdx->second];
			bool isActive = (
				(par.getcurrtime() - currMle->gettimestamp()) <= MP1Node::tFail ||
			  currAddress == senderAddr);
			if (isActive && currHeartbeat > currMle->getheartbeat())
			{
				logEvent("Updating heartbeat for %d.%d.%d.%d:%d", currAddress);
				memberNode->memberList[currAddrIdx->second] = MemberListEntry(
					currId, currPort, currHeartbeat, par.getcurrtime());
			}
		}
	}
}

/**
 * FUNCTION NAME: printMemberTable
 *
 * DESCRIPTION: prints the membership table to the log.
 */
void MP1Node::printMemberTable()
{
	logMsg("Printing member list table:");
	for (auto itr = memberNode->memberList.begin();
       itr != memberNode->memberList.end();
		   itr++)
	{
		Address mleAddress = addressHandler->addressFromIdAndPort(
			itr->getid(), itr->getport());
		static char logMsg[1024];
	  snprintf(
			logMsg,
			sizeof(logMsg),
			"Entry for %d.%d.%d.%d:%d with heartbeat %ld, last updated at %ld",
			mleAddress.addr[0],
			mleAddress.addr[1],
			mleAddress.addr[2],
			mleAddress.addr[3],
			addressHandler->portFromAddress(mleAddress),
			itr->getheartbeat(),
			itr->gettimestamp());
	  log->logDebug(&memberNode->addr, logMsg);
	}
}

void MP1Node::incrementHeartbeat()
{
	memberNode->heartbeat++;
	auto itr = memTableIdx.find(addrStr);
	if (itr == memTableIdx.end())
	{
		logMsg("Something has gone wrong, cannot find self!");
		exit(1);
	}
	memberNode->memberList[itr->second].setheartbeat(memberNode->heartbeat);
	memberNode->memberList[itr->second].settimestamp(par.getcurrtime());
}
