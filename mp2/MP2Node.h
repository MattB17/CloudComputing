/**********************************
 * FILE NAME: MP2Node.h
 *
 * DESCRIPTION: MP2Node class header file
 **********************************/

#ifndef MP2NODE_H_
#define MP2NODE_H_

#include "stdincludes.h"
#include "Config.h"
#include "Address.h"
#include "EmulNet.h"
#include "Node.h"
#include "HashTable.h"
#include "Log.h"
#include "Params.h"
#include "Message.h"
#include "Queue.h"
#include "TransactionState.h"

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
	std::vector<Node> hasMyReplicas;
	// Vector holding the previous two neighbors in the ring whose replicas I have
	std::vector<Node> haveReplicasOf;
	std::vector<Node> ring;
	std::unique_ptr<HashTable> ht;
	std::shared_ptr<Member> memberNode; // This member
	const Params &par;
	std::shared_ptr<EmulNet> emulNet;
	std::shared_ptr<Log> log;
	std::unique_ptr<AddressHandler> addressHandler;

  // Stores replica metadata, that is the replica type for the given key.
	// This could be extended to hold other metadata in the future.
	std::unordered_map<std::string, ReplicaType> replicaMetadata;

  // Tracks writes initiated by this node.
	std::unordered_map<int, WriteTransactionState> pendingWrites;
	// Tracks reads initiated by this node.
	std::unordered_map<int, ReadTransactionState> pendingReads;

	static int transactionId;

	void handleCreateMessage(const Message& msg);
	void handleReadMessage(const Message& msg);
	void handleDeleteMessage(const Message& msg);
	void handleUpdateMessage(const Message& msg);
	void handleWriteReplyMessage(const Message& msg);
	void handleReadReplyMessage(const Message& msg);

	void logWriteSuccess(int transId);
	void logWriteFailure(int transId);

	int getTransactionId();
	void sendReplyToCoordinator(const Message& coordMsg,
		                          bool operationSucceeded);

	// Cleans up any transactions that have expired, recording failures if a
	// quorum was not reached.
	void removeExpiredTransactions();

  // Initializes the neighbourhood for the node. The neighbourhood consists of
	// the 2 predecessors and 2 successors of this node on the ring.
	void initializeNeighbourhood();

  // Sets the neighbourhood (2 successors and 2 predecessors) based on the
	// node's position on the ring.
	void setNeighbourhood(int myPosOnRing);

  // Determines whether the current node is the primary for the key.
	bool iAmPrimary(string key, int myIdx);

  // Helper method for sending messages.
	void sendMsg(const Address& toAddr, const Message& msg);

public:
	MP2Node(
		std::shared_ptr<Member> memberNode, const Params &par,
		std::shared_ptr<EmulNet> emulNet, std::shared_ptr<Log> log,
		Address addressOfMember);
	std::shared_ptr<Member> getMemberNode() {
		return this->memberNode;
	}

	// ring functionalities
	void updateRing();
	vector<Node> getMembershipList();
	size_t hashFunction(std::string key);
	void findNeighbors();

	// client side CRUD APIs
	void clientCreate(std::string key, std::string value);
	void clientRead(std::string key);
	void clientUpdate(std::string key, std::string value);
	void clientDelete(std::string key);

	// receive messages from Emulnet
	bool recvLoop();
	static int enqueueWrapper(void *env, char *buff, int size);

	// handle messages from receiving queue
	void checkMessages();

	// coordinator dispatches messages to corresponding nodes
	void dispatchMessages(Message message);

	// find the addresses of nodes that are responsible for a key
	vector<Node> findNodes(std::string key);

	// server
	bool createKeyValue(std::string key, std::string value, ReplicaType replica);
	string readKey(const std::string& key);
	bool updateKeyValue(const std::string& key,
		                  std::string value,
											ReplicaType replica);
	bool deletekey(const std::string& key);

	// stabilization protocol - handle multiple failures
	void stabilizationProtocol();

	~MP2Node();
};

#endif /* MP2NODE_H_ */
