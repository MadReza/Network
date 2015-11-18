char* getmessage(char *);

#include "network.h"

#include <tchar.h> 
#include <strsafe.h>
#include <direct.h>
#define GetCurrentDir _getcwd

/* send and receive codes between client and server */
/* This is your basic WINSOCK shell */
#pragma comment( linker, "/defaultlib:ws2_32.lib" )
#include <winsock2.h>
#include <ws2tcpip.h>

#include <winsock.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <fstream>

#include <string.h>
#include <math.h>

#include <windows.h>
#include <conio.h>

using namespace std;

//user defined port number
#define REQUEST_PORT 0x7070;
int port=REQUEST_PORT;

Network network;

enum TransferType { GET, PUT , LIST, CLOSE };	//CLOSE for connection

//buffer data types
const int BUFF_LENGTH = 1024;
char szbuffer[BUFF_LENGTH];
char fileName[256];
TransferType transferType;	// Get and Put. Maybe.... Check what are actually possible.

char *buffer;

int iResult;
int iBytesRecv=0;

//host data types
HOSTENT *hp;
HOSTENT *rp;

char localhost[21],
     remotehost[21];


//other
HANDLE test;
DWORD dwtest;

std::ifstream::pos_type FileSize(const char* fileName)
{
	std::ifstream in(fileName, std::ifstream::ate | std::ifstream::binary);
	return in.tellg();
}

char *GetAllfilesInWorkingDirectory()
{
	WIN32_FIND_DATA ffd;	//Data for each file is stored in this.
	TCHAR szDir[MAX_PATH];
	HANDLE hFind = INVALID_HANDLE_VALUE;

	char* listText = (char*)malloc(BUFF_LENGTH);		//Dynamic Memory Allocation for Char Array

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
	memset(listText, 0, BUFF_LENGTH);
	do
	{
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			//Ignore Directories.
		}
		else
		{
			strcat_s(listText, BUFF_LENGTH, "\t");
			strcat_s(listText, BUFF_LENGTH, ffd.cFileName);
			strcat_s(listText, BUFF_LENGTH, "\r\n");
		}
	} while (FindNextFile(hFind, &ffd) != 0);

	return listText;
}

bool fileExist(const char *fileName)
{
	std::ifstream infile(fileName);
	return infile.good();
}

TransferType GetUserTransferType()
{
	char selectedType[5];
	cout << "Type direction of transfer (\"get\", \"put\", \"list\"): ";

	while (true)
	{
		cin >> selectedType;

		if (strcmp(selectedType, "get") == 0)
		{
			return GET;
		}
		else if (strcmp(selectedType, "put") == 0)
		{
			return PUT;
		}
		else if (strcmp(selectedType, "list") == 0)
		{
			return LIST;
		}
		else if (strcmp(selectedType, "close") == 0)
		{
			return CLOSE;
		}
		else
		{
			cout << "Please without quotes and lowercase one of the following: (\"get\", \"put\", \"list\", \"close\"): ";
		}
	}
}

int GetUserChoiceOfServer()
{
	do
	{
		cout << "Type name of ftp server(\"quit\" to exit): ";
		cin >> remotehost;

		if (strcmp(remotehost, "quit") == 0)
		{
			return 1;
		}

	} while (network.hostExists(remotehost) == NULL);
	return 0;
}

void GetFileNameFromUser(char* fileName)
{
	char cCurrentPath[FILENAME_MAX];
	GetCurrentDir(cCurrentPath, sizeof(cCurrentPath));

	system("cls");
	cout << "Type name of file to be transferred: ";
	cin >> fileName;

	while (!fileExist(fileName))
	{
		system("cls");
		cout << "File: " << fileName << " does not exist!" << endl;
		cout << "Current Working Directory: " << endl << "\t" << cCurrentPath << endl;
		cout << "Try Again or type list for current directory listing: ";
		cin >> fileName;

		if (strcmp(fileName, "list") == 0)
		{
			system("cls");
			cout << "Files in Current Directory:" << endl;
			cout << GetAllfilesInWorkingDirectory() << endl;
			cout << "Select File Name:";
			cin >> fileName;
		}
	}
}

int RecieveData()
{
	memset(szbuffer, 0, BUFF_LENGTH);
	int buffSizeToRecieve = strlen(szbuffer);
	char *pbuf = szbuffer;

	do
	{
		iResult = recv(network.getSocket(), szbuffer, BUFF_LENGTH, 0);
		if (iResult == SOCKET_ERROR)
		{
			if (WSAGetLastError() == WSAEWOULDBLOCK)	//Buffer is full. Not a real error so SKIP
			{
				continue;
			}
			return iResult;
		}
		pbuf += iResult;	//Move forward to the part that didn't go through
		buffSizeToRecieve -= iResult;
	} while (iResult > 0);

	return iResult;	//0 for finished good.
}

void List()
{
	memset(szbuffer, 0, BUFF_LENGTH);
	sprintf_s(szbuffer, "LIST,,,\r\n");
	network.sendMsg(szbuffer);  //SendBufferData();

	//Recieve answer
	memset(szbuffer, 0, BUFF_LENGTH);
	iResult = recv(network.getSocket(), szbuffer, BUFF_LENGTH, 0);


	cout << "Files on the Server: " << endl;
	cout << szbuffer;
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
		network.sendFile(pFile);
		fclose(pFile);
	}
}

void Get(char* fileName, int totalSize)
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
		bool ok = network.readFile(pFile);
		fclose(pFile);

		if (ok)
		{
			cout << "File Transferred Perfectly" << endl;
		}
		else
			remove("imagefile.jpg");
	}
}

void TranslateHeaderInfo(TransferType* transferType, char(&fileName)[FILENAME_MAX], int &size)		//TODO::: Clean this up.... No time for now..
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
	strncat_s(fileName, _countof(fileName), szbuffer + previousLocation + 1, length - 1);

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


void Answer()
{
	memset(szbuffer, 0, BUFF_LENGTH);
	int iResult = 0;
	iResult = recv(network.getSocket(), szbuffer, BUFF_LENGTH, 0);
	if (iResult == SOCKET_ERROR)
	{
		throw "Initial HandShake Failed.";
	}
}

void RecieveFile(char* fileName)
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
		bool ok = network.readFile(pFile);
		fclose(pFile);

		if (ok)
		{
			cout << "File Transferred Perfectly" << endl;
		}
		else
			remove("imagefile.jpg");
	}
}

int Get()
{
	memset(fileName, 0, FILENAME_MAX);
	int fileSize;
	cout << "What File would you like to recieve: ";
	cin >> fileName;
	cout << "Checking if file is valid...." << endl;

	memset(szbuffer, 0, BUFF_LENGTH);

	strcat_s(szbuffer, BUFF_LENGTH, "GET,");
	strcat_s(szbuffer, BUFF_LENGTH, fileName);		//TODO :::: Remove The Path if any.
	strcat_s(szbuffer, BUFF_LENGTH, ",,\r\n");
	network.sendMsg(szbuffer); //SendBufferData();

	//Recieve answer
	memset(szbuffer, 0, BUFF_LENGTH);
	iResult = recv(network.getSocket(), szbuffer, BUFF_LENGTH, 0);

	if (strcmp(szbuffer, "OK") != 0)
	{
		cout << "File does not exist!" << endl;
		return 0;
	}

	cout << "Recieving File..." << endl;
	RecieveFile(fileName);

}

int main(void)
{
	try {
		network.startWinSock(); //StartWinSock();
		cout << network.getLocalHostInfo() << endl; //DisplayLocalHostInfo(); 
		if (GetUserChoiceOfServer() == 1)	//1 the user selected to quit
		{
			return 1; 
		}

		while (true)
		{
			transferType = GetUserTransferType();

			if (transferType == CLOSE)
				break;

			network.createSocket(SOCK_STREAM);     //CreateSocket();
			network.connectToServer();	//ConnectToServer();

			switch (transferType)
			{
			case LIST:
				system("cls");
				cout << "Sent request to " << remotehost << ", waiting..." << endl;
				List();
				break;
			case GET:
				system("cls");
				if (Get() == 0)
					break;

				cout << "Sent request to " << remotehost << ", waiting..." << endl;
				network.sendMsg(szbuffer);  //SendBufferData();


				//USE THE GET FUNCTION TO TRANSFER THE FILE IN ... ****
				break;
			case PUT:
				GetFileNameFromUser(fileName);
				system("cls");
				cout << "Sent request to " << remotehost << ", waiting..." << endl;
				memset(szbuffer, 0, BUFF_LENGTH);
				strcat_s(szbuffer, BUFF_LENGTH, "PUT,");
				strcat_s(szbuffer, BUFF_LENGTH, fileName);		//TODO :::: Remove The Path if any.
				strcat_s(szbuffer, BUFF_LENGTH, ",");
				string s = std::to_string((int)htonl(FileSize(fileName)));	//Convert int to string then into char array.
				strcat_s(szbuffer, BUFF_LENGTH, s.c_str());
				strcat_s(szbuffer, BUFF_LENGTH, ",");
				network.sendMsg(szbuffer);  //SendBufferData();	//Initial Handshake
				SendFile();
				Answer();
				cout << "Server Response: " << szbuffer << endl;
				break;
			}

			network.closeSocket();  //closesocket(s);
		}//while loop

	} // try loop

	//Display any needed error response.
	catch (char *str) { cerr<<str<<":"<<dec<<WSAGetLastError()<<endl;}

	//close the client socket
	network.closeSocket();//closesocket(s);

	/* When done uninstall winsock.dll (WSACleanup()) and exit */ 
	WSACleanup(); 
	_getch();
	return 0;
}