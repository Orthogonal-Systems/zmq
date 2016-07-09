CC=g++
CFLAGS=-I../include
DEPS = ../include/datapacket.h

datapacket.o:
	$(CC) -c ../src/datapacket.cpp $(CFLAGS)

%.o: %.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

dp_test: datapacket.o datapacket_test.o
	$(CC) -o dp_test $(CFLAGS) datapacket.o datapacket_test.o
