/**********************************
 * FILE NAME: Member.cpp
 *
 * DESCRIPTION: Definition of all Member related class
 **********************************/

#include "Member.h"

/**
 * Constructor
 */
q_elt::q_elt(void *elt, int size): elt(elt), size(size) {}

/**
 * Copy constructor
 */
Address::Address(const Address &anotherAddress)
{
	memcpy(&addr, &anotherAddress.addr, sizeof(addr));
}

/**
 * Assignment operator overloading
 */
Address& Address::operator =(const Address& anotherAddress)
{
	memcpy(&addr, &anotherAddress.addr, sizeof(addr));
	return *this;
}

/**
 * Check for equality of two address objects
 */
bool Address::operator ==(const Address& anotherAddress)
{
	return !memcmp(this->addr, anotherAddress.addr, sizeof(this->addr));
}

/**
 * Check for inequality of two address objects
 */
bool Address::operator !=(const Address& anotherAddress)
{
	return memcmp(this->addr, anotherAddress.addr, sizeof(this->addr));
}

/**
 * Constructor
 */
MemberListEntry::MemberListEntry(int id,
	                               short port,
																 long heartbeat,
																 long timestamp)
	: id(id), port(port), heartbeat(heartbeat), timestamp(timestamp) {}

/**
 * Constuctor
 */
MemberListEntry::MemberListEntry(int id, short port): id(id), port(port) {}

/**
 * Copy constructor
 */
MemberListEntry::MemberListEntry(const MemberListEntry &anotherMLE)
{
	this->heartbeat = anotherMLE.heartbeat;
	this->id = anotherMLE.id;
	this->port = anotherMLE.port;
	this->timestamp = anotherMLE.timestamp;
}

/**
 * Assignment operator overloading
 */
MemberListEntry& MemberListEntry::operator =(const MemberListEntry &anotherMLE)
{
	MemberListEntry temp(anotherMLE);
	swap(heartbeat, temp.heartbeat);
	swap(id, temp.id);
	swap(port, temp.port);
	swap(timestamp, temp.timestamp);
	return *this;
}

/**
 * Constructor
 */
Member::Member()
  : inited(false), inGroup(false), failed(false),
	  numNeighbours(0), heartbeat(0), pingCounter(0) {}

/**
 * Copy Constructor
 */
Member::Member(const Member &anotherMember)
{
	this->addr = anotherMember.addr;
	this->inited = anotherMember.inited;
	this->inGroup = anotherMember.inGroup;
	this->failed = anotherMember.failed;
	this->numNeighbours = anotherMember.numNeighbours;
	this->heartbeat = anotherMember.heartbeat;
	this->pingCounter = anotherMember.pingCounter;
	this->memberList = anotherMember.memberList;
	this->mp1q = anotherMember.mp1q;
	this->mp2q = anotherMember.mp2q;
}

/**
 * Move Constructor
 */
Member::Member(Member &&anotherMember)
{
	// Copy values from anotherMember
	this->addr = anotherMember.addr;
	this->inited = anotherMember.inited;
	this->inGroup = anotherMember.inGroup;
	this->failed = anotherMember.failed;
	this->numNeighbours = anotherMember.numNeighbours;
	this->heartbeat = anotherMember.heartbeat;
	this->pingCounter = anotherMember.pingCounter;
	this->memberList = anotherMember.memberList;
	this->mp1q = anotherMember.mp1q;
	this->mp2q = anotherMember.mp2q;

	// Invalidate anotherMember
	anotherMember.addr = Address();
	anotherMember.inited = false;
	anotherMember.inGroup = false;
	anotherMember.failed = false;
	anotherMember.numNeighbours = 0;
	anotherMember.heartbeat = 0;
	anotherMember.pingCounter = 0;
	anotherMember.memberList.clear();
	anotherMember.mp1q = std::queue<q_elt>();
	anotherMember.mp2q = std::queue<q_elt>();
}

/**
 * Assignment operator overloading
 */
Member& Member::operator =(const Member& anotherMember)
{
	this->addr = anotherMember.addr;
	this->inited = anotherMember.inited;
	this->inGroup = anotherMember.inGroup;
	this->failed = anotherMember.failed;
	this->numNeighbours = anotherMember.numNeighbours;
	this->heartbeat = anotherMember.heartbeat;
	this->pingCounter = anotherMember.pingCounter;
	this->memberList = anotherMember.memberList;
	this->mp1q = anotherMember.mp1q;
	this->mp2q = anotherMember.mp2q;
	return *this;
}
