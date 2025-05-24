/**********************************
 * FILE NAME: TransactionState.h
 *
 * DESCRIPTION: Implements objects used
 *              to maintain state for
 *              ongoing transactions
 **********************************/

#ifndef TRANSACTION_STATE_H_
#define TRANSACTION_STATE_H_

#include "stdincludes.h"

// Transaction types
enum TransactionType
{
  T_CREATE,
  T_READ,
  T_UPDATE,
  T_DELETE
};

/**
 * CLASS NAME: TransactionState
 *
 * DESCRIPTION: An abstract base class representing the state of a transaction.
 */
class TransactionState {
protected:
  static const short timeout;

  std::string key;
  int startTime;

public:
  TransactionState(std::string k, int currTime): key(k), startTime(currTime) {}

  std::string getKey() { return this->key; }
  bool hasTransactionExpired(int currTime);

  virtual bool allRepliesReceived() = 0;
};

/*
 * CLASS NAME: WriteTransactionState
 *
 * DESCRIPTION: Used to handle the state of pending write transactions.
 *
 * This class is only intended for creates, updates, or deletes as we do not
 * need to reconcile possibly conflicting values between the replicas. Rather
 * we just need to track whether the operations were successful or not at each
 * replica and monitor when a quorum is reached.
 */
class WriteTransactionState : public TransactionState {
private:
	std::string value;
	TransactionType type;
	short successCount;
	short failureCount;

public:
	// For create or update transactions
	WriteTransactionState(std::string k,
                        std::string v,
                        TransactionType t,
                        int currTime);

  // For delete transactions
	WriteTransactionState(std::string k, int currTime);

	std::string getValue() { return value; }
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
class ReadTransactionState : public TransactionState {
private:
	// Used to record the counts for each value seen among the replicas.
	// ie. if 2 replicas have the value `v` then `valueCounts` should have an
	// entry for `v` with a count of 2.
	unordered_map<string, int> valueCounts;

public:
	ReadTransactionState(string k, int currTime);

	void recordReplicaValue(string v);

  // Indicates whether we have reached a quorum for value `v`.
	bool hasReachedQuorum(string v);

	// Indicates whether a quorum was reached for any value.
	bool hasReachedQuorum();

  // Indicates whether all replies for the transaction have been received.
	bool allRepliesReceived();
};

#endif  // TRANSACTION_STATE_H_
