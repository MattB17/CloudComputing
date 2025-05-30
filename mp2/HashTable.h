/**********************************
 * FILE NAME: HashTable.h
 *
 * DESCRIPTION: Header file HashTable class
 **********************************/

#ifndef HASHTABLE_H_
#define HASHTABLE_H_

/**
 * Header files
 */
#include "stdincludes.h"
#include "Entry.h"

/**
 * CLASS NAME: HashTable
 *
 * DESCRIPTION: This class is a wrapper to the map provided by C++ STL.
 *
 */
class HashTable {
private:
	std::map<std::string, std::string> hashTable;
public:
	HashTable();
	bool create(std::string& key, std::string& value);
	string read(const std::string& key);
	bool update(const std::string& key, string newValue);
	bool deleteKey(const std::string& key);
	bool isEmpty();
	unsigned long currentSize();
	void clear();
	unsigned long count(const std::string& key);
	virtual ~HashTable();
};

#endif /* HASHTABLE_H_ */
