#pragma once
#pragma comment( linker, "/defaultlib:ws2_32.lib" )
#include <winsock2.h>
#include <ws2tcpip.h>
#include <process.h>
#include <winsock.h>
#include <iostream>
#include <fstream>
#include <windows.h>

#include <tchar.h> 
#include <strsafe.h>
#include <direct.h>
#define GetCurrentDir _getcwd

using namespace std;

//port data types

#define REQUEST_PORT 0x7070
const int BUFF_LENGTH = 1024;

int port=REQUEST_PORT;
char hostname[NI_MAXHOST];

enum TransferType { GET, PUT, LIST };

//socket data types
SOCKET s;

SOCKET s1;
SOCKADDR_IN sa;      // filled by bind
SOCKADDR_IN sa1;     // fill with server info, IP, port
union {struct sockaddr generic;
	struct sockaddr_in ca_in;}ca;

int calen=sizeof(ca); 

//buffer data types
char szbuffer[BUFF_LENGTH];	//Changed the size of the buffer to include path

char *buffer;
int ibufferlen;
int ibytesrecv;
char fileName[FILENAME_MAX];
int ibytessent;

//host data types
char localhost[21];

HOSTENT *hp;

//wait variables
int nsa1;
int r,infds=1, outfds=0;
struct timeval timeout;
const struct timeval *tp=&timeout;

fd_set readfds;

//others
HANDLE test;
DWORD dwtest;

void GetAllfilesInWorkingDirectory()
{
	WIN32_FIND_DATA ffd;	//Data for each file is stored in this.
	TCHAR szDir[MAX_PATH];
	HANDLE hFind = INVALID_HANDLE_VALUE;

	//Get Current Working Directory
	char cCurrentPath[FILENAME_MAX];
	GetCurrentDir(cCurrentPath, sizeof(cCurrentPath));
		
	// Prepare string for use with FindFile functions.  First, copy the
	// string to a buffer, then append '\*' to the directory name.

	StringCchCopy(szDir, MAX_PATH, cCurrentPath);
	StringCchCat(szDir, MAX_PATH, TEXT("\\*"));

	// Find the first file in the directory.
	hFind = FindFirstFile(szDir, &ffd);

	// List all the files in the directory with some info about them.
	memset(szbuffer, 0, BUFF_LENGTH);
	do
	{
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
		{
			//Ignore Directories.
		}
		else
		{
			strcat_s(szbuffer, "\t");
			strcat_s(szbuffer, ffd.cFileName);
			strcat_s(szbuffer, "\r\n");
		}
	} while (FindNextFile(hFind, &ffd) != 0);
}

//TODO Erase this and use the Network one.
void StartWinSock()
{
	WSADATA wsadata;
	if (WSAStartup(0x0202, &wsadata) != 0){
		cout << "Error in starting WSAStartup()\n";
	}
	else{
		buffer = "WSAStartup was successful\n";
		WriteFile(test, buffer, sizeof(buffer), &dwtest, NULL);
	}
}

void HostName()
{
	//Display info of local host
	gethostname(localhost, 128);		//Changed length to 128 for long host names.
	cout << "ftpd_tcp starting at host: " << localhost << endl;
	cout << "Waiting to be contacted for transferring files..." << endl;

	//Error
	hp = gethostbyname(localhost);
	if (hp == NULL) {
		cout << "gethostbyname() cannot get local host info?"
			<< WSAGetLastError() << endl;
		exit(1);
	}
}

void FillConnectionInfo()
{
	//Fill-in Server Port and Address info.
	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);
	sa.sin_addr.s_addr = htonl(INADDR_ANY);
}

void BindPort()
{
	//Bind the server port
	if (bind(s, (LPSOCKADDR)&sa, sizeof(sa)) == SOCKET_ERROR)
		throw "can't bind the socket";
	//cout << "Bind was successful" << endl;	Reza
}

void CreateSocket()
{
	//Create the server TCP socket
	if ((s = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) // For UDP protocol replace SOCK_STREAM with SOCK_DGRAM 
		throw "can't initialize socket";
}

void ListenRequest()
{
	//Successfull bind, now listen for client requests.

	if (listen(s, 10) == SOCKET_ERROR)
		throw "couldn't  set up listen on socket";
	//else cout << "Listen was successful" << endl;		Reza
}

void ListenForConnections()
{
	FD_SET(s, &readfds);  //always check the listener

	outfds = select(infds, &readfds, NULL, NULL, tp);
	if (!outfds) {}
	else if (outfds == SOCKET_ERROR) throw "failure in Select";
	else if (FD_ISSET(s, &readfds))  cout << "got a connection request" << endl;

	//Found a connection request, try to accept. 
	s1 = accept(s, &ca.generic, &calen);
	if (s1 == INVALID_SOCKET)
		throw "Couldn't accept connection\n";

	//Connection request accepted.
	/*cout<<"accepted connection from "<<inet_ntoa(ca.ca_in.sin_addr)<<":"		REZA
	<<hex<<htons(ca.ca_in.sin_port)<<endl;*/
}

int RecieveData(int size)
{
	memset(szbuffer, 0, BUFF_LENGTH);
	int buffSizeToRecieve = size;
	char *pbuf = szbuffer;
	int iResult = 0;

	while (buffSizeToRecieve > 0 )
	{
		iResult = recv(s1, pbuf, BUFF_LENGTH, 0);		//TODO:: SERVER STUCK HERE !!
		if (iResult == SOCKET_ERROR)
		{
			if (WSAGetLastError() == WSAEWOULDBLOCK)	//Buffer is full. Not a real error so SKIP
			{
				continue;
			}
			return -1;
		}

		if (strcmp(szbuffer, "REZAEND") == 0)
		{
			cout << "REZAEND REACHED " << endl;
			return 0;
		}

		buffSizeToRecieve -= iResult;
	} 

	return 1;	//0 for finished good.
}

bool fileExist(const char *fileName)
{
	std::ifstream infile(fileName);
	return infile.good();
}

int SendBufferData()
{
	int buffSizeToSend = strlen(szbuffer);	//TCP doesn't send all the bytes all the time.
	char *pbuf = szbuffer;
	int iResult = 0;

	while (buffSizeToSend > 0)
	{
		iResult = send(s1, pbuf, buffSizeToSend, 0);
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

	return iResult;
}

int FindNextTargetCharacter(char* str, char target, int startingAt)
{
	char* seperator;	//Using ',' as the seperator

	seperator = strchr(str+startingAt, target);

	if (seperator == NULL)
	{
		return NULL;
	}

	return seperator - str;
}

void TranslateHeaderInfo(TransferType* transferType, char (&fileName)[FILENAME_MAX], int &size)		//TODO::: Clean this up.... No time for now..
{
	const char SPLITTER = ',';
	char commandType[10];
	char fileSize[20];
	char* seperator;	//Using ',' as the seperator
	char* prevSeperator;
	int location = -1;
	int previousLocation = -1;
	int length = 0;

	//Clean up
	memset(commandType, 0, 10);
	memset(fileName, 0, FILENAME_MAX);
	memset(fileSize, 0, 20);

	seperator = strchr(szbuffer, SPLITTER);

	if (seperator == NULL)
	{
		throw "No Seperators found";
	}

	//Find Command Type
	location = seperator - szbuffer;
	prevSeperator = seperator;
	seperator = strchr(szbuffer + location + 1, SPLITTER);
	length = seperator - szbuffer;
	strncat_s(commandType, _countof(commandType), szbuffer, location);

	if (strcmp(commandType, "GET") == 0)
	{
		*transferType = GET;
	}
	else if (strcmp(commandType, "PUT") == 0)
	{
		*transferType = PUT;
	}
	else if (strcmp(commandType, "LIST") == 0)
	{
		*transferType = LIST;
	}
	else
	{
		throw "Wrong Command Type recieved.";
	}

	//Find File Name
	previousLocation = location;
	seperator = strchr(szbuffer + location + 1, SPLITTER);
	length = seperator - prevSeperator;
	location = seperator - szbuffer;
	prevSeperator = seperator;
	strncat_s(fileName, _countof(fileName), szbuffer+previousLocation+1, length-1);

	//Find File Size
	previousLocation = location;
	seperator = strchr(szbuffer + location + 1, SPLITTER);
	length = seperator - prevSeperator;
	if (length > 1)
	{
		strncat_s(fileSize, _countof(fileSize), szbuffer + previousLocation + 1, length - 1);
		size = atof(fileSize);	//sscanf_s(fileSize, "%d", &numOfPackets); Might be a safer better way to type cast.
	}
	else
	{
		memset(fileSize, 0, 20);
		size = 0;
	}
}

void HandShake()
{
	memset(szbuffer, 0, BUFF_LENGTH);
	int iResult = 0;
	iResult = recv(s1, szbuffer, BUFF_LENGTH, 0);	
	if (iResult == SOCKET_ERROR)
	{
		throw "Initial HandShake Failed.";
	}
}

bool readdata(SOCKET sock, void *buf, int buflen)
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

bool readlong(SOCKET sock, long *value)
{
	if (!readdata(sock, value, sizeof(value)))
		return false;
	*value = ntohl(*value);
	return true;
}

bool readfile(SOCKET sock, FILE *f)
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

void RecieveFile(char* fileName, int totalSize)
{
	errno_t err;
	memset(szbuffer, 0, BUFF_LENGTH);

	FILE *pFile;

	err = fopen_s(&pFile, fileName, "wb");

	if (err != 0)
	{
		throw "Cannot Open File.";
		return;
	}


	if (pFile != NULL)
	{
		bool ok = readfile(s1, pFile);
		fclose(pFile);

		if (ok)
		{
			cout << "File Transferred Perfectly" << endl;
		}
		else
			remove("imagefile.jpg");
	}
}

bool senddata(SOCKET sock, void *buf, int buflen)
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

bool sendlong(SOCKET sock, long value)
{
	value = htonl(value);
	return senddata(sock, &value, sizeof(value));
}

bool sendfile(SOCKET sock, FILE *f)
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

void SendFile()
{
	errno_t err;
	memset(szbuffer, 0, BUFF_LENGTH);

	FILE *pFile;

	err = fopen_s(&pFile, fileName, "rb");

	if (err != 0)
	{
		throw "Cannot Open File.";
		return;
	}

	if (pFile != NULL)
	{
		sendfile(s1, pFile);
		fclose(pFile);
	}
}

int main(void)
{
	try{
		StartWinSock();
		HostName();
		CreateSocket();
		FillConnectionInfo();
		BindPort();		
		ListenRequest();	

		FD_ZERO(&readfds);	//Constant Read

		while (1)
		{
			ListenForConnections();

			getnameinfo((struct sockaddr *) &sa, sizeof(struct sockaddr), hostname, NI_MAXHOST, NULL, NI_MAXSERV, NI_NUMERICSERV);

			system("cls");
			HandShake();
			//RecieveData();	//Initial Handshake
			
			TransferType transferType;

			memset(fileName, 0, FILENAME_MAX);
			int fileSize;
			int numOfPackets = 0;

			TranslateHeaderInfo(&transferType, fileName, fileSize);
			cout << fileSize << endl;
			switch (transferType)
			{
			case LIST:
				GetAllfilesInWorkingDirectory();
				SendBufferData();
				cout << "List of Files successfully transferred" << endl;
				break;
			case PUT:
				cout << "User \"" << hostname << "\" is uploading file: " << fileName << endl;
				cout << "Sending file to " << hostname << ", waiting..." << endl;

				RecieveFile(fileName, fileSize);

				cout << "The file has been successfully Uploaded." << endl;
				sprintf_s(szbuffer, "The File has been successfully uploaded.");
				SendBufferData();
				break;
			case GET:
				cout << "User \"" << hostname << "\" requested file " << fileName << " to be sent." << endl;
				if (!fileExist(fileName))
				{
					cout << "File: " << fileName << " does not exist. Ignoring request" << endl;
					memset(szbuffer, 0, BUFF_LENGTH);
					sprintf_s(szbuffer, "FAIL");
					SendBufferData();
					break;
				}
				memset(szbuffer, 0, BUFF_LENGTH);
				sprintf_s(szbuffer, "OK");
				SendBufferData();
				cout << "Sending file to " << hostname << ", waiting..." << endl;
				SendFile();
				cout << "File Has been succesfully transferred." << endl;
				break;
			}					

		}//wait loop

	} //try loop

	//Display needed error message.
	catch(char* str) { cerr<<str<<WSAGetLastError()<<endl;}

	//close Client socket
	closesocket(s1);		

	//close server socket
	closesocket(s);

	/* When done uninstall winsock.dll (WSACleanup()) and exit */ 
	WSACleanup();
	system("pause");
	return 0;
}