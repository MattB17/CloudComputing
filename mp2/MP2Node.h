/**********************************
 * FILE NAME: MP2Node.h
 *
 * DESCRIPTION: MP2Node class header file
 **********************************/

#ifndef MP2NODE_H_
#define MP2NODE_H_

/**
 * Header files
 */
#include "stdincludes.h"
#include "EmulNet.h"
#include "Node.h"
#include "HashTable.h"
#include "Log.h"
#include "Params.h"
#include "Message.h"
#include "Queue.h"

// Transaction types
enum TransactionType {T_CREATE, T_READ, T_UPDATE, T_DELETE};

/*
 * CLASS NAME: TransactionState
 *
 * DESCRIPTION: Used to handle the state of pending transactions.
 *
 * This class is only intended for creates, updates, or deletes as we do not
 * need to reconcile possibly conflicting values between the replicas. Rather
 * we just need to track whether the operations were successful or not at each
 * replica and monitor when a quorum is reached.
 */
class TransactionState {
private:
	string key;
	string value;
	TransactionType type;
	short successCount;
	short failureCount;

public:
	// For create or update transactions
	TransactionState(string k, string v, TransactionType t);

  // For delete transactions
	TransactionState(string k);

  string getKey() { return key; }
	string getValue() { return value; }
	TransactionType getTransactionType() { return type; }

	void recordSuccess() { successCount++; }
	void recordFailure() { failureCount++; }

  // Indicates whether the transaction has succeeded.
	// Note, we record the successCount equaling 2 to ensure we don't record a
	// success multiple times (when it equals 2 and if it reaches a higher value).
	bool hasTransactionSucceeded() { return successCount == 2; }
	// Indicates whether the transaction has failed.
	bool hasTransactionFailed() { return failureCount == 2; }

  // Indicates whether all replies for the transaction have been received.
	bool allRepliesReceived() { return successCount + failureCount == 3; }
};

/**
 * CLASS NAME: ReadTransactionState
 *
 * DESCRIPTION: Maintains state for a READ transaction.
 *
 * The read transaction functions differently from the other operations, as
 * we need to track the values reported by the replicas. If 2 replicas report
 * the same value, a quorum is reached and we can return that value.
 *
 */
class ReadTransactionState {
private:
	string key;
	// Used to record the counts for each value seen among the replicas.
	// ie. if 2 replicas have the value `v` then `valueCounts` should have an
	// entry for `v` with a count of 2.
	unordered_map<string, int> valueCounts;

public:
	ReadTransactionState(string k);

	string getKey() { return key; }

	void recordReplicaValue(string v);

  // Indicates whether we have reached a quorum for value `v`.
	bool hasReachedQuorum(string v);

  // Indicates whether all replies for the transaction have been received.
	bool allRepliesReceived();
};

/**
 * CLASS NAME: MP2Node
 *
 * DESCRIPTION: This class encapsulates all the key-value store functionality
 * 				including:
 * 				1) Ring
 * 				2) Stabilization Protocol
 * 				3) Server side CRUD APIs
 * 				4) Client side CRUD APIs
 */
class MP2Node {
private:
	// Vector holding the next two neighbors in the ring who have my replicas
	vector<Node> hasMyReplicas;
	// Vector holding the previous two neighbors in the ring whose replicas I have
	vector<Node> haveReplicasOf;
	// Ring
	vector<Node> ring;
	// Hash Table
	HashTable * ht;
	// Member representing this member
	Member *memberNode;
	// Params object
	Params *par;
	// Object of EmulNet
	EmulNet * emulNet;
	// Object of Log
	Log * log;

	std::unordered_map<int, TransactionState> incompleteTxns;

	void handleCreateMessage(Message& msg);
	void handleDeleteMessage(Message& msg);
	void handleUpdateMessage(Message& msg);
	void handleReplyMessage(Message& msg);

	void logTransactionSuccess(int transId);
	void logTransactionFailure(int transId);

	int getTransactionId();
	void sendReplyToCoordinator(Message& coordMsg, bool operationSucceeded);

public:
	MP2Node(Member *memberNode, Params *par, EmulNet *emulNet, Log *log, Address *addressOfMember);
	Member * getMemberNode() {
		return this->memberNode;
	}

	// ring functionalities
	void updateRing();
	vector<Node> getMembershipList();
	size_t hashFunction(string key);
	void findNeighbors();

	// client side CRUD APIs
	void clientCreate(string key, string value);
	void clientRead(string key);
	void clientUpdate(string key, string value);
	void clientDelete(string key);

	// receive messages from Emulnet
	bool recvLoop();
	static int enqueueWrapper(void *env, char *buff, int size);

	// handle messages from receiving queue
	void checkMessages();

	// coordinator dispatches messages to corresponding nodes
	void dispatchMessages(Message message);

	// find the addresses of nodes that are responsible for a key
	vector<Node> findNodes(string key);

	// server
	bool createKeyValue(string key, string value, ReplicaType replica);
	string readKey(string key);
	bool updateKeyValue(string key, string value, ReplicaType replica);
	bool deletekey(string key);

	// stabilization protocol - handle multiple failures
	void stabilizationProtocol();

	~MP2Node();
};

#endif /* MP2NODE_H_ */
