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

  // Logging Configuration Variables
  static const int maxWrites;  // number of writes after which to flush file
  static const std::string magicNumber;
  static const std::string debugLog;
  static const std::string statsLog;
};

#endif  // CONFIG_H_
