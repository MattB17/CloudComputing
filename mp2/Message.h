/**********************************
 * FILE NAME: Message.h
 *
 * DESCRIPTION: Message class header file
 **********************************/
#ifndef MESSAGE_H_
#define MESSAGE_H_

#include "stdincludes.h"
#include "Address.h"
#include "Member.h"
#include "common.h"

/**
 * STRUCT NAME: MessageHdr
 *
 * DESCRIPTION: Header and content of a message
 */
typedef struct MessageHdr
{
  enum MembershipMessageType msgType;
} MessageHdr;

class MembershipMessage {
protected:
  size_t msgSize;
	MessageHdr *msg;

public:
	virtual char* getMessage() = 0;
	size_t getMessageSize() { return msgSize; }
};


/**
 * CLASS NAME: JoinMessage
 *
 * DESCRIPTION: Used to build join (JOINREP or JOINREQ) messages.
 */
class JoinMessage : public MembershipMessage {
public:
	JoinMessage(Address* fromAddr,
		          MembershipMessageType&& joinType,
							long* heartbeat);
	~JoinMessage();
	char* getMessage();
};

/**
 * CLASS NAME: GossipMessage
 *
 * DESCRIPTION: Used to build a gossip message that encapsulates the
 *              membership table.
 */
class GossipMessage : public MembershipMessage {
public:
	GossipMessage(Address* fromAddr, std::vector<MemberListEntry>& memTable);
	~GossipMessage();
	char* getMessage();
};

/**
 * CLASS NAME: Message
 *
 * DESCRIPTION: This class is used for message passing among nodes
 */
class Message{
public:
	KVMessageType type;
	ReplicaType replica;
	std::string key;
	std::string value;
	Address fromAddr;
	int transID;
	bool success; // success or not

	// delimiter
  static const std::string delimiter;

	// construct a message from a string (basically deserialize)
	Message(string message);
	Message(const Message& anotherMessage);
	// construct a create or update message
	Message(int _transID, Address _fromAddr, KVMessageType _type, string _key, string _value);
	Message(int _transID, Address _fromAddr, KVMessageType _type, string _key, string _value, ReplicaType _replica);
	// construct a read or delete message
	Message(int _transID, Address _fromAddr, KVMessageType _type, string _key);
	// construct reply message
	Message(int _transID, Address _fromAddr, bool _success);
	// construct read reply message
	Message(int _transID, Address _fromAddr, string _value);
	Message& operator = (const Message& anotherMessage);
	// serialize to a string
	std::string toString();
};

#endif
