/**********************************
 * FILE NAME: TransactionState.cpp
 *
 * DESCRIPTION: Implements objects used
 *              to maintain state for
 *              ongoing transactions
 **********************************/

#include "TransactionState.h"

const short TransactionState::timeout = 10;

/**
 * FUNCTION NAME: hasTransactionExpired
 *
 * DESCRIPTION: indicates whether the transaction has expired.
 */
bool TransactionState::hasTransactionExpired(int currTime)
{
 	return (currTime - this->startTime) > TransactionState::timeout;
}

/**
 * CONSTRUCTOR
 *
 * Used in the case the transaction is a create or update
 */
WriteTransactionState::WriteTransactionState(std::string k,
                                             std::string v,
                                             TransactionType t,
                                             int currTime)
  : TransactionState(k, currTime), value(v), type(t),
 	  successCount(0), failureCount(0) {}

/**
 * CONSTRUCTOR
 *
 * Used in the case the transaction is a delete
 */
WriteTransactionState::WriteTransactionState(string k, int currTime)
  : TransactionState(k, currTime), value(""), type(TransactionType::T_DELETE),
 	  successCount(0), failureCount(0) {}

/**
 * CONSTRUCTOR
 */
ReadTransactionState::ReadTransactionState(std::string k, int currTime)
  : TransactionState(k, currTime) {}

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
