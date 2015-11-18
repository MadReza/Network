#include "Network.h"

void Network::startWinSock()
{
	WSADATA wsadata;
	if (WSAStartup(0x0202, &wsadata) != 0){
		cout << "Error in starting WSAStartup()" << endl;	//TODO What To do when Scoket doesn't open ?
	}
	else {
		//"WSAStartup was successful\n";
		//WriteFile(test, buffer, sizeof(buffer), &dwtest, NULL);	//TODO: Write To a File for error
	}
}

//TODO MAKE THIS RETURN THE localhost Info &&&& Remove Junk
char* Network::getLocalHostInfo()
{
	//Display name of local host.
	char *localHost = new char[21];
	HOSTENT *hp;	//TODO usless ?

	gethostname(localHost, 20);
	cout << "Localhost: " << localHost << endl;	//TODO remove after testing

	if ((hp = gethostbyname(localHost)) == NULL)
		throw "gethostbyname failed\n";	//TODO manage it better

	return localHost;
}

HOSTENT* Network::hostExists(char name[])
{
	rp = gethostbyname(name);
	if (rp == NULL)
	{
		return NULL;
	}
	return rp;
}

void Network::createSocket(int sockType)
{
	//Create the socket
	if ((sock = socket(AF_INET, sockType, 0)) == INVALID_SOCKET)
		throw "Socket failed\n";
	/* For UDP protocol replace SOCK_STREAM with SOCK_DGRAM */

	//Specify server address for client to connect to server.
	memset(&sa_in, 0, sizeof(sa_in));
	memcpy(&sa_in.sin_addr, rp->h_addr, rp->h_length);
	sa_in.sin_family = rp->h_addrtype;
	sa_in.sin_port = htons(port);
}

void Network::connectToServer()
{
	//Connect Client to the server
	if (connect(sock, (LPSOCKADDR)&sa_in, sizeof(sa_in)) == SOCKET_ERROR)
		throw "connect failed\n";
}

void Network::closeSocket()
{
	closesocket(sock);
}

bool Network::sendData(void *buf, int buflen)
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

bool Network::sendLong(long value)
{
	value = htonl(value);
	return sendData(&value, sizeof(value));
}

bool Network::sendFile(FILE *f)
{
	fseek(f, 0, SEEK_END);
	long filesize = ftell(f);
	rewind(f);
	if (filesize == EOF)
		return false;
	if (!sendLong(filesize))
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
			if (!sendData(buffer, num))
				return false;
			filesize -= num;
		} while (filesize > 0);
	}
	return true;
}


bool Network::readData(void *buf, int buflen)
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

bool Network::readLong(long *value)
{
	if (!readData(value, sizeof(value)))
		return false;
	*value = ntohl(*value);
	return true;
}

bool Network::readFile(FILE *f)
{
	long filesize;
	if (!readLong(&filesize))
		return false;
	if (filesize > 0)
	{
		char buffer[1024];
		do
		{
			int num = min(filesize, sizeof(buffer));
			if (!readData(buffer, num))
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

int Network::sendMsg(char msg[])
{
	int buffSizeToSend = strlen(msg);	//TCP doesn't send all the bytes all the time.
	char *pbuf = msg;
	int iResult(-1);

	while (buffSizeToSend > 0)
	{
		iResult = send(sock, pbuf, buffSizeToSend, 0);
		if (iResult == SOCKET_ERROR)
		{
			if (WSAGetLastError() == WSAEWOULDBLOCK)	//Buffer is full. Not a real error so SKIP
			{
				continue;
			}
			return iResult;
		}
		pbuf += iResult;	//Move forward to the part that didn't go through
		buffSizeToSend -= iResult;
	}

	return iResult;	//0 for finished good.
}

SOCKET Network::getSocket()
{
	return sock;
}