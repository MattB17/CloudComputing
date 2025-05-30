/**********************************
 * FILE NAME: HashTable.cpp
 *
 * DESCRIPTION: Hash Table class definition
 **********************************/

#include "HashTable.h"

HashTable::HashTable() {}

HashTable::~HashTable() {}

/**
 * FUNCTION NAME: create
 *
 * DESCRIPTION: This function inserts they (key,value) pair into the local hash table
 *
 * It creates the key if it does not exist or overwrites it if it does.
 *
 * RETURNS:
 * true on SUCCESS
 * false in FAILURE
 */
bool HashTable::create(std::string& key, std::string& value)
{
	hashTable.emplace(key, value);
	return true;
}

/**
 * FUNCTION NAME: read
 *
 * DESCRIPTION: This function searches for the key in the hash table
 *
 * RETURNS:
 * string value if found
 * else it returns an empty string
 */
std::string HashTable::read(const std::string& key)
{
	std::map<string, string>::iterator itr = hashTable.find(key);
	if (itr != hashTable.end())
	{
		// Value found
		return itr->second;
	}
	// Value not found
	return "";
}

/**
 * FUNCTION NAME: update
 *
 * DESCRIPTION: This function updates the given key with the updated value passed in
 * 				if the key is found
 *
 *
 * RETURNS:
 * true on SUCCESS (when the key exists and is updated)
 * false on FAILURE (if the key doesn't exist)
 */
bool HashTable::update(const std::string& key, std::string newValue)
{
	std::map<string, string>::iterator itr = hashTable.find(key);

	if (itr == hashTable.end())
	{
		// Key not found
		return false;
	}
	// Key found
	hashTable.at(key) = newValue;
	return true;
}

/**
 * FUNCTION NAME: deleteKey
 *
 * DESCRIPTION: This function deletes the given key and the corresponding value if the key is found
 *
 * RETURNS:
 * true on SUCCESS (if key is found and deleted)
 * false on FAILURE (if key is not found or fail to delete)
 */
bool HashTable::deleteKey(const std::string& key)
{
	uint eraseCount = 0;

	if (read(key).empty())
	{
		// Key not found
		return false;
	}
	eraseCount = hashTable.erase(key);
	if (eraseCount < 1)
	{
		// Could not erase
		return false;
	}
	// Delete was successful
	return true;
}

/**
 * FUNCTION NAME: isEmpty
 *
 * DESCRIPTION: Returns if the hash table is empty
 *
 * RETURNS:
 * true if hash table is empty
 * false otherwise
 */
bool HashTable::isEmpty()
{
	return hashTable.empty();
}

/**
 * FUNCTION NAME: currentSize
 *
 * DESCRIPTION: Returns the current size of the hash table
 *
 * RETURNS:
 * size of the table as unit
 */
unsigned long HashTable::currentSize()
{
	return (unsigned long)(hashTable.size());
}

/**
 * FUNCTION NAME: clear
 *
 * DESCRIPTION: Clear all contents from the hash table
 */
void HashTable::clear()
{
	hashTable.clear();
}

/**
 * FUNCTION NAME: count
 *
 * DESCRIPTION: Returns the count of the number of values for the passed in key
 *
 * RETURNS:
 * unsigned long count (Should be always 1)
 */
unsigned long HashTable::count(const std::string& key)
{
	return (unsigned long)(hashTable.count(key));
}
