/**********************************
 * FILE NAME: Message.cpp
 *
 * DESCRIPTION: Message class definition
 **********************************/
#include "Message.h"

MembershipMessage::~MembershipMessage() {}

/**
 * JoinMessage constructor.
 *
 * DESCRIPTION: Builds the message based on the source address `fromAddr`, the
 * heartbeat `heartbeat`, and the join message type `joinType`
 */
JoinMessage::JoinMessage(Address* fromAddr,
												 MembershipMessageType&& joinType,
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
GossipMessage::GossipMessage(const Address& fromAddr,
														std::vector<MemberListEntry>& memTable)
{
  long numEntries = memTable.size();

  // Will have message header, followed by source address, 1 bit for null
  // terminator, and then the number of entries in the membership table.
  msgSize = sizeof(MessageHdr) + sizeof(fromAddr.addr) + 1 + sizeof(long);
  // For each active entry in the membership table we will send its id, port,
  // and heartbeat (we don't need to send the timestamp as that is local time
	// and won't be used by the receiving process)
  msgSize += (numEntries * (sizeof(int) + sizeof(short) + sizeof(long)));

	// Allocate space for the message and set the message type, from address,
  // and size of membership table.
  msg = (MessageHdr *) malloc(msgSize * sizeof(char));
  msg->msgType = GOSSIP;
	memcpy((char *)(msg + 1), &fromAddr.addr, sizeof(fromAddr.addr));
  memcpy(
	 (char *)(msg + 1) + sizeof(fromAddr.addr) + 1, &numEntries, sizeof(long));


  // Now fill in all the member entries.
  size_t offset = sizeof(fromAddr.addr) + 1 + sizeof(long);
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

// Initializing static delimiter
const std::string Message::delimiter = "::";

/**
 * Constructor
 */
// transID::fromAddr::CREATE::key::value::ReplicaType
// transID::fromAddr::READ::key
// transID::fromAddr::UPDATE::key::value::ReplicaType
// transID::fromAddr::DELETE::key
// transID::fromAddr::WRITE_REPLY::sucess
// transID::fromAddr::READ_REPLY::value
Message::Message(std::string message)
{
	std::vector<string> tuple;
	size_t pos = message.find(delimiter);
	size_t start = 0;
	while (pos != string::npos)
	{
		string field = message.substr(start, pos-start);
		tuple.push_back(field);
		start = pos + 2;
		pos = message.find(delimiter, start);
	}
	tuple.push_back(message.substr(start));

	transID = stoi(tuple.at(0));
	Address addr(tuple.at(1));
	fromAddr = addr;
	type = static_cast<KVMessageType>(stoi(tuple.at(2)));
	switch (type)
	{
		case CREATE:
		case UPDATE:
			key = tuple.at(3);
			value = tuple.at(4);
			if (tuple.size() > 5)
				replica = static_cast<ReplicaType>(stoi(tuple.at(5)));
			break;
		case READ:
		case DELETE:
			key = tuple.at(3);
			break;
		case WRITE_REPLY:
			if (tuple.at(3) == "1")
				success = true;
			else
				success = false;
			break;
		case READ_REPLY:
			value = tuple.at(3);
			break;
	}
}

/**
 * Constructor
 */
// construct a create or update message
Message::Message(int _transID,
	               Address _fromAddr,
								 KVMessageType _type,
								 std::string _key,
								 std::string _value,
								 ReplicaType _replica)
	: type(_type), replica(_replica), key(_key), value(_value),
	  fromAddr(_fromAddr), transID(_transID) {}

/**
 * Constructor
 */
Message::Message(const Message& anotherMessage)
{
	this->fromAddr = anotherMessage.fromAddr;
	this->key = anotherMessage.key;
	this->replica = anotherMessage.replica;
	this->success = anotherMessage.success;
	this->transID = anotherMessage.transID;
	this->type = anotherMessage.type;
	this->value = anotherMessage.value;
}

/**
 * Constructor
 */
Message::Message(int _transID,
	               Address _fromAddr,
								 KVMessageType _type,
								 std::string _key,
								 std::string _value)
	: type(_type), key(_key), value(_value),
	  fromAddr(_fromAddr), transID(_transID) {}

/**
 * Constructor
 */
// construct a read or delete message
Message::Message(int _transID,
	               Address _fromAddr,
								 KVMessageType _type,
								 std::string _key)
	: type(_type), key(_key), value(""), fromAddr(_fromAddr), transID(_transID) {}

/**
 * Constructor
 */
// construct reply message
Message::Message(int _transID,
	               Address _fromAddr,
								 bool _success)
	: type(WRITE_REPLY), fromAddr(_fromAddr),
	  transID(_transID), success(_success) {}

/**
 * Constructor
 */
// construct read reply message
Message::Message(int _transID, Address _fromAddr, string _value)
  : type(READ_REPLY), value(_value), fromAddr(_fromAddr), transID(_transID) {}

/**
 * FUNCTION NAME: toString
 *
 * DESCRIPTION: Serialized Message in string format
 */
std::string Message::toString()
{
	std::string message = (to_string(transID) + delimiter +
	  fromAddr.getAddress() + delimiter + to_string(type) + delimiter);
	switch(type){
		case CREATE:
		case UPDATE:
			message += key + delimiter + value + delimiter + to_string(replica);
			break;
		case READ:
		case DELETE:
			message += key;
			break;
		case WRITE_REPLY:
			if (success)
				message += "1";
			else
				message += "0";
			break;
		case READ_REPLY:
			message += value;
			break;
	}
	return message;
}

/**
 * Assignment operator overloading
 */
Message& Message::operator =(const Message& anotherMessage)
{
	this->fromAddr = anotherMessage.fromAddr;
	this->key = anotherMessage.key;
	this->replica = anotherMessage.replica;
	this->success = anotherMessage.success;
	this->transID = anotherMessage.transID;
	this->type = anotherMessage.type;
	this->value = anotherMessage.value;
	return *this;
}
