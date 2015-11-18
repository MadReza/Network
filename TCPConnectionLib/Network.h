#pragma once

/* send and receive codes between client and server */
/* This is your basic WINSOCK shell */
#pragma comment( linker, "/defaultlib:ws2_32.lib" )
#include <winsock2.h>
#include <ws2tcpip.h>

#include <fstream>
#include <iostream>

using std::cout;
using std::endl;

class Network
{
public:

	bool readFile(FILE *f);
	bool sendFile(FILE *f);

	int sendMsg(char msg[]);	//return result with 0 finished good.
	//int recvMsg(char msg[]); //Maybe ?

	SOCKET getSocket();

	void startWinSock();	//Todo eventually move it out to private.
	void createSocket(int sockType);
	void connectToServer();
	void closeSocket();

	char* getLocalHostInfo();
	HOSTENT* hostExists(char name[]);

private:
	SOCKET sock;
	SOCKADDR_IN sa;         // filled by bind
	SOCKADDR_IN sa_in;      // fill with server info, IP, port
	HOSTENT *rp = NULL;
	//user defined port number
	#define REQUEST_PORT 0x7070;
	int port = REQUEST_PORT;

	static const int BUFF_LENGTH = 1024;
	char hostname[NI_MAXHOST];


	bool readLong(long *value);
	bool readData(void *buf, int buflen);

	bool sendLong(long value);
	bool sendData(void *buf, int buflen);
};

