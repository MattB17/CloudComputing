/**********************************
 * FILE NAME: Log.h
 *
 * DESCRIPTION: Header file of Log class
 **********************************/

#ifndef _LOG_H_
#define _LOG_H_

#include "stdincludes.h"
#include "Params.h"
#include "Address.h"
#include "Member.h"

/*
 * Macros
 */
// number of writes after which to flush file
#define MAXWRITES 1
#define MAGIC_NUMBER "CS425"
#define DBG_LOG "dbg.log"
#define STATS_LOG "stats.log"

/**
 * CLASS NAME: Log
 *
 * DESCRIPTION: Functions to log messages in a debug log
 */
class Log{
private:
	std::shared_ptr<Params> par;
	bool firstTime;
	bool debugMode;

public:
	Log(std::shared_ptr<Params> p, bool debug);
	Log(const Log &anotherLog);
	Log& operator = (const Log &anotherLog);
	virtual ~Log();

	// Generic logging method.
	void logDebug(Address *, const char * str);
	void unconditionalLog(Address *, const char * str, ...);

	// Failure detection logging
	void logNodeAdd(Address *, Address *);
	void logNodeRemove(Address *, Address *);

	// successful transaction logging
	void logCreateSuccess(Address * address, bool isCoordinator, int transID, string key, string value);
	void logReadSuccess(Address * address, bool isCoordinator, int transID, string key, string value);
	void logUpdateSuccess(Address * address, bool isCoordinator, int transID, string key, string newValue);
	void logDeleteSuccess(Address * address, bool isCoordinator, int transID, string key);

	// transaction failure logging
	void logCreateFail(Address * address, bool isCoordinator, int transID, string key, string value);
	void logReadFail(Address * address, bool isCoordinator, int transID, string key);
	void logUpdateFail(Address * address, bool isCoordinator, int transID, string key, string newValue);
	void logDeleteFail(Address * address, bool isCoordinator, int transID, string key);
};

#endif /* _LOG_H_ */
