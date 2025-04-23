/**********************************
 * FILE NAME: MP2Node.cpp
 *
 * DESCRIPTION: MP2Node class definition
 **********************************/
#include "MP2Node.h"

/**
 * CONSTRUCTOR
 *
 * Used in the case the transaction is a create or update
 */
TransactionState::TransactionState(string k, string v, TransactionType t)
  : key(k), value(v), type(t), successCount(0), failureCount(0) {}

	/**
	 * CONSTRUCTOR
	 *
	 * Used in the case the transaction is a delete
	 */
TransactionState::TransactionState(string k)
  : key(k), value(""), type(TransactionType::T_DELETE),
	  successCount(0), failureCount(0) {}

/**
 * constructor
 */
MP2Node::MP2Node(Member *memberNode, Params *par, EmulNet * emulNet, Log * log, Address * address) {
	this->memberNode = memberNode;
	this->par = par;
	this->emulNet = emulNet;
	this->log = log;
	ht = new HashTable();
	this->memberNode->addr = *address;
}

/**
 * Destructor
 */
MP2Node::~MP2Node() {
	delete ht;
	delete memberNode;
}

/**
 * FUNCTION NAME: updateRing
 *
 * DESCRIPTION: This function does the following:
 * 				1) Gets the current membership list from the Membership Protocol
 *           (MP1Node). The membership list is returned as a vector of Nodes.
 *           See Node class in Node.h
 * 				2) Constructs the ring based on the membership list
 * 				3) Calls the Stabilization Protocol
 */
void MP2Node::updateRing() {
	/*
	 * Implement this. Parts of it are already implemented
	 */
	vector<Node> currMemList;
	// Indicates whether the ring has changed
	bool change = false;

	/*
	 *  Step 1. Get the current membership list from Membership Protocol / MP1
	 *
	 * Note this membership list will consist on nodes and their addresses.
	 */
	currMemList = getMembershipList();

	/*
	 * Step 2: Construct the ring: Sort the list based on the hashCode
	 *
	 * As nodes are sorted by the hash code of their address this puts the nodes
	 * in their clockwise order around the ring.
	 */
	sort(currMemList.begin(), currMemList.end());

	// Now need to determine if the ring has changed.
	if (currMemList.size() != this->ring.size())
	{
		// Clearly if the membership list has a different size then it has.
		change = true;
	}
	else
	{
		// Otherwise, we iterate through until either we find a node that differs
		// or all nodes match.
		for (size_t ringIdx = 0; ringIdx < currMemList.size(); ringIdx++) {
			if (currMemList[ringIdx].nodeHashCode != this->ring[ringIdx].nodeHashCode)
			{
				change = true;
				break;
			}
		}
	}

	// Then assign the current membership list to the ring
	this->ring = currMemList;


	/*
	 * Step 3: Run the stabilization protocol IF REQUIRED
	 */
	// Run stabilization protocol if the hash table size is greater than zero and
	// if there has been a change in the ring
	if (change && !this->ht->isEmpty())
	{
		this->stabilizationProtocol();
	}
}

/**
 * FUNCTION NAME: getMemberhipList
 *
 * DESCRIPTION: This function goes through the membership list from the Membership protocol/MP1 and
 * 				i) generates the hash code for each member
 * 				ii) populates the ring member in MP2Node class
 * 				It returns a vector of Nodes. Each element in the vector contain the following fields:
 * 				a) Address of the node
 * 				b) Hash code obtained by consistent hashing of the Address
 */
vector<Node> MP2Node::getMembershipList() {
	unsigned int i;
	vector<Node> curMemList;
	for ( i = 0 ; i < this->memberNode->memberList.size(); i++ ) {
		Address addressOfThisMember;
		int id = this->memberNode->memberList.at(i).getid();
		short port = this->memberNode->memberList.at(i).getport();
		memcpy(&addressOfThisMember.addr[0], &id, sizeof(int));
		memcpy(&addressOfThisMember.addr[4], &port, sizeof(short));
		curMemList.emplace_back(Node(addressOfThisMember));
	}
	return curMemList;
}

/**
 * FUNCTION NAME: hashFunction
 *
 * DESCRIPTION: This functions hashes the key and returns the position on the ring
 * 				HASH FUNCTION USED FOR CONSISTENT HASHING
 *
 * RETURNS:
 * size_t position on the ring
 */
size_t MP2Node::hashFunction(string key) {
	std::hash<string> hashFunc;
	size_t ret = hashFunc(key);
	return ret%RING_SIZE;
}

/**
 * FUNCTION NAME: clientCreate
 *
 * DESCRIPTION: client side CREATE API
 * 				The function does the following:
 * 				1) Constructs the message
 * 				2) Finds the replicas of this key
 * 				3) Sends a message to the replica
 */
void MP2Node::clientCreate(string key, string value) {
	std::vector<Node> replicas = findNodes(key);
	std::vector<ReplicaType> replicaTypes = {
		ReplicaType::PRIMARY, ReplicaType::SECONDARY, ReplicaType::TERTIARY};

  // Get an ID for the current transaction.
	int currTransId = g_transID;
	g_transID++;


  // Iterate through the replicas.
	for (int rIdx = 0; rIdx < 3; rIdx++) {
		// Construct create message for the replica.
		Message cMsg = Message(
			currTransId,
			this->memberNode->addr,
		  MessageType::CREATE,
		  key,
		  value,
		  replicaTypes[rIdx]);
		// Send the create message to the replica.
		this->emulNet->ENsend(
			&(this->memberNode->addr),
			&(replicas[rIdx].nodeAddress),
			cMsg.toString());
	}

  // Keep a record of the pending transaction.
	incompleteTxns.insert(
		{currTransId, TransactionState(key, value, TransactionType::T_CREATE)});
}

/**
 * FUNCTION NAME: clientRead
 *
 * DESCRIPTION: client side READ API
 * 				The function does the following:
 * 				1) Constructs the message
 * 				2) Finds the replicas of this key
 * 				3) Sends a message to the replica
 */
void MP2Node::clientRead(string key){
	/*
	 * Implement this
	 */
}

/**
 * FUNCTION NAME: clientUpdate
 *
 * DESCRIPTION: client side UPDATE API
 * 				The function does the following:
 * 				1) Constructs the message
 * 				2) Finds the replicas of this key
 * 				3) Sends a message to the replica
 */
void MP2Node::clientUpdate(string key, string value){
	/*
	 * Implement this
	 */
}

/**
 * FUNCTION NAME: clientDelete
 *
 * DESCRIPTION: client side DELETE API
 * 				The function does the following:
 * 				1) Constructs the message
 * 				2) Finds the replicas of this key
 * 				3) Sends a message to the replica
 */
void MP2Node::clientDelete(string key){
	std::vector<Node> replicas = findNodes(key);

  // Get the transaction ID for this transaction.
	int currTransId = g_transID;
	g_transID++;

  // Iterate through the replicas.
	for (int rIdx = 0; rIdx < replicas.size(); rIdx++)
	{
		// Construct the delete message for the replica.
		Message dMsg = Message(
			currTransId,
			this->memberNode->addr,
			MessageType::DELETE,
			key);

		// Send the delete message to the replica.
		this->emulNet->ENsend(
			&(this->memberNode->addr),
			&(replicas[rIdx].nodeAddress),
			dMsg.toString());
	}

	// Keep a record of the pending transaction.
	// We use the TransactionState constructor for delete states.
	incompleteTxns.insert({currTransId, TransactionState(key)});
}

/**
 * FUNCTION NAME: createKeyValue
 *
 * DESCRIPTION: Server side CREATE API
 * 			   	The function does the following:
 * 			   	1) Inserts key value into the local hash table
 * 			   	2) Return true or false based on success or failure
 */
bool MP2Node::createKeyValue(string key, string value, ReplicaType replica) {
	return this->ht->create(key, value);
}

/**
 * FUNCTION NAME: readKey
 *
 * DESCRIPTION: Server side READ API
 * 			    This function does the following:
 * 			    1) Read key from local hash table
 * 			    2) Return value
 */
string MP2Node::readKey(string key) {
	/*
	 * Implement this
	 */
	// Read key from local hash table and return value
}

/**
 * FUNCTION NAME: updateKeyValue
 *
 * DESCRIPTION: Server side UPDATE API
 * 				This function does the following:
 * 				1) Update the key to the new value in the local hash table
 * 				2) Return true or false based on success or failure
 */
bool MP2Node::updateKeyValue(string key, string value, ReplicaType replica) {
	/*
	 * Implement this
	 */
	// Update key in local hash table and return true or false
}

/**
 * FUNCTION NAME: deleteKey
 *
 * DESCRIPTION: Server side DELETE API
 * 				This function does the following:
 * 				1) Delete the key from the local hash table
 * 				2) Return true or false based on success or failure
 */
bool MP2Node::deletekey(string key) {
	return this->ht->deleteKey(key);
}

/**
 * FUNCTION NAME: checkMessages
 *
 * DESCRIPTION: This function is the message handler of this node.
 * 				This function does the following:
 * 				1) Pops messages from the queue
 * 				2) Handles the messages according to message types
 */
void MP2Node::checkMessages() {
	char * data;
	int size;

	/*
	 * Declare your local variables here
	 */

	// dequeue all messages and handle them
	while ( !memberNode->mp2q.empty() ) {
		/*
		 * Pop a message from the queue
		 */
		data = (char *)memberNode->mp2q.front().elt;
		size = memberNode->mp2q.front().size;
		memberNode->mp2q.pop();

		string message(data, data + size);
		Message msg = Message(message);

		switch (msg.type)
		{
			case MessageType::CREATE:
			  handleCreateMessage(msg);
				break;
			case MessageType::DELETE:
			  handleDeleteMessage(msg);
				break;
			case MessageType::REPLY:
			  handleReplyMessage(msg);
				break;
			default:
			  break;
		}
	}

	/*
	 * This function should also ensure all READ and UPDATE operation
	 * get QUORUM replies
	 */
}

/**
 * FUNCTION NAME: findNodes
 *
 * DESCRIPTION: Find the replicas of the given keyfunction
 * 				This function is responsible for finding the replicas of a key
 */
vector<Node> MP2Node::findNodes(string key) {
	size_t pos = hashFunction(key);
	vector<Node> addr_vec;
	if (ring.size() >= 3) {
		// if pos <= min || pos > max, the leader (primary replica) is the min
		if (pos <= ring.at(0).getHashCode() ||
		    pos > ring.at(ring.size()-1).getHashCode()) {
			addr_vec.emplace_back(ring.at(0));
			addr_vec.emplace_back(ring.at(1));
			addr_vec.emplace_back(ring.at(2));
		}
		else {
			// go through the ring until pos <= node
			for (int i=1; i<ring.size(); i++){
				Node addr = ring.at(i);
				if (pos <= addr.getHashCode()) {
					addr_vec.emplace_back(addr);
					addr_vec.emplace_back(ring.at((i+1)%ring.size()));
					addr_vec.emplace_back(ring.at((i+2)%ring.size()));
					break;
				}
			}
		}
	}
	return addr_vec;
}

/**
 * FUNCTION NAME: recvLoop
 *
 * DESCRIPTION: Receive messages from EmulNet and push into the queue (mp2q)
 */
bool MP2Node::recvLoop() {
    if ( memberNode->bFailed ) {
    	return false;
    }
    else {
    	return emulNet->ENrecv(
				&(memberNode->addr), this->enqueueWrapper,
				NULL, 1, &(memberNode->mp2q));
    }
}

/**
 * FUNCTION NAME: enqueueWrapper
 *
 * DESCRIPTION: Enqueue the message from Emulnet into the queue of MP2Node
 */
int MP2Node::enqueueWrapper(void *env, char *buff, int size) {
	Queue q;
	return q.enqueue((queue<q_elt> *)env, (void *)buff, size);
}
/**
 * FUNCTION NAME: stabilizationProtocol
 *
 * DESCRIPTION: This runs the stabilization protocol in case of Node joins and leaves
 * 				It ensures that there always 3 copies of all keys in the DHT at all times
 * 				The function does the following:
 *				1) Ensures that there are three "CORRECT" replicas of all the keys in spite of failures and joins
 *				Note:- "CORRECT" replicas implies that every key is replicated in its two neighboring nodes in the ring
 */
void MP2Node::stabilizationProtocol() {
	/*
	 * Implement this
	 */
}

/**
 * FUNCTION NAME: handleCreateMessage
 *
 * DESCRIPTION: Performs the server-side handling of a create message
 *              That is, it:
 *              1) Creates the key-value pair on the server side
 *              2) Logs whether the creation was successful
 *              3) Sends a reply back to the client node that initiated the
 *                 create.
 */
void MP2Node::handleCreateMessage(Message& msg)
{
	bool created = this->createKeyValue(msg.key, msg.value, msg.replica);

  // Log success or failure.
	if (created)
	{
		this->log->logCreateSuccess(
			&(this->memberNode->addr),
			false,
			msg.transID,
			msg.key,
			msg.value);
	}
	else
	{
		this->log->logCreateFail(
			&(this->memberNode->addr),
			false,
			msg.transID,
			msg.key,
			msg.value);
	}

  // Construct the reply message.
	Message replyMsg = Message(
		msg.transID,
	  this->memberNode->addr,
		MessageType::REPLY,
		created);

	// Send the reply back to the coordinator.
	this->emulNet->ENsend(
		&(this->memberNode->addr),
		&(msg.fromAddr),
		replyMsg.toString());
}

/**
 * FUNCTION NAME: handleDeleteMessage
 *
 * DESCRIPTION: Performs the server-side handling of a delete message
 *              That is, it:
 *              1) Deletes the key-value pair on the server side
 *              2) Logs whether the deletion was successful
 *              3) Sends a reply back to the client node that initiated the
 *                 delete.
 */
void MP2Node::handleDeleteMessage(Message& msg)
{
	bool deleted = this->deletekey(msg.key);

	// Log success or failure.
	if (deleted)
	{
		this->log->logDeleteSuccess(
			&(this->memberNode->addr),
			true,
			msg.transID,
			msg.key);
	}
	else
	{
		this->log->logDeleteFail(
			&(this->memberNode->addr),
			true,
			msg.transID,
			msg.key);
	}

	// Construct the reply message.
	Message replyMsg = Message(
		msg.transID,
		this->memberNode->addr,
		MessageType::REPLY,
		deleted);

	// Send the reply to the coordinator
	this->emulNet->ENsend(
		&(this->memberNode->addr),
		&(msg.fromAddr),
		replyMsg.toString());
}

/*
 * FUNCTION NAME: handleReplyMessage
 *
 * DESCRIPTION: Handles receipt of a reply message on the server side.
 *              That is, it
 *              1) Checks the reply is for a pending transaction
 *              2) Updates the success or failure count of that transaction
 *                 based on the reply message.
 *              3) If the success/failure count has reached a quorum the
 *                 success/failure of the transaction is logged
 *              4) If all replies have been completed for the transaction then
 *                 the transaction is marked as completed.
 */
void MP2Node::handleReplyMessage(Message& msg)
{
	auto txnPointer = this->incompleteTxns.find(msg.transID);
	if (txnPointer == this->incompleteTxns.end())
	{
		// This is not a pending transaction, so discard the message.
		return;
	}

	if (msg.success)
	{
		txnPointer->second.recordSuccess();
	}
	else
	{
		txnPointer->second.recordFailure();
	}

	if (txnPointer->second.hasTransactionSucceeded())
	{
		logTransactionSuccess(msg.transID);
	}
	else if (txnPointer->second.hasTransactionFailed())
	{
		logTransactionFailure(msg.transID);
	}

	if (txnPointer->second.allRepliesReceived())
	{
		this->incompleteTxns.erase(msg.transID);
	}
}

/*
 * FUNCTION NAME: logTransactionSuccess
 *
 * DESCRIPTION: Logs the successful completion of a transaction from the
 *              coordinator node.
 */
void MP2Node::logTransactionSuccess(int transId)
{
	auto txnPointer = this->incompleteTxns.find(transId);
	if (txnPointer == incompleteTxns.end()) {
		return;
	}

	switch (txnPointer->second.getTransactionType())
	{
		case TransactionType::T_CREATE:
		  this->log->logCreateSuccess(
				&(this->memberNode->addr),
			  true,
			  transId,
			  txnPointer->second.getKey(),
			  txnPointer->second.getValue());
			break;
		case TransactionType::T_UPDATE:
		  this->log->logUpdateSuccess(
				&(this->memberNode->addr),
				true,
				transId,
				txnPointer->second.getKey(),
				txnPointer->second.getValue());
		  break;
		case TransactionType::T_DELETE:
		  this->log->logDeleteSuccess(
				&(this->memberNode->addr),
				true,
				transId,
				txnPointer->second.getKey());
			break;
		default:
		  this->log->LOG(&(this->memberNode->addr), "Unsupported transaction type");
			break;
	}
}


/*
 * FUNCTION NAME: logTransactionFailure
 *
 * DESCRIPTION: Logs the unsuccessful completion of a transaction from the
 *              coordinator node.
 */
void MP2Node::logTransactionFailure(int transId)
{
	auto txnPointer = this->incompleteTxns.find(transId);
	if (txnPointer == incompleteTxns.end()) {
		return;
	}

	switch (txnPointer->second.getTransactionType())
	{
		case TransactionType::T_CREATE:
		  this->log->logCreateFail(
				&(this->memberNode->addr),
			  true,
			  transId,
			  txnPointer->second.getKey(),
			  txnPointer->second.getValue());
			break;
		case TransactionType::T_UPDATE:
		  this->log->logUpdateFail(
				&(this->memberNode->addr),
				true,
				transId,
				txnPointer->second.getKey(),
				txnPointer->second.getValue());
		  break;
		case TransactionType::T_DELETE:
		  this->log->logDeleteFail(
				&(this->memberNode->addr),
				true,
				transId,
				txnPointer->second.getKey());
			break;
		default:
		  this->log->LOG(&(this->memberNode->addr), "Unsupported transaction type");
			break;
	}
}
