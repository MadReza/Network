#include "Network.h"

/* send and receive codes between client and server */
/* This is your basic WINSOCK shell */
#pragma comment( linker, "/defaultlib:ws2_32.lib" )
#include <winsock2.h>
#include <ws2tcpip.h>

#include <fstream>

Network::Network()
{
}


Network::~Network()
{
}


bool Network::senddata(SOCKET sock, void *buf, int buflen)
{
	char *pbuf = (char *)buf;

	while (buflen > 0)
	{
		int num = send(sock, pbuf, buflen, 0);
		if (num == SOCKET_ERROR)
		{
			if (WSAGetLastError() == WSAEWOULDBLOCK)
			{
				// optional: use select() to check for timeout to fail the send
				continue;
			}
			return false;
		}

		pbuf += num;
		buflen -= num;
	}

	return true;
}

bool Network::sendlong(SOCKET sock, long value)
{
	value = htonl(value);
	return senddata(sock, &value, sizeof(value));
}

bool Network::sendfile(SOCKET sock, FILE *f)
{
	fseek(f, 0, SEEK_END);
	long filesize = ftell(f);
	rewind(f);
	if (filesize == EOF)
		return false;
	if (!sendlong(sock, filesize))
		return false;
	if (filesize > 0)
	{
		char buffer[BUFF_LENGTH];
		do
		{
			size_t num = min(filesize, sizeof(buffer));
			num = fread(buffer, 1, num, f);
			if (num < 1)
				return false;
			if (!senddata(sock, buffer, num))
				return false;
			filesize -= num;
		} while (filesize > 0);
	}
	return true;
}


bool Network::readdata(SOCKET sock, void *buf, int buflen)
{
	char *pbuf = (char *)buf;

	while (buflen > 0)
	{
		int num = recv(sock, pbuf, buflen, 0);
		if (num == SOCKET_ERROR)
		{
			if (WSAGetLastError() == WSAEWOULDBLOCK)
			{
				// optional: use select() to check for timeout to fail the read
				continue;
			}
			return false;
		}
		else if (num == 0)
			return false;

		pbuf += num;
		buflen -= num;
	}

	return true;
}

bool Network::readlong(SOCKET sock, long *value)
{
	if (!readdata(sock, value, sizeof(value)))
		return false;
	*value = ntohl(*value);
	return true;
}

bool Network::readfile(SOCKET sock, FILE *f)
{
	long filesize;
	if (!readlong(sock, &filesize))
		return false;
	if (filesize > 0)
	{
		char buffer[1024];
		do
		{
			int num = min(filesize, sizeof(buffer));
			if (!readdata(sock, buffer, num))
				return false;
			int offset = 0;
			do
			{
				size_t written = fwrite(&buffer[offset], 1, num - offset, f);
				if (written < 1)
					return false;
				offset += written;
			} while (offset < num);
			filesize -= num;
		} while (filesize > 0);
	}
	return true;
}
