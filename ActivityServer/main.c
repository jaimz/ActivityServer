//
//  main.c
//  ActivityServer
//
//  Created by J O'Brien on 31/08/2011.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>

#include "MxDebug.h"
#include "MxStatus.h"
#include "MxStringBuffer.h"
#include "MxHttpTransaction.h"
#include "MxHttpServer.h"
#include "MxHttpResponse.h"

static void dieWithStatus(const char *msg, MxStatus status);
static MxStatus transaction_handler(MxHttpTransactionRef transaction);
static MxStatus htmlise_header(const void *header_name, const void *header_value, void *http_request);

int main (int argc, const char * argv[]) {
	int port = 8181;
	
	MxHttpServerRef server = MxHttpServerCreateOnPort(port);
	if (server == NULL)
	{
		printf("Could not create server\n");
		return -1;
	}
    
	MxStatus result = MxHttpServerStart(server, transaction_handler);
	if (result != MxStatusOK)
		dieWithStatus("Could not start server", result);
	
	return 0;
}


static MxStatus transaction_handler(MxHttpTransactionRef transaction)
{
	if (transaction == NULL)
		return MxStatusNullArgument;
	
	MxHttpResponseRef response;
	MxStatus status = MxHttpTransactionGetResponseRef(transaction, &response);
	MxStatusCheck(status)
	
	status = MxHttpResponseAppendContent(response, "<html><head><title>Metro Server Test</title></head><body><h1>It works!</h1><div><table>");
	MxStatusCheck(status);
	
	MxHttpRequestRef request;
	status = MxHttpTransactionGetRequestRef(transaction, &request);
	MxStatusCheck(status);
	
	MxHashtableRef header_table = MxHttpRequestGetAllHeaders(request);
	
	status = MxHashtableIteratePairs(header_table, htmlise_header, response);
	MxStatusCheck(status);
	
	status = MxHttpResponseAppendContent(response, "</table></div></body>");
	MxStatusCheck(status);
	
	
	return status;
}


static MxStatus htmlise_header(const void *header_name, const void *header_value, void *http_response)
{
	if (header_name == NULL || header_value == NULL || http_response == NULL)
		return MxStatusNullArgument;
	
	MxHttpResponseRef response = (MxHttpResponseRef)http_response;
	MxStatus status = MxHttpResponseAppendContent(response, "<tr><td>");
	MxStatusCheck(status);
	
	status = MxHttpResponseAppendContent(response, header_name);
	MxStatusCheck(status);
	
	status = MxHttpResponseAppendContent(response, "</td><td>");
	MxStatusCheck(status);
	
	status = MxHttpResponseAppendContent(response, MxStringBufferAsCStr((MxStringBufferRef)header_value));
	MxStatusCheck(status);
	
	status = MxHttpResponseAppendContent(response, "</td>");
	return status;
}




static void dieWithStatus(const char *msg, MxStatus status)
{
	fprintf(stderr, "%s: %s\n", msg, MxStatusMsg(status));
	exit(-1);
}

