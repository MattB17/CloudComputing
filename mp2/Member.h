/**********************************
 * FILE NAME: Member.h
 *
 * DESCRIPTION: Definition of all Member related class
 **********************************/

#ifndef MEMBER_H_
#define MEMBER_H_

#include "stdincludes.h"
#include "Address.h"
#include "Queue.h"

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
