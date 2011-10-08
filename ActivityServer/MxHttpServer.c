//
//  MxHttpServer.c
//  ActivityServer
//
//  Created by J O'Brien on 27/09/2011.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stddef.h> // for offsetof

#include "MxStatus.h"
#include "MxDebug.h"

#include "MxHttpServer.h"
#include "MxHttpTransaction.h"



static MxStatus InitInet(MxHttpServerRef server, int port);
static MxStatus InitLocal(MxHttpServerRef server, const char *filename);
static MxStatus InitNonNet(MxHttpServerRef server);



MxHttpServerRef MxHttpServerCreate()
{
	MxHttpServerRef result = (MxHttpServerRef)malloc(sizeof(MxHttpServer));
	if (result)
	{
		if (MxHttpServerInit(result) != MxStatusOK) {
			free(result);
			result = NULL;
		}
	}
	
	return result;
}



MxHttpServerRef MxHttpServerCreateOnPort(int port_num)
{
	MxHttpServerRef result = (MxHttpServerRef)malloc(sizeof(MxHttpServer));
	if (result) {
		if (MxHttpServerInitOnPort(result, port_num) != MxStatusOK) {
			free(result);
			result = NULL;
		}
	}
	
	return result;
}



MxHttpServerRef MxHttpServerCreateOnLocal(const char *filename)
{
	MxHttpServerRef result = (MxHttpServerRef)malloc(sizeof(MxHttpServer));
	if (result) {
		if (MxHttpServerInitOnLocal(result, filename)) {
			free(result);
			result = NULL;
		}
	}
	
	return result;
}



MxStatus MxHttpServerInit(MxHttpServerRef server)
{
	return MxHttpServerInitOnPort(server, -1);
}



MxStatus MxHttpServerInitOnPort(MxHttpServerRef server, int port_num)
{
	MxAssertPointer(server);
	
	MxStatus result = InitNonNet(server);
	MxStatusCheck(result);
	
	result = InitInet(server, port_num);
    
	return result;
}



MxStatus MxHttpServerInitOnLocal(MxHttpServerRef server, const char *filename)
{
	MxAssertPointer(server);
	MxAssertPointer(filename);
	
	MxStatus result = InitNonNet(server);
	MxStatusCheck(result);
	
	result = InitLocal(server, filename);
	
	return result;
}



static MxStatus InitNonNet(MxHttpServerRef server)
{
	MxAssertPointer(server);
	
	server->backlog = MxHttpServerDefaultBacklog;
	server->running = 0;
	server->local_sock = NULL;
	server->port = -1;
	
	return MxStatusOK;
}



static MxStatus InitInet(MxHttpServerRef server, int port_num)
{
	server->server_fd = socket(PF_INET, SOCK_STREAM, 0);
	if (server->server_fd < 0) {
		MxStatusErrorWithStdErr("Could not create TCP socket", MxStatusUnixError);
		MxDebug("Could not create socket for HTTP server");
		
		return MxStatusUnixError;
	}
	
	
	
	int yes = 1;
	int callresult = setsockopt(server->server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
	if (callresult < 0) {
		MxStatusErrorWithStdErr("Can't set SO_REUSEADDR", MxStatusUnixError);
		MxDebug("Could not set socket option for HTTP server");
		int tmp = errno;
		close(server->server_fd);
		errno = tmp;
		
		return MxStatusUnixError;
	}
	
	
	server->addr.inet_addr.sin_family = AF_INET;
	server->addr.inet_addr.sin_len = sizeof(struct sockaddr_in);
	server->addr.inet_addr.sin_port = htons(port_num);
	server->addr.inet_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	memset(server->addr.inet_addr.sin_zero, 0, sizeof(server->addr.inet_addr.sin_zero));
	
	callresult = bind(server->server_fd, (struct sockaddr *)(&(server->addr.inet_addr)), sizeof(server->addr.inet_addr));
	if (callresult == -1)
	{
		MxDebug("Could not bind TCP socket for HTTP server");
		int tmp = errno;
		close(server->server_fd);
		errno = tmp;
		
		return MxStatusUnixError;
	}
	
	if (port_num == 0)
	{
		// Caller requested a random port...
		server->port = server->addr.inet_addr.sin_port;
	}
	else
	{
		server->port = port_num;
	}
	
	server->server_type = MxServerTcp;
	
	return MxStatusOK;
}



static MxStatus InitLocal(MxHttpServerRef server, const char *filename)
{
	MxAssertPointer(server);
	MxAssertPointer(filename);

    // ::SECURITY:: - what's the max len of 'filename'??
	size_t s_len = strlen(filename);
	if (s_len == 0)
		return MxStatusIllegalArgument;
    
    
	
	// figure out the maximum length of the filename a local socket can take...
	// UNIX programming rocks!
	size_t sock_max_len = sizeof(server->addr.local_addr);
	sock_max_len -= offsetof(struct sockaddr_un, sun_path);
	
	// if the given filename + terminating \0 is too long then we're stuck...
	// UNIX programming rocks!
	if (s_len > sock_max_len)
		return MxStatusFilenameTooLong;
	
	
	// if the local port already exists - remove it...
	// TODO: Should be configurable...
	int callResult = unlink(filename);
	if (callResult < 0) {
		// ENOENT is OK - just means there was no existing file...
		if (errno != ENOENT) {
			MxDebug("Unix error removing socket %s: %s", filename, strerror(errno));
			return MxStatusUnixError;
		}
	}
	
	
	server->server_fd = socket(PF_LOCAL, SOCK_STREAM, 0);
	if (server->server_fd < 0) {
		MxStatusErrorWithStdErr("Could not create local socket", MxStatusUnixError);
		MxDebug("Could not create local socket for HTTP server: %s", filename);
		
		return MxStatusUnixError;
	}
	
	
	memset((void *)(&(server->addr.local_addr)), 0, sizeof(server->addr.local_addr));
	server->addr.local_addr.sun_family = AF_LOCAL;
	strncpy(server->addr.local_addr.sun_path, filename, sizeof(server->addr.local_addr.sun_path));
    
	socklen_t addr_len = (socklen_t)(offsetof(struct sockaddr_un, sun_path) + s_len);
	callResult = bind(server->server_fd, (struct sockaddr *)(&(server->addr.local_addr)), addr_len);
	
	if (callResult < 0)
	{
		MxDebug("Unix error binding to local socket %s: %s", filename, strerror(errno));
		int tmp = errno;
		close(server->server_fd);
		errno = tmp;
		
		return MxStatusUnixError;
	}
	
	
	server->server_type = MxServerLocal;
	
	return MxStatusOK;
}


MxStatus MxHttpServerStart(MxHttpServerRef server, MxHttpConnectionHandler handler)
{
	MxAssertPointer(server);
	
	MxHttpTransaction transaction;
	MxStatus status = MxHttpTransactionInit(&transaction);
	MxStatusCheck(status);
	
	struct sockaddr *client_addr = NULL;
	socklen_t cli_addr_len = 0;
	
	switch (server->server_type) {
		case MxServerTcp:
			client_addr = alloca(sizeof(struct sockaddr_in));
			cli_addr_len = sizeof(struct sockaddr_in);
			break;
		case MxServerLocal:
			client_addr = alloca(sizeof(struct sockaddr_un));
			cli_addr_len = sizeof(struct sockaddr_un);
			break;
		default:
			MxDebug("Can't understand server type");
			break;
	}
	
	if (client_addr == NULL)
		return MxStatusNoMemory;
	
	int callresult = listen(server->server_fd, server->backlog);
	if (callresult == -1)
	{
		MxDebug("Listen failed in HTTP server: %s\n", strerror(errno));
		return MxStatusUnixError;
	}
	
	
	// ::DEBUG::
	MxDebug("Starting web server...\n");
	// ::END DEBUG::
	
	int remote_socket;
	server->running = MxStatusTrue;
	while (server->running) {
		callresult = accept(server->server_fd, client_addr, &cli_addr_len);
		if (callresult == -1)
		{
			MxDebug("Accept failed in HTTP server: %s\n", strerror(errno));
			continue;
		}
		
		// ::DEBUG::
		MxDebug("Accepted connection - resetting transaction...\n");
		// ::END DEBUG::
		
		remote_socket = callresult;
		status = MxHttpTransactionResetWithClientFD(&transaction, remote_socket);
		if (status != MxStatusOK)
		{
			MxDebug("Could not reset HTTP transaction: %s", MxStatusMsg(status));
			close(remote_socket);
			continue;
		}
        
		
		
		// ::DEBUG::
		MxDebug("\nOpening HTTP transaction...\n");
		// ::END DEBUG::
		status = MxHttpTransactionOpen(&transaction);
        
        if (status == MxStatusOK)
        {
            
            // ::DEBUG::
            //		MxHttpRequestRef req;
            //		status = MxHttpTransactionGetRequestRef(&transaction, &req);
            //		if (status == MxStatusOK)
            //		{
            //			MxStringBufferRef s = MxHttpRequestGetUrl(req);
            //			if (s != NULL)
            //			{
            //				MxDebug("...request is for: %s\n\n", MxStringBufferAsCStr(s));
            //			}
            //			else {
            //				MxDebug("...could not get URL from request object!\n");
            //			}
            //			
            //			s = MxHttpRequestGetRequestData(req);
            //			if (s)
            //			{
            //				MxDebug("Request:\n%s\n---\n", MxStringBufferAsCStr(s));
            //			}
            //			else {
            //				MxDebug("Could not get request data!\n");
            //			}
            //		}
            //		else {
            //			MxDebug("...could not get request object from transaction!\n");
            //		}
            // ::END DEBUG::
            
            
            
            // ::DEBUG::
            MxDebug("Calling transaction handler...\n");
            // ::END DEBUG::
            status = handler(&transaction);
            
            if (status != MxStatusOK)
            {
                MxDebug("Callback has a problem handling transaction: %s\n", MxStatusMsg(status));
                MxHttpResponseRef response;
                
                status = MxHttpTransactionGetResponseRef(&transaction, &response);
                if (status == MxStatusOK)
                {
                    status = MxHttpResponseSetStatus(response, 500);
                    if (status != MxStatusOK)
                    {
                        MxDebug("Could not set error status on transaction - client might get confused");
                    }
                }
                else {
                    MxDebug("Could not get response object form HTTP transaction when trying to set error");
                    MxStatusError("Could not get response object from HTTP transaction when trying to set error", status);
                }
                
            }
            
            // ::DEBUG::
            MxDebug("Closing transaction...\n");
            // ::END DEBUG::
            status = MxHttpTransactionClose(&transaction);
            if (status != MxStatusOK)
            {
                MxDebug("Could not close transaction nicely. Will force close on the client fd:\n");
                MxStatusErrorWithStdErr("Could not close transaction nicely. WIll force close on the client fd.", status);
                
                close(remote_socket);
            }
        }
        else
        {
            // ::DEBUG::
            MxDebug("Could not open HTTP transaction");
            MxStatusError("Could not open HTTP transaction", status);
        }
		
		
		// ::DEBUG::
		MxDebug("Resetting after close...\n");
		// ::END DEBUG::
		status = MxHttpTransactionReset(&transaction);
		if (status != MxStatusOK)
			MxStatusError("Couldn't wipe transaction", status);
		
		// ::DEBUG::
		MxDebug("\nTransaction handled.\n");
		// ::END DEBUG::
        
	}
	server->running = MxStatusFalse;
	
	
	if (close(server->server_fd) < 0)
	{
		MxDebug("Problem closing server socket: %s", strerror(errno));
		status = MxStatusUnixError;
	}
	
	return status;
}



MxStatus MxHttpServerWipe(MxHttpServerRef server)
{
	MxAssertPointer(server);
	
	memset((void *)server, 0, sizeof(MxHttpServerRef));
	
	return MxStatusOK;
}

MxStatus MxHttpServerDestroy(MxHttpServerRef server)
{
	MxAssertPointer(server);
    
	MxStatus result = MxHttpServerWipe(server);
	MxStatusCheck(result);
    
	free(server);
	
	return MxStatusOK;
}