// When using this class, you *must* link with wsock32.lib

#ifndef __SOCKET_H
#define __SOCKET_H

#define CAVE_SYNC 1

//#include <WS2tcpip.h>
#include <iostream>

#ifdef WIN32
#include <WinSock.h>
#else
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#ifndef SOCKET
typedef int SOCKET;
#endif
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

class Socket
{
public:
	//------- Constructors
	Socket();
	Socket(SOCKET created);
	~Socket();

	//initialize this socket from another already made socket
	void InitFromSocket(SOCKET socket);

	// Socket::Create(
	//		address family	(AF_INET default),
	//		socket type		(SOCK_STREAM default, use SOCK_STREAM for tcp, SOCK_DGRAM for udp)
	//		protocol		(0 default)
	//	)
	//
	// Creates a socket for use in either 
	//	client or server relationship
	// returns true on success, false on failure
	bool Create(int af = AF_INET, int type = SOCK_STREAM, int protocol = 0);

	// Socket::Connect(
	//		host name as string	(e.g. "localhost","127.0.0.1")
	//		host port as short
	//	)
	//
	// Connects to the specified server and port
	// returns true on success, false on failure
	bool Connect(char* message, short hostport);

	// Socket::Send(
	//		message to send as a string
	//		size of message	(0 default means string length)
	//		flags			(0 default)
	//	)
	//
	// Sends a message to the previously connected
	//	to machine
	// returns number of bytes sent or error
	int Send(char* message, int size = 0, int flags = 0);

	// Socket::Recieve(
	//		number of bytes to recieve.
	//			> 0  : Recieve at most, this many bytes.
	//			<= 0 : Continue recieving bytes until
	//					the termination char is encountered
	//			(0 default or inherited from recieving socket. See Socket::Accept),
	//		flags (0 default)
	//	)
	//
	// Recieve bytes from the previously connected to socket
	// You can recieve a specific number of bytes, or wait
	//	until the termination char is encountered to
	//	stop recieving
	// return recieved buffer on success, "" on error
	char* Receive(int length = 0, int flags = 0);

	// recieve using a personal buffer. Assumes that message fits in buffer
	int Receive(char* recieved, int length = 0, int flags = 0);
	
	// Socket::Bind(
	//		port for server socket to bind to
	//	)
	//
	// Binds a socket to a port
	void Bind(short port);

	// Socket::Listen(
	//		port for server socket to listen on
	//	)
	//
	// Sets up a socket to recieve connecting sockets
	//   Binds and Listens
	void Listen(short port);

	// Socket::Accept()
	//
	// When an incoming socket connects, accept
	//   will return a socket to interface with it
	//   The termination char of the listening socket
	//   is inherited by by the new socket.
	//   Accept blocks until a socket is connected
	// returns a connected socket
	Socket* Accept();

	// Socket::setTerminationChar(
	//		char to terminate recieving on ('\0' default)
	//	)
	//
	// When recieving, we may want to terminate when we recieve a 
	//	specific char, this sets that char
	void setTerminationChar(char terminationChar);
	
	// Socket::sendTerminationChar()
	//
	// send the preset termination character
	// returns number of bytes sent or error
	int sendTerminationChar(int flags = 0);

	// UDP Setup, not quite right yet
	void fillSockaddr_in(sockaddr_in* addr, char* hostname, short port);

	int SendTo(char* msg, sockaddr_in* to, int length = 0, int flags = 0);
	char* RecieveFrom(sockaddr_in* from, int length, int flags = 0);

	void ShutDown(void);
	SOCKET _socket;
	sockaddr_in _address;
	char _terminationChar;
	
	bool firstView;			//first or second viewer


private:
	hostent* _hostentry;
	int _af;
	bool error;
	//-------------- bookeeping
	// Keeps track of number of sockets opened
	static unsigned int socketsOpen;
	// tells us if winsocks are started
	static bool WSAStarted;

	// called to start winsocks, based on WSAStarted
	static bool Startup()
	{
#ifdef WIN32
		WSAData info;
		if (WSAStartup(MAKEWORD(2,0), &info)) {
			std::cerr << "Could not start WSA" << std::endl;
			return false;
		}
		else
#endif
		{
			return true;
		}
	}

	// called to shutdown winsocks, when socketsOpen reaches 0
	static int Cleanup()
	{
#ifdef WIN32
		return WSACleanup();
#else
		return 0;
#endif
	}
};

#endif