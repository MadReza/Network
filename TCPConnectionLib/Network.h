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
	Network(SOCKET socket);
	~Network();

	bool readFile(FILE *f);
	bool sendFile(FILE *f);
private:
	SOCKET sock;
	static const int BUFF_LENGTH = 1024;
	char hostname[NI_MAXHOST];

	void startWinSock();
	void GetLocalHostInfo();

	bool readLong(long *value);
	bool readData(void *buf, int buflen);

	bool sendLong(long value);
	bool sendData(void *buf, int buflen);
};

