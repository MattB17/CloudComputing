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
WriteTransactionState::WriteTransactionState(
	string k, string v, TransactionType t, int currTime)
  : key(k), value(v), type(t), startTime(currTime),
	  successCount(0), failureCount(0) {}

/**
 * CONSTRUCTOR
 *
 * Used in the case the transaction is a delete
 */
WriteTransactionState::WriteTransactionState(string k, int currTime)
  : key(k), value(""), type(TransactionType::T_DELETE),
	  startTime(currTime), successCount(0), failureCount(0) {}

/**
 * FUNCTION NAME: hasTransactionExpired
 *
 * DESCRIPTION: indicates whether the transaction has expired.
 */
bool WriteTransactionState::hasTransactionExpired(int currTime)
{
	return currTime - this->startTime > TRANSACTION_TIMEOUT;
}

/**
 * CONSTRUCTOR
 */
ReadTransactionState::ReadTransactionState(
	string k, int currTime): key(k), startTime(currTime) {}

/**
 * FUNCTION NAME: hasTransactionExpired
 *
 * DESCRIPTION: Indicates whether the transaction has timed out.
 */
bool ReadTransactionState::hasTransactionExpired(int currTime)
{
	return currTime - this->startTime > TRANSACTION_TIMEOUT;
}

/**
 * FUNCTION NAME: recordReplicaValue
 *
 * DESCRIPTION: Records the value `v` received from a replica.
 */
void ReadTransactionState::recordReplicaValue(string v)
{
	auto vItr = this->valueCounts.find(v);

	if (vItr == this->valueCounts.end())
	{
		// If we haven't seen this value before record it.
		this->valueCounts.insert({v, 1});
	}
	else
	{
		// Otherwise, increment the count for this value
		this->valueCounts[v]++;
	}
}

/*
 * FUNCTION NAME: hasReachedQuorum
 *
 * DESCRIPTION: Indicates whether we have reached a quorum for value `v`.
 */
bool ReadTransactionState::hasReachedQuorum(string v)
{
	auto vItr = this->valueCounts.find(v);

	// If we've never seen this value, then obviously we don't have a quorum
	// for this value.
	if (vItr == this->valueCounts.end())
	{
		return false;
	}
	// Otherwise, we check if the count equals 2 (note if we checked >= we could
  // potentially record a quorum multiple times if we receive the same value
  // from all 3 replicas).
	return vItr->second == 2;
}

/**
 * FUNCTION NAME: hasReachedQuorum
 *
 * DESCRIPTION: Indicates whether a quorum has been reached for any value.
 */
bool ReadTransactionState::hasReachedQuorum()
{
	for (auto itr = this->valueCounts.begin();
       itr != this->valueCounts.end();
		   itr++)
	{
		if (itr->second >= 2)
		{
			return true;
		}
	}
	return false;
}

/**
 * FUNCTION NAME: allRepliesReceived
 *
 * DESCRIPTION: Indicates whether replies have been received from all replicas
 *              and thus the transaction can be marked complete.
 */
bool ReadTransactionState::allRepliesReceived()
{
	int replyCount = 0;
	for (auto itr = this->valueCounts.begin();
	     itr != this->valueCounts.end();
			 itr++)
	{
		replyCount += itr->second;
	}

	return replyCount == 3;
}

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
void MP2Node::updateRing()
{
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

  bool ringUninitialized = this->haveReplicasOf.size() == 0;
	// Then assign the current membership list to the ring
	this->ring = currMemList;


	/*
	 * Step 3: Run the stabilization protocol IF REQUIRED
	 */
	if (ringUninitialized && this->ring.size() >= 3)
	{
		this->initializeNeighbourhood();
	}
	else if (change && !this->ht->isEmpty())
	{
		// Run stabilization protocol if there has been a change in the ring and the
		// hash table is not empty.
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
	int currTransId = getTransactionId();


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
	this->pendingWrites.insert({currTransId, WriteTransactionState(
		key, value, TransactionType::T_CREATE, this->par->getcurrtime())});
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
void MP2Node::clientRead(string key)
{
	std::vector<Node> replicas = findNodes(key);

	// Get an ID for the current transaction.
	int currTransId = getTransactionId();

	// Iterate through the replicas.
	for (int rIdx = 0; rIdx < replicas.size(); rIdx++)
	{
		// Construct message for the replica.
		Message rMsg = Message(
			currTransId,
			this->memberNode->addr,
			MessageType::READ,
			key);

		// Send the read message to the replica.
		this->emulNet->ENsend(
			&(this->memberNode->addr),
			&(replicas[rIdx].nodeAddress),
			rMsg.toString());
	}

	// Keep a record of the pending read transaction.
	this->pendingReads.insert(
		{currTransId, ReadTransactionState(key, this->par->getcurrtime())});
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
void MP2Node::clientUpdate(string key, string value)
{
	std::vector<Node> replicas = this->findNodes(key);
	std::vector<ReplicaType> replicaTypes = {
		ReplicaType::PRIMARY, ReplicaType::SECONDARY, ReplicaType::TERTIARY};

	// Get the transaction id for this transaction.
	int currTransId = this->getTransactionId();

	for (int rIdx = 0; rIdx < 3; rIdx++)
	{
		// Construct the message to send to the replica
		Message rMsg = Message(
			currTransId,
			this->memberNode->addr,
			MessageType::UPDATE,
			key,
			value,
			replicaTypes[rIdx]);

		// Send the message to the replica
		this->emulNet->ENsend(
			&(this->memberNode->addr),
			&(replicas[rIdx].nodeAddress),
			rMsg.toString());
	}

	// The coordinator will track the pending transaction
	this->pendingWrites.insert({currTransId, WriteTransactionState(
		key, value, TransactionType::T_UPDATE, this->par->getcurrtime())});
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
	int currTransId = getTransactionId();

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
	// We use the WriteTransactionState constructor for delete states.
	this->pendingWrites.insert(
		{currTransId, WriteTransactionState(key, this->par->getcurrtime())});
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
	bool created = this->ht->create(key, value);

	// If it was successfully created we want to also track its metadata.
	if (created)
	{
		this->replicaMetadata.insert({key, replica});
	}

	return created;
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
	return this->ht->read(key);
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
	bool updated = this->ht->update(key, value);

  // If it was updated record the replica type.
	if (updated)
	{
		auto metaItr = this->replicaMetadata.find(key);
		if (metaItr == this->replicaMetadata.end())
		{
			std::cout << "Something went wrong, we have no metadata for ";
			std::cout << key << std::endl;
			exit(1);
		}
		metaItr->second = replica;
	}

	return updated;
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
	bool deleted = this->ht->deleteKey(key);

	if (deleted)
	{
		this->replicaMetadata.erase(key);
	}

	return deleted;
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

    // Note: when re-replicating for failures / nodes joining we set the
		// transaction ID to -1 and do the necessary creates/deletes/updates but
		// without logging or replying to the coordinator.
		switch (msg.type)
		{
			case MessageType::CREATE:
			  handleCreateMessage(msg);
				break;
			case MessageType::READ:
			  handleReadMessage(msg);
				break;
			case MessageType::DELETE:
			  handleDeleteMessage(msg);
				break;
			case MessageType::UPDATE:
			  handleUpdateMessage(msg);
				break;
			case MessageType::REPLY:
			  handleReplyMessage(msg);
				break;
			case MessageType::READREPLY:
			  handleReadReplyMessage(msg);
				break;
			default:
			  std::cout << "Invalid message type received" << std::endl;
			  exit(1);
		}
	}

	this->removeExpiredTransactions();
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
 * FUNCTION NAME：iAmPrimary
 *
 * DESCRIPTION: determines whether the current node is the primary replica for
 *              `key` based on the node's position in the ring `myIdx`.
 */
bool MP2Node::iAmPrimary(string key, int myIdx)
{
	size_t keyHash = hashFunction(key);
	size_t myHash = this->ring.at(myIdx).getHashCode();
	// I'm the first process in the ring.
	if (myIdx == 0)
	{
		// Find the hash code of the last process in the ring
		size_t endHash = this->ring.at(this->ring.size() - 1).getHashCode();
		// You're the primary if the keyHash exceeds the last process in the ring
		// or is less than you're hashcode.
		if (keyHash <= myHash || keyHash > endHash)
		{
			return true;
		}
		return false;
	}

	// Otherwise, there's a node that comes before you in the ring and you're the
	// primary as long as the key's hash is greater than the hash of your
	// predecssor and <= your hash.
	size_t prevHash = this->ring.at(myIdx - 1).getHashCode();
	if (keyHash > prevHash && keyHash <= myHash)
	{
		return true;
	}
	return false;
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
  // We first iterate through the ring to find our position as well as the
	// position of our successors and predecessors if they are still alive.
	// Note: even if they are still alive they might not be in the same position
	// on the ring because of nodes joining and/or failing.
	std::vector<bool> haveReplicasOfAlive = std::vector<bool>({false, false});
	std::vector<bool> hasMyReplicasAlive = std::vector<bool>({false, false});
	int myPos = -1;

	for (int ringPos = 0; ringPos < this->ring.size(); ringPos++)
	{
		if (!haveReplicasOfAlive[0] && (this->ring[ringPos].nodeAddress ==
			  this->haveReplicasOf[0].nodeAddress))
		{
			haveReplicasOfAlive[0] = true;
		}
		else if (!haveReplicasOfAlive[1] && (this->ring[ringPos].nodeAddress ==
			       this->haveReplicasOf[1].nodeAddress))
		{
			haveReplicasOfAlive[1] = true;
		}
		else if (this->ring[ringPos].nodeAddress == this->memberNode->addr)
		{
			myPos = ringPos;
		}
		else if (!hasMyReplicasAlive[0] && (this->ring[ringPos].nodeAddress ==
		         this->hasMyReplicas[0].nodeAddress))
		{
			hasMyReplicasAlive[0] = true;
		}
		else if (!hasMyReplicasAlive[1] && (this->ring[ringPos].nodeAddress ==
		         this->hasMyReplicas[1].nodeAddress))
		{
			hasMyReplicasAlive[1] = true;
		}
	}

	if (myPos < 0)
	{
		std::cout << "Cannot find self!" << std::endl;
		exit(1);
	}

  // Copy old values as these will be needed for replica management.
	std::vector<Node> oldHaveReplicasOf = this->haveReplicasOf;
	std::vector<Node> oldHasMyReplicas = this->hasMyReplicas;

  // Update successors and predecessors.
	this->setNeighbourhood(myPos);

	// Now we loop through our keys and see which one's we are primary for.
	// For any key for which we are primary we handle the re-replication of keys
	// (if necessary). If we are not primary then some other process is primary
	// and will handle the replication.
	for (auto repItr = this->replicaMetadata.begin();
       repItr != this->replicaMetadata.end();
		   repItr++)
	{
		if (this->iAmPrimary(repItr->first, myPos))
		{
			// Start by retrieving the value and updating the replica metadata to
			// show you are primary.
			ReplicaType oldType = repItr->second;
			repItr->second = ReplicaType::PRIMARY;
			std::string v = this->ht->read(repItr->first);
			if (v.compare("") == 0)
			{
				std::cout << "Cannot find value for " << repItr->first << std::endl;
				exit(1);
			}

			// Now we use the previous ReplicaType to decide how to replicate the
			// keys.
			// For now we will assume there are only failures and no rejoins / new
			// nodes joining.

			// If I used to be tertiary but am now primary it means my two
			// predecessors have failed, so issue 2 creates to my new neighbours.
			if (oldType == ReplicaType::TERTIARY)
			{
				// create for secondary
				Message createMsg = Message(
					-1,
					this->memberNode->addr,
					MessageType::CREATE,
					repItr->first,
					v,
					ReplicaType::SECONDARY);
				this->emulNet->ENsend(
					&(this->memberNode->addr),
					&(this->hasMyReplicas[0].nodeAddress),
					createMsg.toString());

				// And for tertiary
				createMsg.replica = ReplicaType::TERTIARY;
				this->emulNet->ENsend(
					&(this->memberNode->addr),
					&(this->hasMyReplicas[1].nodeAddress),
					createMsg.toString());
			}
			else if (oldType == ReplicaType::SECONDARY)
			{
				// So this node was secondary but is now primary meaning its
				// predecessor failed.
				Message secondaryMsg = Message(
					-1,
					this->memberNode->addr,
					MessageType::UPDATE,
					repItr->first,
					v,
					ReplicaType::SECONDARY);
				if (this->hasMyReplicas[0].nodeAddress ==
					  oldHasMyReplicas[0].nodeAddress)
				{
					// If its first successor hasn't changed then it was a TERTIARY but is
					// now a SECONDARY.
					this->emulNet->ENsend(
						&(this->memberNode->addr),
						&(this->hasMyReplicas[0].nodeAddress),
						secondaryMsg.toString());
				}
				else
				{
					// Otherwise, the original TERTIARY also failed so just issue a
					// create for the now SECONDARY node.
					secondaryMsg.type = MessageType::CREATE;
					this->emulNet->ENsend(
						&(this->memberNode->addr),
						&(this->hasMyReplicas[0].nodeAddress),
						secondaryMsg.toString());
				}

				// And as we are assuming no rejoins / new nodes, the now 2nd successor
				// of this node has never seen the key (as this node was SECONDARY but
			  // is now PRIMARY). So we just create the third replica.
				Message createMsg = Message(
					-1,
					this->memberNode->addr,
					MessageType::CREATE,
					repItr->first,
					v,
					ReplicaType::TERTIARY);
				this->emulNet->ENsend(
					&(this->memberNode->addr),
					&(this->hasMyReplicas[0].nodeAddress),
					createMsg.toString());
			}
			else
			{
				// This node was already the PRIMARY, so we just need to check if
				// either its SECONDARY or TERTIARY failed.
				// So there are 3 cases:
				// 1. My SECONDARY failed and my old TERTIARY is now my SECONDARY.
				if (this->hasMyReplicas[0].nodeAddress ==
					  oldHasMyReplicas[1].nodeAddress)
				{
					// So update the successor to now have the SECONDARY.
					Message secondaryMsg = Message(
						-1,
						this->memberNode->addr,
						MessageType::UPDATE,
						repItr->first,
						v,
						ReplicaType::SECONDARY);
					this->emulNet->ENsend(
						&(this->memberNode->addr),
						&(this->hasMyReplicas[0].nodeAddress),
						secondaryMsg.toString());

					// And my now second successor has not seen the key before.
					Message tertiaryMsg = Message(
						-1,
						this->memberNode->addr,
						MessageType::CREATE,
						repItr->first,
						v,
						ReplicaType::TERTIARY);
					this->emulNet->ENsend(
						&(this->memberNode->addr),
						&(this->hasMyReplicas[1].nodeAddress),
						tertiaryMsg.toString());
				}
				// 2. My successor has not changed meaning it has my secondary.
				else if (this->hasMyReplicas[0].nodeAddress ==
					       oldHasMyReplicas[0].nodeAddress)
				{
					// If my old second successor failed I need to create the key at my
					// current second successor.
					if (!(this->hasMyReplicas[1].nodeAddress ==
						    oldHasMyReplicas[1].nodeAddress))
					{
						Message tertiaryMsg = Message(
							-1,
							this->memberNode->addr,
							MessageType::CREATE,
							repItr->first,
							v,
							ReplicaType::TERTIARY);
						this->emulNet->ENsend(
							&(this->memberNode->addr),
							&(this->hasMyReplicas[1].nodeAddress),
							tertiaryMsg.toString());
					}
					// Otherwise, both are intact so do nothing.
				}
				// 3. My now successor is neither my old successor nor old secondary
				// successor, meaning both failed. So create both messages.
				else
				{
					Message secondaryMsg = Message(
						-1,
						this->memberNode->addr,
						MessageType::CREATE,
						repItr->first,
						v,
						ReplicaType::SECONDARY);
					this->emulNet->ENsend(
						&(this->memberNode->addr),
						&(this->hasMyReplicas[0].nodeAddress),
						secondaryMsg.toString());

					Message tertiaryMsg = Message(
						-1,
						this->memberNode->addr,
						MessageType::CREATE,
						repItr->first,
						v,
						ReplicaType::TERTIARY);
					this->emulNet->ENsend(
						&(this->memberNode->addr),
						&(this->hasMyReplicas[1].nodeAddress),
						tertiaryMsg.toString());	
				}
			}
		}
	}
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

  // We only do logging and sending a reply to the coordinator if it is a
	// transaction. Transaction ID -1 is used for handling re-replication.
	if (msg.transID >= 0)
	{
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

	  this->sendReplyToCoordinator(msg, created);
	}
}

/**
 * FUNCTION NAME: handleReadMessage
 *
 * DESCRIPTION: Performs the server-side handling of a read message.
 *              That is, it
 *              1) Tries to read the value for the requested key
 *              2) Logs whether the read was successful
 *              3) Sends a read reply back to the coordinator
 */
void MP2Node::handleReadMessage(Message& msg)
{
	string val = this->readKey(msg.key);

	if (val.compare("") == 0)
	{
		// If the value is the empty string, the read failed.
		this->log->logReadFail(
			&(this->memberNode->addr),
			false,
			msg.transID,
			msg.key);
	}
	else
	{
		// Otherwise the read succeeded.
		this->log->logReadSuccess(
			&(this->memberNode->addr),
			false,
			msg.transID,
			msg.key,
			val);
	}

	// Send reply to the coordinator
	Message replyMsg = Message(
		msg.transID,
		this->memberNode->addr,
		val);
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

	// We only do logging and sending a reply to the coordinator if it is a
	// transaction. Transaction ID -1 is used for handling re-replication.
	if (msg.transID >= 0)
	{
	  // Log success or failure.
	  if (deleted)
	  {
		  this->log->logDeleteSuccess(
			  &(this->memberNode->addr),
			  false,
			  msg.transID,
			  msg.key);
	  }
	  else
	  {
		  this->log->logDeleteFail(
			  &(this->memberNode->addr),
			  false,
			  msg.transID,
			  msg.key);
	  }

	  this->sendReplyToCoordinator(msg, deleted);
	}
}

/**
 * FUNCTION NAME: handleUpdateMessage
 *
 * DESCRIPTION: Performs the server-side handling of an update message
 *              That is, it:
 *              1) Updates the key-value pair on the server side
 *              2) Logs whether the update was successful
 *              3) Sends a reply back to the client node that initiated the
 *                 update.
 */
void MP2Node::handleUpdateMessage(Message& msg)
{
	bool updated = this->updateKeyValue(msg.key, msg.value, msg.replica);

	// We only do logging and sending a reply to the coordinator if it is a
	// transaction. Transaction ID -1 is used for handling re-replication.
	if (msg.transID >= 0)
	{
	  // Log success or failure.
	  if (updated)
	  {
		  this->log->logUpdateSuccess(
			  &(this->memberNode->addr),
			  false,
			  msg.transID,
			  msg.key,
		    msg.value);
	  }
	  else
	  {
		  this->log->logUpdateFail(
			  &(this->memberNode->addr),
			  false,
			  msg.transID,
			  msg.key,
		    msg.value);
	  }

	  this->sendReplyToCoordinator(msg, updated);
	}
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
	auto txnPointer = this->pendingWrites.find(msg.transID);
	if (txnPointer == this->pendingWrites.end())
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
		logWriteSuccess(msg.transID);
	}
	else if (txnPointer->second.hasTransactionFailed())
	{
		logWriteFailure(msg.transID);
	}

	if (txnPointer->second.allRepliesReceived())
	{
		this->pendingWrites.erase(msg.transID);
	}
}

/**
 * FUNCTION NAME: handleReadReplyMessage
 *
 * DESCRIPTION: Handles receipt of a read reply message.
 *              That is, it
 *              1) Checks the reply is for a pending read transaction.
 *              2) Updates the set of received values for the transaction
 *                 based on the message.
 *              3) Checks if a quorum has been reached for that value and logs
 *                 success or failure if so.
 *              4) Removes the transaction and logs failure if no quorum could
 *                 be reached.
 */
void MP2Node::handleReadReplyMessage(Message& msg)
{
	auto readTxnPointer = this->pendingReads.find(msg.transID);
	if (readTxnPointer == this->pendingReads.end())
	{
		// This is not a pending transaction, so discard the message.
		return;
	}

	readTxnPointer->second.recordReplicaValue(msg.value);
	if (readTxnPointer->second.hasReachedQuorum(msg.value))
	{
		if (msg.value.compare("") == 0)
		{
			// A quorum of nodes did not find the key.
			this->log->logReadFail(
				&(this->memberNode->addr),
				true,
				msg.transID,
				readTxnPointer->second.getKey());
		}
		else
		{
			// Otherwise a quorum of nodes returned the same value.
			this->log->logReadSuccess(
				&(this->memberNode->addr),
				true,
				msg.transID,
				readTxnPointer->second.getKey(),
				msg.value);
		}
	}

	if (readTxnPointer->second.allRepliesReceived())
	{
		// If we did not receive a quorum for any value, then log read failure.
		if (!readTxnPointer->second.hasReachedQuorum())
		{
			this->log->logReadFail(
				&(this->memberNode->addr),
				true,
				msg.transID,
				readTxnPointer->second.getKey());
		}

		// We have received all replies so this transaction is no longer pending.
		this->pendingReads.erase(msg.transID);
	}
}

/*
 * FUNCTION NAME: logWriteSuccess
 *
 * DESCRIPTION: Logs the successful completion of a write transaction from the
 *              coordinator node.
 */
void MP2Node::logWriteSuccess(int transId)
{
	auto txnPointer = this->pendingWrites.find(transId);
	if (txnPointer == this->pendingWrites.end()) {
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
 * FUNCTION NAME: logWriteFailure
 *
 * DESCRIPTION: Logs the unsuccessful completion of a write transaction from the
 *              coordinator node.
 */
void MP2Node::logWriteFailure(int transId)
{
	auto txnPointer = this->pendingWrites.find(transId);
	if (txnPointer == this->pendingWrites.end()) {
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

/*
 * FUNCTION NAME: getTransactionId
 *
 * DESCRIPTION: Retrieves a transaction ID for the transaction.
 */
int MP2Node::getTransactionId()
{
	int transId = g_transID;
	g_transID++;
	return transId;
}

/*
 * FUNCTION NAME: sendReplyToCoordinator
 *
 * DESCRIPTION: Sends a reply message to the coordinator. `coordMsg` is the
 *              `Message` from the coordinator that initiated the operation and
 *              `operationSucceeded` is a boolean representing whether the
 *              operation succeeded on the node.
 */
void MP2Node::sendReplyToCoordinator(Message& coordMsg, bool operationSucceeded)
{
	// Construct the reply message.
	Message replyMsg = Message(
		coordMsg.transID,
		this->memberNode->addr,
		MessageType::REPLY,
		operationSucceeded);

	// Send the reply to the coordinator
	this->emulNet->ENsend(
		&(this->memberNode->addr),
		&(coordMsg.fromAddr),
		replyMsg.toString());
}

/**
 * FUNCTION NAME: removeExpiredTransactions
 *
 * DESCRIPTION: removes any pending read or write transactions that have expired
 *              (have timed out).
 *              Logs a failure for any removed transactions that could not reach
 *              a quorum before the timeout.
 */
void MP2Node::removeExpiredTransactions()
{
	int currTime = par->getcurrtime();

	std::unordered_map<int, ReadTransactionState> pendingReads;
	// Start with the read transactions.
	for (auto readTxnPointer = this->pendingReads.begin();
       readTxnPointer != this->pendingReads.end();
		   readTxnPointer++)
	{
		if (readTxnPointer->second.hasTransactionExpired(currTime))
		{
		  // Any reads here are pending, meaning not all replies were received.
		  // However, they may have already reached a quorum even without all replies.
		  if (!readTxnPointer->second.hasReachedQuorum())
		  {
			  // If it hasn't reached a quorum then it timed out and failed.
			  this->log->logReadFail(
				  &(this->memberNode->addr),
				  true,
				  readTxnPointer->first,
				  readTxnPointer->second.getKey());
		  }
		}
		else
		{
			pendingReads.insert({readTxnPointer->first, readTxnPointer->second});
		}
	}
	this->pendingReads = pendingReads;

  std::unordered_map<int, WriteTransactionState> pendingWrites;
	// Next do write transactions.
	for (auto writeTxnPointer = this->pendingWrites.begin();
       writeTxnPointer != this->pendingWrites.end();
		   writeTxnPointer++)
	{
		if (writeTxnPointer->second.hasTransactionExpired(currTime))
		{
		  // Any writes here are pending, meaning not all replies were received.
		  // However we could have already received a quorum of successes or failures.
      if (!writeTxnPointer->second.hasTransactionSucceeded() &&
		      !writeTxnPointer->second.hasTransactionFailed())
		  {
			  switch (writeTxnPointer->second.getTransactionType())
			  {
				  case TransactionType::T_CREATE:
				    this->log->logCreateFail(
						  &(this->memberNode->addr),
						  true,
						  writeTxnPointer->first,
						  writeTxnPointer->second.getKey(),
						  writeTxnPointer->second.getValue());
					  break;
				  case TransactionType::T_UPDATE:
				    this->log->logUpdateFail(
						  &(this->memberNode->addr),
						  true,
						  writeTxnPointer->first,
						  writeTxnPointer->second.getKey(),
						  writeTxnPointer->second.getValue());
					  break;
				  case TransactionType::T_DELETE:
				    this->log->logDeleteFail(
						  &(this->memberNode->addr),
						  true,
						  writeTxnPointer->first,
						  writeTxnPointer->second.getKey());
					  break;
				  default:
				    std::cout << "Invalid write transaction type" << std::endl;
					  exit(1);
			  }
			}
		}
		else
		{
			pendingWrites.insert({writeTxnPointer->first, writeTxnPointer->second});
		}
	}
	this->pendingWrites = pendingWrites;
}

/**
 * FUNCTION NAME: initializeNeighbourhood
 *
 * Description: Initializes the neighbourhood of the node, consisting of its
 *              2 predecessors and 2 successors on the ring.
 *              That is, it sets the hasMyReplicas and haveReplicasOf vectors.
 */
void MP2Node::initializeNeighbourhood()
{
	// First find my position on ring.
	int myPos = -1;
	for (int idx = 0; idx < this->ring.size(); idx++)
	{
		if (this->ring[idx].nodeAddress == this->memberNode->addr)
		{
			myPos = idx;
			break;
		}
	}

	if (myPos == -1)
	{
		std::cout << "Error: cannot find self" << std::endl;
		exit(1);
	}

	// Set the predecessors and successors.
	this->setNeighbourhood(myPos);

	return;
}

/**
 * FUNCTION NAME: setNeighbourhood
 *
 * DESCRIPTION: Sets the 2 successors and 2 predecessors of the node based on
 *              the nodes position in the ring, given by `myPosOnRing`.
 */
void MP2Node::setNeighbourhood(int myPosOnRing)
{
	// Set the 2 predecessors.
	this->haveReplicasOf = std::vector<Node>();
	for (int currPos = myPosOnRing - 2; currPos < myPosOnRing; currPos++)
	{
		this->haveReplicasOf.emplace_back(
			this->ring.at((currPos % this->ring.size())));
	}

	// Then set the 2 successors.
	this->hasMyReplicas = std::vector<Node>();
	for (int currPos = myPosOnRing; currPos < myPosOnRing + 2; currPos++)
	{
		this->hasMyReplicas.emplace_back(
			this->ring.at(((currPos + 1) % this->ring.size())));
	}

	return;
}
