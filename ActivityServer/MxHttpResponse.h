//
//  MxHttpResponse.h
//  ActivityServer
//
//  An HTTP response.
//
//  Created by J O'Brien on 27/09/2011.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#ifndef ActivityServer_MxHttpResponse_h
#define ActivityServer_MxHttpResponse_h

#include "MxStatus.h"
#include "MxStringBuffer.h"
#include "MxHashtable.h"

#include "MxHttpStatus.h"


#define MxHttpResponseStatusNotServiced (-1)


typedef struct _MxHttpResponse {
	int client_fd;
	
	MxStringBuffer content;
	MxHashtable headers;
    
	int http_status;
	
	int is_buffered;
} MxHttpResponse, *MxHttpResponseRef;


MxHttpResponseRef MxHttpResponseCreate(void);
MxHttpResponseRef MxHttpResponseCreateWithClientFD(int client_fd);

MxStatus MxHttpResponseInit(MxHttpResponseRef response);
MxStatus MxHttpResponseInitWithClientFD(MxHttpResponseRef response, int client_fd);
MxStatus MxHttpResponseReset(MxHttpResponseRef response);
MxStatus MxHttpResponseResetWithClientFD(MxHttpResponseRef response, int client_fd);


MxStatus MxHttpResponseSetBuffered(MxHttpResponseRef response, int buffered);
MxStatus MxHttpResponseIsBuffered(MxHttpResponseRef response, int *buffered);


MxStatus MxHttpResponseSetStatus(MxHttpResponseRef response, MxHttpStatus status);
MxStatus MxHttpResponseSetHeader(MxHttpResponseRef response, const char *name, const char *value);
MxStatus MxHttpResponseGetHeader(MxHttpResponseRef response, const char *name, MxStringBufferRef result);
MxStatus MxHttpResponseClearContent(MxHttpResponseRef response);
MxStatus MxHttpResponseAppendContent(MxHttpResponseRef response, const char *content);
MxStatus MxHttpResponseSendFile(MxHttpResponseRef response, const char *filename);


MxStatus MxHttpResponseFlush(MxHttpResponseRef response);

MxStatus MxHttpResponseWipe(MxHttpResponseRef response);
MxStatus MxHttpResponseDestroy(MxHttpResponseRef response);


#endif
