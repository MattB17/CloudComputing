/**********************************
 * FILE NAME: Config.cpp
 *
 * DESCRIPTION: Contains high-level configuration
 *              for the class.
 **********************************/

#include "Config.h"

// KV Store Configuration Variables
const short Config::ringSize = 512;  // 2^9
const short Config::stabilizeTime = 50;
const short Config::firstFailTime = 25;
const short Config::lastFailTime = 10;
const short Config::numReplicas = 3;
const short Config::numInserts = 100;
const short Config::keyLength = 5;

// Logging Configuration Variables
const int Config::maxWrites = 1;
const std::string Config::magicNumber = "CS425";
const std::string Config::debugLog = "dbg.log";
const std::string Config::statsLog = "stats.log";
