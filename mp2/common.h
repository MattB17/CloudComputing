#ifndef COMMON_H_
#define COMMON_H_

/**
 * Global variable
 */
// Transaction Id
static int g_transID = 0;

enum MembershipMessageType
{
  JOINREQ,  // Join request
  JOINREP,  // Join reply
  GOSSIP,  // Gossip Message
  UNKNOWN
};

// message types, reply is the message from node to coordinator
enum KVMessageType
{
  CREATE,
  READ,
  UPDATE,
  DELETE,
  WRITE_REPLY,  // Write reply from node to coordinator
  READ_REPLY // Read reply from node to coordinator
};

// enum of replica types
enum ReplicaType
{
  PRIMARY,
  SECONDARY,
  TERTIARY
};

#endif
