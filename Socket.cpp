#include "Socket.h"
#ifndef WIN32
#include <netinet/tcp.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef LINUX_BUILD
#include <unistd.h>
#endif

#if CAVE_SYNC

unsigned int Socket::socketsOpen = 0;
bool Socket::WSAStarted = false;
int MAXBYTES = 3000;

//------- Constructor
// Whenever a new socket is created, we must
// first make sure that winsocks are started up.
// If they are not started and we fail at starting them
// then we exit, otherwise, we increase the count of
// number of sockets open
//
Socket::Socket()
{
	_terminationChar = '\0';
	error = false;
	firstView = true;
}

Socket::Socket(SOCKET created)
{
	_terminationChar = '\0';
	_socket = created;
	firstView = true;

	if(!Socket::WSAStarted)
	{
		Socket::WSAStarted = Startup();
	}

	if(!Socket::WSAStarted)
	{
		std::cerr << "Exiting" << std::endl;
		exit(0);
	}

	else
	{
		Socket::socketsOpen++;
	}

	error = false;
}

void Socket::InitFromSocket(SOCKET created)
{
	_terminationChar = '\0';
	_socket = created;
	firstView = true;

	if(!Socket::WSAStarted)
	{
		Socket::WSAStarted = Startup();
	}

	if(!Socket::WSAStarted)
	{
		std::cerr << "Exiting" << std::endl;
		exit(0);
	}

	else
	{
		Socket::socketsOpen++;
	}

	error = false;
}

//------- Destructor
// Whenever we delete a socket, we decrement
// the number of sockets open. If that number
// reaches 0, then we need to cleanup winsocks
//
// Mutex?
Socket::~Socket()
{
	ShutDown();
}


//------- ShutDown
// This is the copy of the Destructor contents
void Socket::ShutDown()
{
	shutdown(_socket, 0x02);
	shutdown(_socket, 0x02);
#ifdef WIN32
	closesocket(_socket);
#else
	shutdown(_socket, 2);
	close(_socket);
#endif
	
	Socket::socketsOpen--;
	if(Socket::socketsOpen <= 0)
	{
		Cleanup();
		Socket::WSAStarted = false;
	}
}

//------- Methods

// Socket::Create(
//		address family	(AF_INET default),
//		socket type		(SOCK_STREAM default),
//		protocol		(0 default)
//	)
//
// Creates a socket for use in either 
//	client or server relationship
bool // true on success, false on failure
Socket::Create(int af, int type, int protocol)
{
	if(!Socket::WSAStarted)
	{
		Socket::WSAStarted = Startup();
	}

	if(!Socket::WSAStarted)
	{
		std::cerr << "WSAStarted Failed: Exiting" << std::endl;
		exit(0);
	}
	else
	{
		Socket::socketsOpen++;
	}

	_af = af;
	_socket = socket(af, type, protocol);
	
	// Setting socket options:
	//   TCP_NODELAY is for immediate transmission
	//   SO_REUSEADDR is to prevent unbindable error due to legacy
	//   SO_LINGER allowes it to send last command (exit) sent
	bool flag = true;
#ifdef WIN32
	LINGER lng;
#else
	linger lng;
#endif
	lng.l_onoff=1;
	lng.l_linger=500;	// which is 5 sec
	
	//Ross 5/19/2015: this is the master node
	setsockopt(_socket, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));
	setsockopt(_socket, SOL_SOCKET, SO_REUSEADDR, (char *) &flag, sizeof(int));
	setsockopt(_socket, SOL_SOCKET, SO_LINGER, (char*) &lng, sizeof(lng));
	if(_socket == INVALID_SOCKET)
	{
		std::cerr << "Invalid Socket" << std::endl;
		return false;
	}
	else
	{
		return true;
	}
	
	//if iMode isn't zero, non-blocking, if zero, blocking.
	//int iMode = 1;
	//ioctlsocket(_socket, FIONBIO, (u_long FAR*) &iMode);
}

// Socket::Connect(
//		host name as string	(e.g. "localhost","127.0.0.1")
//		host port as short
//	)
//
// Connects to the specified server and port
bool // true on success, false on failure
Socket::Connect(char* hostname, short hostport)
{
	if(!(_hostentry = gethostbyname(hostname)))
	{
		std::cerr << "Can't get host name" << std::endl;
		return false;
	}
	
	_address.sin_family = _af;
	_address.sin_port = htons(hostport);
	_address.sin_addr = *((in_addr *)_hostentry->h_addr);
	memset(&(_address.sin_zero), 0, 8);

	int ret = connect(_socket, (sockaddr*)&_address, sizeof(_address));
	if(ret)
	{
		std::cerr << "Could not connect" << std::endl;
		return false;
	}

	return true;
}

// Socket::Send(
//		message to send as a string
//		flags (0 default)
//	)
//
// Sends a message to the previously connected
//	to machine
int // returns number of bytes sent or error
Socket::Send(char* message, int size, int flags)
{
	if(size == 0)
	{
		return send(_socket, message, (int)strlen(message), flags);
	}
	else 
	{
		return send(_socket, message, size, flags);
	}
}

// Socket::sendTerminationChar()
//
// send the preset termination character
int
Socket::sendTerminationChar(int flags)
{
	char message = _terminationChar;
	return Send(&message, 1, flags);
}

// Socket::setTerminationChar(
//		char to terminate recieving on ('\0' default)
//	)
//
// When recieving, we may want to terminate when we recieve a 
//	specific char, this sets that char
void
Socket::setTerminationChar(char terminationChar)
{
	_terminationChar = terminationChar;
}

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
char* // return recieved buffer on success, NULL on error
Socket::Receive(int length, int flags)
{
	char* recieved = NULL;

	// get a specified number of bytes
	if(length > 0)
	{
		recieved = new char[length];
		// clear buffer
		memset((void*)recieved, '\0', sizeof(char)*length);
		
		int err = recv(_socket, recieved, length, flags);
		if(err > 0) // Success!
		{
			error = false;
		}

		else if(err == 0) // connection closed
		{
			if(!error)
			{
				std::cerr << "Connection Closed" << std::endl;
				error = true;
			}
			delete[] recieved;
			recieved = NULL;
		}

		else // error
		{
			if(!error)
			{
				std::cerr << "Error Recieving" << std::endl;
				error = true;
			}
			delete[] recieved;
			recieved = NULL;
		}
	}

	// recieve until terminationChar is found
	else
	{
		bool recieving = true;
		char buffer;
		int err;
		int i = 0;
		
		// assume a maximum of 1024 bytes to recieve
		// not good practice, I know.
		recieved = new char[1024];
		//recieved = new char[MAXBYTES];

		//while(recieving && i < MAXBYTES)
		while(recieving && i < 1024)
		{
			buffer = 0;
			
			// get 1 byte at a time
			err = recv(_socket, &buffer, 1, flags);

			if(err > 0) // Success!
			{
				error = false;

				// if it finds the termination char, then stop receiving
				if(buffer == _terminationChar)
				{
					recieved[i] = '\0';
					i++;
					recieving = false;
				}
				else
				{
					recieved[i] = buffer;
					i++;
				}
			}

			else if(err == 0) // connection closed
			{
				if(!error)
				{
					std::cerr << "Connection Closed" << std::endl;
					error = true;
				}
				recieving = false;
			}

			else // error
			{
				if(!error)
				{
					std::cerr << "Error Recieving" << std::endl;
					error = true;
				}
				recieving = false;
				delete[] recieved;
				recieved = NULL;
			}
		}
	}
	return recieved;
}



// Socket::Recieve(
//		user supplied buffer,
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
int
Socket::Receive(char* recieved, int length, int flags)
{
	// get a specified number of bytes
	if(length > 0)
	{
		// clear buffer
		memset((void*)recieved, '\0', sizeof(char)*length);

		char * ptr = recieved;	//may have to delete later.

		long sum = 0;
		int err = recv(_socket, ptr, length, flags);
		//printf("received %i bytes of data\n", err);
		if(err == length)
		{
			ptr += err;
			sum += err;
		}
		else if(err < length)
		{
			sum = err;
			long toReceive = 0;
			ptr += err;
			while(sum < length)
			{
				toReceive = length - sum;
				err = recv(_socket, ptr, toReceive, flags);
				sum += err;
				ptr += err;
			}
		}
		else if(err == 0) // connection closed
		{
			if(!error)
			{
				std::cerr << "Connection Closed" << std::endl;
				error = true;
			}
			recieved[0] = _terminationChar;
			sum = -1;
		}
		else // error
		{
			if(!error)
			{
				std::cerr << "Error Recieving" << std::endl;
				error = true;
			}
			recieved[0] = _terminationChar;
			sum = -1;
		}

		return sum;
	}

	// recieve until terminationChar is found
	else
	{
		bool recieving = true;
		char buffer;
		int err;
		int i = 0;
		
		memset((void*)recieved, '\0', sizeof(char)*length);
		//char * ptr = recieved;

		while(recieving)
		{
			buffer = 0;
			
			// get 1 byte at a time
			err = recv(_socket, &buffer, 1, flags);
			//err = recv(_socket, ptr, 1, flags);
			if(err > 0) // Success!
			{
				error = false;
				recieved[i] = buffer;
				i++;
				//ptr++;
				// if it finds the termination char, then stop receiving
				if(buffer == _terminationChar)
				//if(recieved[i-1] == _terminationChar)
				{
					recieving = false;
				}
			}

			else if(err == 0) // connection closed
			{
				if(!error)
				{
					std::cerr << "Connection Closed" << std::endl;
					error = true;
				}
				recieving = false;
			}

			else // error
			{
				if(!error)
				{
					std::cerr << "Error Recieving" << std::endl;
					error = true;
				}
				recieving = false;
				recieved[0] = _terminationChar;
				return -1;
			}
		}

		return i;
	}
}


void
Socket::Bind(short port)
{
	sockaddr_in addr;

	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_family = _af;
	addr.sin_port = htons(port);
	memset(addr.sin_zero, '\0', 8);

	if(bind(_socket, (const sockaddr*)&addr, sizeof(sockaddr_in)))
	{
		perror("Error binding Socket");
		return;
	}
}

void
Socket::Listen(short port)
{
	Bind(port);
	listen(_socket, 7);	//changed this from 5 to 7 - Ross
}


Socket*
Socket::Accept()
{
	SOCKET _accepted = SOCKET_ERROR;
#ifdef WIN32
	int size = sizeof(sockaddr_in);
#else
	socklen_t size = sizeof(sockaddr_in);
#endif
	do
	{
		_accepted = accept(_socket, (sockaddr*)&_address, &size);
	} while(_accepted == SOCKET_ERROR);

	Socket* client = new Socket(_accepted);

	bool flag = true;
//	LINGER linger;
//	linger.l_onoff=1;
//	linger.l_linger=100;
	
	//Ross 5/19/2015: this is a slave node connection...
	setsockopt(_accepted, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));       
	setsockopt(_socket, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));       
//	setsockopt(_socket, SOL_SOCKET, SO_LINGER, (char*) &linger, sizeof(LINGER));

	// let the connected client inherit the server's termination char
	client->setTerminationChar(_terminationChar);

	return client;
}


void
Socket::fillSockaddr_in(sockaddr_in* addr, char* hostname, short port)
{
	addr->sin_family = _af;

	hostent* hp = gethostbyname(hostname);
	if(hp == 0)
	{
		perror("Unknown Host");
		return;
	}

	memcpy(
		(void*)&addr->sin_addr,
		(void*)hp->h_addr, 
		hp->h_length);

	addr->sin_port = htons(port);
	memset(addr->sin_zero, '\0', 8);
}

int
Socket::SendTo(char* msg, sockaddr_in* to, int length, int flags)
{
	if(length == 0)
	{
		return sendto(_socket,msg,(int)strlen(msg),flags,(sockaddr*)to,sizeof(sockaddr_in));
	}
	else
	{
		return sendto(_socket,msg,length,flags,(sockaddr*)to,sizeof(sockaddr_in));
	}
}

char*
Socket::RecieveFrom(sockaddr_in* from, int length, int flags)
{
#ifdef WIN32
	int fromlen = sizeof(sockaddr_in);
#else
	socklen_t fromlen = sizeof(sockaddr_in);
#endif
	char* recieved = NULL;

	// get a specified number of bytes
	if(length > 0)
	{
		char* recieved = new char[length];
		// clear buffer
		memset((void*)recieved, '\0', sizeof(char)*length);
		
		int err = recvfrom(_socket, recieved, length, flags, (sockaddr*)from, &fromlen);

		if(err > 0) // Success!
		{
		}

		else if(err == 0) // connection closed
		{
			std::cerr << "Connection Closed" << std::endl;
			delete[] recieved;
			recieved = NULL;
		}

		else // error
		{
			std::cerr << "Error Recieving" << std::endl;
			delete[] recieved;
			recieved = NULL;
		}
	}

	// recieve until terminationChar is found
	else
	{
		std::cerr << "Error Recieving from: Invalid length specified" << std::endl;
		recieved = NULL;
	}
	return recieved;
}

#endif

