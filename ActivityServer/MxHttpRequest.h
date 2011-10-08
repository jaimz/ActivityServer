//
//  MxHttpRequest.h
//  ActivityServer
//
//  An HTTP request.
//
//  Created by J O'Brien on 27/09/2011.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#ifndef ActivityServer_MxHttpRequest_h
#define ActivityServer_MxHttpRequest_h

#include "http11.h"
#include "MxStringBuffer.h"
#include "MxHashtable.h"


// Possible states the HTTP parser can be in...
enum MxHttpParserState {
	MxHttpParserNotStarted, 
	MxHttpParserNotFinished, 
	MxHttpParserFinished, 
	MxHttpParserErrored
};



typedef struct _MxHttpRequest
{
	int client_fd;
	
	http_parser *parser;
	
	int parser_state;
	size_t parser_consumed;
    
	// char * -> MxStringBufferRef map...
	MxHashtable headers;
	
	MxStringBuffer request_method;
	MxStringBuffer request_url;
	MxStringBuffer fragment;
	MxStringBuffer request_path;
	MxStringBuffer query_string;
	MxStringBuffer http_version;
	
	MxStringBuffer request_data;
} MxHttpRequest, *MxHttpRequestRef;



MxHashtableRef MxHttpRequestGetAllHeaders(MxHttpRequestRef request);
MxStringBufferRef MxHttpRequestGetMethod(MxHttpRequestRef request);
MxStringBufferRef MxHttpRequestGetUrl(MxHttpRequestRef request);
MxStringBufferRef MxHttpRequestGetFragment(MxHttpRequestRef request);
MxStringBufferRef MxHttpRequestGetPath(MxHttpRequestRef request);
MxStringBufferRef MxHttpRequestGetQueryString(MxHttpRequestRef request);
MxStringBufferRef MxHttpRequestGetHttpVersionString(MxHttpRequestRef request);
MxStringBufferRef MxHttpRequestGetRequestData(MxHttpRequestRef request);


MxHttpRequestRef MxHttpRequestCreateWithClientFD(int clientFd);
MxStatus MxHttpRequestInitWithClientFD(MxHttpRequestRef request, int clientFd);
MxStatus MxHttpRequestResetWithClientFD(MxHttpRequestRef request, int clientFd);

MxStatus MxHttpRequestParse(MxHttpRequestRef request);

MxStatus MxHttpRequestGetHeader(MxHttpRequestRef request, const char *header_name, MxStringBufferRef result);
MxStatus MxHttpRequestGetParameter(MxHttpRequestRef request, const char *param_name, MxStringBufferRef result);
MxStatus MxHttpRequestGetResource(MxHttpRequestRef request, MxStringBufferRef result);
MxStatus MxHttpRequestFragment(MxHttpRequestRef request, MxStringBufferRef result);
MxStatus MxHttpRequestGetPostBody(MxHttpRequestRef request, MxStringBufferRef result);

MxStatus MxHttpRequestWipe(MxHttpRequestRef request);
MxStatus MxHttpRequestDelete(MxHttpRequestRef request);


#endif