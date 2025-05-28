/**********************************
 * FILE NAME: MP1Node.h
 *
 * DESCRIPTION: Membership protocol run by this Node.
 * 				Header file of MP1Node class.
 **********************************/

#ifndef _MP1NODE_H_
#define _MP1NODE_H_

#include "stdincludes.h"
#include "Log.h"
#include "Params.h"
#include "Address.h"
#include "Member.h"
#include "Message.h"
#include "EmulNet.h"
#include "Queue.h"

/**
 * CLASS NAME: MP1Node
 *
 * DESCRIPTION: Class implementing Membership protocol functionalities for failure detection
 */
class MP1Node {
private:
	std::shared_ptr<EmulNet> emulNet;
	std::shared_ptr<Log> log;
	const Params &par;
	std::shared_ptr<Member> memberNode;
	char NULLADDR[6];
  std::unique_ptr<AddressHandler> addressHandler;
  std::unordered_map<std::string, size_t> memTableIdx;
  std::string addrStr;

  static const short tCleanup;
  static const short tFail;
  static const short tGossip;

  void initThisNode();
	int introduceSelfToGroup(Address& joinAddress);
  void logEvent(const char* eventMsg, const Address& addr);
  void logMsg(const char* msg);
  void cleanMemberList();
  std::vector<MemberListEntry> getActiveNodes();
  void sendGossip(std::vector<MemberListEntry>& activeNodes,
                  std::unique_ptr<GossipMessage> gossipMsg);
  void handleGossipMessage(char* gossipData,
                           long numGossipEntries,
                           const Address& senderAddr);
  void addMembershipEntry(Address& newAddr, long newHeartbeat);
  void printMemberTable();
  void incrementHeartbeat();
	void printAddress(const Address& addr);

public:
	MP1Node(std::shared_ptr<Member>,
		      const Params&,
					std::shared_ptr<EmulNet>,
          std::shared_ptr<Log>,
					Address);

	std::shared_ptr<Member> getMemberNode() {
		return memberNode;
	}
	int recvLoop();
	static int enqueueWrapper(void *env, char *buff, int size);
	void nodeStart(char *servaddrstr, short serverport);
	int finishUpThisNode();
	void nodeLoop();
	void checkMessages();
	bool recvCallBack(char *data, int size);
	void nodeLoopOps();
	Address getJoinAddress();
	void initMemberListTable();
	virtual ~MP1Node();
};

#endif /* _MP1NODE_H_ */
