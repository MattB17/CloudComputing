#**********************
#*
#* Progam Name: MP1. Membership Protocol.
#*
#* Current file: Makefile
#* About this file: Build Script.
#*
#***********************

CFLAGS =  -Wall -g -std=c++14

all: Application

Application: Config.o MP1Node.o EmulNet.o Application.o Log.o Params.o Address.o Member.o MP2Node.o Node.o HashTable.o Entry.o Message.o TransactionState.o
	g++ -o Application Config.o MP1Node.o EmulNet.o Application.o Log.o Params.o Address.o Member.o MP2Node.o Node.o HashTable.o Entry.o Message.o TransactionState.o ${CFLAGS}

MP1Node.o: MP1Node.cpp MP1Node.h Log.h Params.h Address.h Member.h Message.h EmulNet.h Queue.h
	g++ -c MP1Node.cpp ${CFLAGS}

EmulNet.o: EmulNet.cpp EmulNet.h Config.h Params.h Address.h Member.h
	g++ -c EmulNet.cpp ${CFLAGS}

Application.o: Application.cpp Application.h Member.h Log.h Config.h Params.h Address.h Member.h EmulNet.h Queue.h
	g++ -c Application.cpp ${CFLAGS}

Log.o: Log.cpp Log.h Config.h Params.h Address.h Member.h
	g++ -c Log.cpp ${CFLAGS}

Params.o: Params.cpp Params.h
	g++ -c Params.cpp ${CFLAGS}

Address.o: Address.cpp Address.h
	g++ -c Address.cpp ${CFLAGS}

Member.o: Member.cpp Member.h Address.h
	g++ -c Member.cpp ${CFLAGS}

TransactionState.o: TransactionState.cpp TransactionState.h
	g++ -c TransactionState.cpp ${CFLAGS}

MP2Node.o: MP2Node.cpp MP2Node.h EmulNet.h Params.h Address.h Member.h Node.h HashTable.h Log.h Params.h Message.h TransactionState.h
	g++ -c MP2Node.cpp ${CFLAGS}

Node.o: Node.cpp Node.h Address.h Member.h
	g++ -c Node.cpp ${CFLAGS}

HashTable.o: HashTable.cpp HashTable.h Entry.h
	g++ -c HashTable.cpp ${CFLAGS}

Entry.o: Entry.cpp Entry.h Message.h
	g++ -c Entry.cpp ${CFLAGS}

Config.o: Config.cpp Config.h
	g++ -c Config.cpp ${CFLAGS}

Message.o: Message.cpp Message.h Address.h Member.h
	g++ -c Message.cpp ${CFLAGS}

clean:
	rm -rf *.o Application dbg.log msgcount.log stats.log machine.log
