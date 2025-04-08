# Cloud Computing: Programming Assignment 1
In this programming assignment we implement a failure detection protocol.

The idea of failure detection is that we have a set of distributed notes in an asynchronous system, and want to be able to detect when a node has failed.

The simplest algorithm for this problem is all-to-all heartbeating where a node periodically sends a heartbeat to every other node it knows of. In this setting if node `i` has not received a heartbeat from node `j` for a certain period of time, `i` marks `j` as failed.

The problem with all-to-all heartbeating is that it requires a lot of messages sent over the network - `O(N^2)` every heartbeat interval.

An alternative to this is the [gossip protocol](). In this protocol, a node periodically sends its entire membership table (all the nodes it knows of and their latest heartbeats) to a random subset of its neighbours. Upon receiving a membership table, a node updates an entry's heartbeat in the table if the received heartbeat is greater or equal to whatever is already in the table. Each node's membership table has a timestamp in each row, indicating time that the entry was last updated. Entries not updated within a timeout window are deemed as failed.

My solution to this programming assignment implements the gossip protocol.

**NOTE:** While this code is written in C++, it is really a mix of old C++ and C. This is because we are only allowed to modify `MP1Node.{h, cpp}`. This is also why the files are quite large. In a custom implementation, it would be refactored into a more modular form and would use modern C++ functionality, such as smart pointers.

### Building
There are a set of test scenarios in the `testcases` folder that simulate different node failures / message drops.

To build the project run:
* `make clean`
* `make`

This will create a binary called `Application` which we can then run a testcase as:
* `./Application testcases/<testcase-file>`
* where `<testcase-file>` is one of the files in the `testcases` folder

To run the Coursera grader and see the performance of all tests cases execute the following:
* `python grader.sh`
* note the warnings are in the `Log` files which students of the class do not modify
