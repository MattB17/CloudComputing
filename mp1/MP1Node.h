/**********************************
 * FILE NAME: MP1Node.cpp
 *
 * DESCRIPTION: Membership protocol run by this Node.
 * 				Header file of MP1Node class.
 **********************************/

#ifndef _MP1NODE_H_
#define _MP1NODE_H_

#include "stdincludes.h"
#include "Log.h"
#include "Params.h"
#include "Member.h"
#include "EmulNet.h"
#include "Queue.h"
#include <unordered_map>

/**
 * Macros
 */
#define TCLEANUP 20
#define TFAIL 5
#define TGOSSIP 2

/*
 * Note: You can change/add any functions in MP1Node.{h,cpp}
 */

/**
 * Message Types
 */
enum MsgTypes{
    // Join request
    JOINREQ,
    // Join reply
    JOINREP,
    // Gossip message
    GOSSIP,
    UNKNOWN
};

/**
 * STRUCT NAME: MessageHdr
 *
 * DESCRIPTION: Header and content of a message
 */
typedef struct MessageHdr {
	enum MsgTypes msgType;
}MessageHdr;

/**
 * CLASS NAME: JoinMessage
 *
 * DESCRIPTION: Used to build join (JOINREP or JOINREQ) messages.
 */
class JoinMessage {
private:
  size_t msgSize;
  MessageHdr *msg;

public:
  JoinMessage(Address* fromAddr, MsgTypes&& joinType, long* heartbeat);
  ~JoinMessage();
  char* getMessage();
  size_t getMessageSize() {
    return msgSize;
  }
};

/**
 * CLASS NAME: GossipMessage
 *
 * DESCRIPTION: Used to build a gossip message that encapsulates the
 *              membership table.
 */
class GossipMessage {
private:
  size_t msgSize;
  MessageHdr *msg;

public:
  GossipMessage(Address* fromAddr, std::vector<MemberListEntry>& memTable);
  ~GossipMessage();
  char* getMessage();
  size_t getMessageSize() {
    return msgSize;
  }
};

/**
 * CLASS NAME: AddressHandler
 *
 * DESCRIPTION: Handles address low level operations.
 *              Used primarily for mapping from an address to id and port and
 *              vice versa.
 */
class AddressHandler {
public:
  AddressHandler() {}
  void addressFromIdAndPort(Address* addr, int id, short port);
  int idFromAddress(Address* addr);
  short portFromAddress(Address* addr);
};

/**
 * CLASS NAME: MP1Node
 *
 * DESCRIPTION: Class implementing Membership protocol functionalities for failure detection
 */
class MP1Node {
private:
	EmulNet *emulNet;
	Log *log;
	Params *par;
	Member *memberNode;
	char NULLADDR[6];
  AddressHandler *addressHandler;
  double gossipProp;
  std::unordered_map<std::string, size_t> memTableIdx;

  void logEvent(const char* eventMsg, Address* addr);
  void logMsg(const char* msg);
  void cleanMemberList();
  std::vector<MemberListEntry> getActiveNodes();
  void sendGossip(std::vector<MemberListEntry>& activeNodes,
                  GossipMessage& gossipMsg);

public:
	MP1Node(Member *, Params *, EmulNet *, Log *, Address *);
	Member * getMemberNode() {
		return memberNode;
	}
	int recvLoop();
	static int enqueueWrapper(void *env, char *buff, int size);
	void nodeStart(char *servaddrstr, short serverport);
	int initThisNode(Address *joinaddr);
	int introduceSelfToGroup(Address *joinAddress);
	int finishUpThisNode();
	void nodeLoop();
	void checkMessages();
	bool recvCallBack(void *env, char *data, int size);
	void nodeLoopOps();
	int isNullAddress(Address *addr);
	Address getJoinAddress();
	void initMemberListTable(Member *memberNode);
	void printAddress(Address *addr);
  void addMembershipEntry(Address *memberAddr, long memberHeartbeat);
	virtual ~MP1Node();
};

#endif /* _MP1NODE_H_ */
