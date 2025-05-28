/**********************************
 * FILE NAME: Config.h
 *
 * DESCRIPTION: Contains high-level configuration
 *              for the class.
 **********************************/

#ifndef CONFIG_H_
#define CONFIG_H_

#include "stdincludes.h"

class Config {
public:
  // KV Store Configuration Variables
  static const short ringSize;
  static constexpr short totalRunningTime = 700;
  static constexpr short insertTime = totalRunningTime - 600;
  static constexpr short testTime = insertTime + 50;
  static const short stabilizeTime;
  static const short firstFailTime;
  static const short lastFailTime;
  static const short numReplicas;
  static const short numInserts;
  static const short keyLength;

  // Emulation Variables
  static constexpr short maxNodes = 1000;
  static constexpr short maxTime = 3600;
  static constexpr size_t enBuffSize = 30000;

  // Logging Configuration Variables
  static const int maxWrites;  // number of writes after which to flush file
  static const std::string magicNumber;
  static const std::string debugLog;
  static const std::string statsLog;

  // Membership variables
  static const double gossipProportion;
};

#endif  // CONFIG_H_
