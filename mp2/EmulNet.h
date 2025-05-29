/**********************************
 * FILE NAME: EmulNet.h
 *
 * DESCRIPTION: Emulated Network classes header file
 **********************************/

#ifndef _EMULNET_H_
#define _EMULNET_H_

#include "stdincludes.h"
#include "Config.h"
#include "Address.h"
#include "Params.h"
#include "Member.h"

using namespace std;

/**
 * Struct Name: en_msg
 */
typedef struct en_msg {
	// Number of bytes after the class
	int size;
	// Source node
	Address from;
	// Destination node
	Address to;
} en_msg;

/**
 * Class Name: EM
 */
class EM {
public:
	int nextid;
	int currbuffsize;
	int firsteltindex;
	en_msg* buff[Config::enBuffSize];
	EM() {}
	EM& operator = (EM &anotherEM) {
		this->nextid = anotherEM.getNextId();
		this->currbuffsize = anotherEM.getCurrBuffSize();
		this->firsteltindex = anotherEM.getFirstEltIndex();
		int i = this->currbuffsize;
		while (i > 0) {
			this->buff[i] = anotherEM.buff[i];
			i--;
		}
		return *this;
	}
	int getNextId() {
		return nextid;
	}
	int getCurrBuffSize() {
		return currbuffsize;
	}
	int getFirstEltIndex() {
		return firsteltindex;
	}
	void setNextId(int nextid) {
		this->nextid = nextid;
	}
	void settCurrBuffSize(int currbuffsize) {
		this->currbuffsize = currbuffsize;
	}
	void setFirstEltIndex(int firsteltindex) {
		this->firsteltindex = firsteltindex;
	}
	virtual ~EM() {}
};

/**
 * CLASS NAME: EmulNet
 *
 * DESCRIPTION: This class defines an emulated network
 */
class EmulNet
{
private:
	std::shared_ptr<Params> par;
	int sent_msgs[Config::maxNodes + 1][Config::maxTime];
	int recv_msgs[Config::maxNodes + 1][Config::maxTime];
	int enInited;
	EM emulnet;
public:
 	EmulNet(std::shared_ptr<Params> p);
 	EmulNet(EmulNet &anotherEmulNet);
 	EmulNet& operator = (EmulNet &anotherEmulNet);
 	virtual ~EmulNet();
	Address ENinit();
	int ENsend(const Address& myaddr, const Address& toaddr, std::string data);
	int ENsend(const Address& myaddr,
		         const Address& toaddr,
						 char* data,
						 int size);
	int ENrecv(const Address& myaddr,
		         int (* enq)(void *, char *, int),
						 struct timeval *t,
						 int times,
						 void *queue);
	int ENcleanup();
};

#endif /* _EMULNET_H_ */
