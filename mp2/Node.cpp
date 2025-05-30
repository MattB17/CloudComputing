/**********************************
 * FILE NAME: Node.cpp
 *
 * DESCRIPTION: Node class definition
 **********************************/

#include "Node.h"

/**
 * constructor
 */
Node::Node() {}

/**
 * constructor
 */
Node::Node(Address address) {
	this->nodeAddress = address;
	computeHashCode();
}

/**
 * Destructor
 */
Node::~Node() {}

/**
 * FUNCTION NAME: computeHashCode
 *
 * DESCRIPTION: This function computes the hash code of the node address
 */
void Node::computeHashCode() {
	nodeHashCode = hashFunc(nodeAddress.addr) % Config::ringSize;
}

/**
 * copy constructor
 */
Node::Node(const Node& another) {
	this->nodeAddress = another.nodeAddress;
	this->nodeHashCode = another.nodeHashCode;
}

/**
 * Assignment operator overloading
 */
Node& Node::operator=(const Node& another) {
	this->nodeAddress = another.nodeAddress;
	this->nodeHashCode = another.nodeHashCode;
	return *this;
}

/**
 * operator overloading
 *
 * we order nodes by their hashcode: which is the hash of their address
 * modulo the ring size (ie. their position on the ring).
 */
bool Node::operator < (const Node& another) const {
	return this->nodeHashCode < another.nodeHashCode;
}

/**
 * FUNCTION NAME: getHashCode
 *
 * DESCRIPTION: return hash code of the node
 */
size_t Node::getHashCode() {
	return nodeHashCode;
}

/**
 * FUNCTION NAME: getAddress
 *
 * DESCRIPTION: return the address of the node
 */
Address * Node::getAddress() {
	return &nodeAddress;
}

/**
 * FUNCTION NAME: setHashCode
 *
 * DESCRIPTION: set the hash code of the node
 */
void Node::setHashCode(size_t hashCode) {
	this->nodeHashCode = hashCode;
}

/**
 * FUNCTION NAME: setAddress
 *
 * DESCRIPTION: set the address of the node
 */
void Node::setAddress(Address address) {
	this->nodeAddress = address;
}
