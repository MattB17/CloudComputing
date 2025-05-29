/**********************************
 * FILE NAME: Address.h
 *
 * DESCRIPTION: Definition of all Address related objects
 **********************************/

#ifndef ADDRESS_H_
#define ADDRESS_H_

#include "stdincludes.h"

/**
 * CLASS NAME: Address
 *
 * DESCRIPTION: Class representing the address of a single node
 */
class Address {
public:
  char addr[6];

  // Constructors
  Address() {}
  Address(string address);

  // Copy constructor
  Address(const Address &anotherAddress);

  // Overloaded = operator
  Address& operator =(const Address &anotherAddress);

  // Equality and inequality
  bool operator ==(const Address &anotherAddress);
  bool operator !=(const Address &anotherAddress);

  string getAddress() const;
  void init();
};

/**
 * CLASS NAME: AddressHandler
 *
 * DESCRIPTION: Handles address low level operations.
 *              Used primarily for mapping from an address to id and port and
 *              vice versa.
 */
class AddressHandler {
public:
  AddressHandler() {}
  Address addressFromIdAndPort(int id, short port);
  int idFromAddress(const Address& addr);
  short portFromAddress(const Address& addr);
};

#endif  // ADDRESS_H_
