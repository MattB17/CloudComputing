/**********************************
 * FILE NAME: Member.h
 *
 * DESCRIPTION: Definition of all Member related class
 **********************************/

#ifndef MEMBER_H_
#define MEMBER_H_

#include "stdincludes.h"

/**
 * CLASS NAME: q_elt
 *
 * DESCRIPTION: Entry in the queue
 */
class q_elt {
public:
	void *elt;
	int size;
	q_elt(void *elt, int size);
};

/**
 * CLASS NAME: Address
 *
 * DESCRIPTION: Class representing the address of a single node
 */
class Address {
public:
	char addr[6];
	Address() {}
	// Copy constructor
	Address(const Address &anotherAddress);
	 // Overloaded = operator
	Address& operator =(const Address &anotherAddress);
	bool operator ==(const Address &anotherAddress);
	bool operator !=(const Address &anotherAddress);
	Address(string address) {
		size_t pos = address.find(":");
		int id = stoi(address.substr(0, pos));
		short port = (short)stoi(address.substr(pos + 1, address.size()-pos-1));
		memcpy(&addr[0], &id, sizeof(int));
		memcpy(&addr[4], &port, sizeof(short));
	}
	string getAddress() {
		int id = 0;
		short port;
		memcpy(&id, &addr[0], sizeof(int));
		memcpy(&port, &addr[4], sizeof(short));
		return to_string(id) + ":" + to_string(port);
	}
	void init() {
		memset(&addr, 0, sizeof(addr));
	}
};

/**
 * CLASS NAME: MemberListEntry
 *
 * DESCRIPTION: Entry in the membership list
 */
class MemberListEntry {
public:
	int id;
	short port;
	long heartbeat;
	long timestamp;
	MemberListEntry(int id, short port, long heartbeat, long timestamp);
	MemberListEntry(int id, short port);
	MemberListEntry(): id(0), port(0), heartbeat(0), timestamp(0) {}
	MemberListEntry(const MemberListEntry &anotherMLE);
	MemberListEntry& operator =(const MemberListEntry &anotherMLE);
	int getid() { return id; }
	short getport() { return port; }
	long getheartbeat() { return heartbeat; }
	long gettimestamp() { return timestamp; }
	void setid(int id) { this->id = id; }
	void setport(short port) { this->port = port; }
	void setheartbeat(long heartbeat) { this->heartbeat = heartbeat; }
	void settimestamp(long timestamp) { this->timestamp = timestamp; }
};

/**
 * CLASS NAME: Member
 *
 * DESCRIPTION: Class representing a member in the distributed system
 */
// Declaration and definition here
class Member {
public:
	Address addr; // my address
	bool inited; // indicates if I have been initialized
	bool inGroup; // indicates if i'm in group
	bool failed; // indicates if member has failed.
	int numNeighbours; // number of neighbours
	long heartbeat; // my heartbeat
	int pingCounter; // counter for next ping
	vector<MemberListEntry> memberList; // Membership table
	queue<q_elt> mp1q; // Queue for failure detection messages
	queue<q_elt> mp2q; // Queue for KVstore messages
	/**
	 * Constructor
	 */
	Member();
	// copy constructor
	Member(const Member &anotherMember);
	// Assignment operator overloading
	Member& operator =(const Member &anotherMember);
	// Move Constructor
	Member(Member &&anotherMember);
	// Virtual destructor.
	// The destructor does nothing but the virtual ensures it is cleaned up
	// before any child classes.
	virtual ~Member() {}
};

#endif /* MEMBER_H_ */
