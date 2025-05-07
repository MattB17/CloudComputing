# Cloud Computing: Programming Assignment 2
In this programming assignment we implement a distributed key value store.

The idea of the key value store is that processes are arranged sequentially around a ring based on a hash of their id.

Keys are then hashed also around the ring and stored at the first process on or after the hashed value on the ring.
Moreover, the key is replicated at the 2 nodes following as well so that each key has 3 replicas.

The key-value store supports the standard CRUD operations (create, read, update, delete).
When issuing one of these operations a client can contact any node and that node then acts as the coordinator for the operation.
It hashes the key to determine the 3 replicas responsible for storing the key and its corresponding value.
The coordinator then forwards the request to the 3 replicas and waits for a quorum of replies (in this case 2) before responding back to the client.
This is similar to the approach used by other popular key value stores, such as Cassandra.

In addition, the membership protocol from the first programming assignment is used to detect failures.
And when failures are detected each node runs a stabilization protocol to ensure there are exactly 3 replicas for every key for which it is the primary.

**NOTE:** While this code is written in C++, it is really a mix of old C++ and C. This is because we are only allowed to modify `MP1Node.{h, cpp}` and `MP2Node.{h, cpp}`. This is also why the files are quite large. In a custom implementation, it would be refactored into a more modular form and would use modern C++ functionality, such as smart pointers. I am currently in the process of doing this task.

### Building
There are a set of test scenarios in the `testcases` folder for the CRUD operations and with possible failures.

To build the project run:
* `make clean`
* `make`

This will create a binary called `Application` which we can then run a testcase as:
* `./Application testcases/<testcase-file>`
* where `<testcase-file>` is one of the files in the `testcases` folder

To run the Coursera grader and see the performance of all tests cases execute the following:
* `python ./KVStoreGrader.sh`
