//
//  MxHttpServer.h
//  ActivityServer
//
//  An HTTP server. Can listen on TCP and UNIX sockets.
//
//  Created by J O'Brien on 27/09/2011.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#ifndef ActivityServer_MxHttpServer_h
#define ActivityServer_MxHttpServer_h

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>

#include "MxStatus.h"
#include "MxHttpTransaction.h"

#define MxHttpServerDefaultBacklog (10)



enum MxServerType { MxServerTcp, MxServerLocal };


typedef struct _MxHttpServer
{
	enum MxServerType server_type;
	
	int port;
	char *local_sock;
	
	int server_fd;
	
	int backlog;
	int running;
    
	union {
		struct sockaddr_un local_addr;
		struct sockaddr_in inet_addr;
	} addr;
    
} MxHttpServer, *MxHttpServerRef;



MxHttpServerRef	MxHttpServerCreate(void);
MxHttpServerRef MxHttpServerCreateOnPort(int port_num);
MxHttpServerRef MxHttpServerCreateOnLocal(const char *filename);


MxStatus MxHttpServerInit(MxHttpServerRef server);
MxStatus MxHttpServerInitOnPort(MxHttpServerRef server, int port_num);
MxStatus MxHttpServerInitOnLocal(MxHttpServerRef server, const char *filename);

MxStatus MxHttpServerGetPort(MxHttpServerRef server, int *result);
MxStatus MxHttpServerIsRunning(MxHttpServerRef server, int *result);


MxStatus MxHttpServerStart(MxHttpServerRef server, MxHttpConnectionHandler handler);
MxStatus MxHttpServerStop(MxHttpServerRef server);


MxStatus MxHttpServerWipe(MxHttpServerRef server);
MxStatus MxHttpServerDestroy(MxHttpServerRef server);


#endif
