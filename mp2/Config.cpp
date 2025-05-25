/**********************************
 * FILE NAME: Config.cpp
 *
 * DESCRIPTION: Contains high-level configuration
 *              for the class.
 **********************************/

#include "Config.h"

// KV Store Configuration Variables
const short Config::ringSize = 512;  // 2^9

// Logging Configuration Variables
const int Config::maxWrites = 1;
const std::string Config::magicNumber = "CS425";
const std::string Config::debugLog = "dbg.log";
const std::string Config::statsLog = "stats.log";
