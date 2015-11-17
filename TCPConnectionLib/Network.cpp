#include "Network.h"

Network::Network(SOCKET socket)
{
	sock = socket;
}


Network::~Network()
{
}


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
void Network::GetLocalHostInfo()
{
	//Display name of local host.
	char localHost[21];

	gethostname(localHost, 20);
	cout << "ftp_tcp starting on host: " << localHost << endl;

	if ((hp = gethostbyname(localhost)) == NULL)
		throw "gethostbyname failed\n";
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
