/**********************************
 * FILE NAME: Application.h
 *
 * DESCRIPTION: Header file Entry class
 **********************************/

#include "stdincludes.h"
#include "Message.h"

/**
 * CLASS NAME: Entry
 *
 * DESCRIPTION: This class describes the entry for each key in the DHT
 *
 * `delimiter` is used to convert the entry to a string or parse an entry from
 * a string.
 */
class Entry {
public:
	std::string value;
	int timestamp;
	ReplicaType replica;
	const std::string delimiter;

	Entry(std::string entry);
	Entry(std::string _value, int _timestamp, ReplicaType _replica);
	std::string convertToString() const;
};
