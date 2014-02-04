CXXFLAGS=-std=c++11 -pthread
LDLIBS=-lboost_thread -lboost_system

all: chat_client chat_server

clean:
	-rm -f chat_client chat_server
