#pragma once
class Network
{
public:
	Network();
	~Network();

	bool readfile(SOCKET sock, FILE *f);
	bool sendfile(SOCKET sock, FILE *f);
private:
	SOCKET sock;

	bool readlong(SOCKET sock, long *value);
	bool readdata(SOCKET sock, void *buf, int buflen);

	bool sendlong(SOCKET sock, long value);
	bool senddata(SOCKET sock, void *buf, int buflen);
};

