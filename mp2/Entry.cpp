/**********************************
 * FILE NAME: Application.h
 *
 * DESCRIPTION: Entry class definition
 **********************************/
#include "Entry.h"

/**
 * constructor
 */
Entry::Entry(string _value, int _timestamp, ReplicaType _replica)
  : value(_value), timestamp(_timestamp), replica(_replica), delimiter(":") {}

/**
 * constructor
 *
 * DESCRIPTION: Convert string to get an Entry object
 */
Entry::Entry(std::string entry): delimiter(":")
{
	std::vector<string> tuple;
	size_t pos = entry.find(delimiter);
	size_t start = 0;

	while (pos != string::npos)
	{
		string field = entry.substr(start, pos-start);
		tuple.push_back(field);
		start = pos + delimiter.size();
		pos = entry.find(delimiter, start);
	}
	tuple.push_back(entry.substr(start));

	value = tuple.at(0);
	timestamp = stoi(tuple.at(1));
	replica = static_cast<ReplicaType>(stoi(tuple.at(2)));
}

/**
 * FUNCTION NAME: converToString
 *
 * DESCRIPTION: Convert the object to a string representation
 */
string Entry::convertToString() const
{
	return (value + delimiter + to_string(timestamp) +
	        delimiter + to_string(replica));
}
