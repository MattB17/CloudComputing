/**********************************
 * FILE NAME: Address.cpp
 *
 * DESCRIPTION: Definition of all Address related class
 **********************************/

#include "Address.h"

/**
 * Constructor from string.
 *
 * The `address` string is in the form `id:port`.
 */
Address::Address(string address) {
  size_t pos = address.find(":");
  int id = stoi(address.substr(0, pos));
  short port = (short)stoi(address.substr(pos + 1, address.size()-pos-1));
  memcpy(&addr[0], &id, sizeof(int));
  memcpy(&addr[4], &port, sizeof(short));
}

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
 * FUNCTION NAME: getAddress
 *
 * DESCRIPTION: Returns a string representing the address.
 */
string Address::getAddress() {
  int id = 0;
  short port;
  memcpy(&id, &addr[0], sizeof(int));
  memcpy(&port, &addr[4], sizeof(short));
  return to_string(id) + ":" + to_string(port);
}

/**
 * FUNCTION NAME: init
 *
 * DESCRIPTION: Initializes the address to a default 0 address.
 */
void Address::init() {
  memset(&addr, 0, sizeof(addr));
}

/**
 * FUNCTION NAME: addressFromIdAndPort
 *
 * DESCRIPTION: builds Address `addr` from the supplied `id` and `port`.
 */
Address AddressHandler::addressFromIdAndPort(int id, short port)
{
  Address addr;
	*(int *)(addr.addr) = id;
	*(short *)(&addr.addr[4]) = port;
	return addr;
}

/**
 * FUNCTION NAME: idFromAddress
 *
 * DESCRIPTION: extracts the id portion of `addr`.
 */
int AddressHandler::idFromAddress(const Address& addr)
{
	return *(int *)(addr.addr);
}

/**
 * FUNCTION NAME: portFromAddress
 *
 * DESCRIPTION: extracts the port portion of `addr`.
 */
short AddressHandler::portFromAddress(const Address& addr)
{
	return *(short *)(&addr.addr[4]);
}
