//
//  MxHttpRequest.c
//  ActivityServer
//
//  Created by J O'Brien on 27/09/2011.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#include <stdlib.h>
#include <string.h> // memset

#include "MxStatus.h"
#include "MxStringBuffer.h"
#include "MxIO.h"
#include "MxHashtable.h"
#include "MxHttpRequest.h"
#include "http11.h"

static MxStatus CopyToResultBuffer(MxStringBufferRef source, MxStringBufferRef result);


MxHttpRequestRef MxHttpRequestCreateWithClientFD(int clientFd)
{
	MxHttpRequestRef result = malloc(sizeof(MxHttpRequest));
	if (result)
	{
		if (MxHttpRequestInitWithClientFD(result, clientFd) != MxStatusOK)
		{
			free(result);
			result = NULL;
		}
	}
	
	return result;
}


MxStatus MxHttpRequestInitWithClientFD(MxHttpRequestRef request, int clientFd)
{
	if (request == NULL)
		return MxStatusNullArgument;
	
	memset((void *)request, 0, sizeof(MxHttpRequest));
	
	request->client_fd = clientFd;
	request->parser_state = MxHttpParserNotStarted;
	request->parser_consumed = 0;
	request->parser = NULL;
	
	
	MxStatus status = MxStatusOK;
	
	
	if ((status = MxHashtableInitAsPropertyMap(&(request->headers))) != MxStatusOK)
		return status;
	
	// The headers hashtable takes ownership of strings calloc'd in http11.c...
	status = MxHashtableSetKeyFreeFunction(&(request->headers), free);
	MxStatusCheck(status);
	
	// NOTE: This means there will be a buch of mallocs/frees for each request - may want to do something
	// cleverer with memory...
	status = MxHashtableSetValueFreeFunction(&(request->headers), (MxFreeFunction)MxStringBufferDestroy);
	MxStatusCheck(status);
	
	if ((status = MxStringBufferInit(&(request->request_method))) != MxStatusOK)
		return status;
	
	if ((status = MxStringBufferInit(&(request->request_url))) != MxStatusOK)
		return status;
	
	if ((status = MxStringBufferInit(&(request->fragment))) != MxStatusOK)
		return status;
	
	if ((status = MxStringBufferInit(&(request->request_path))) != MxStatusOK)
		return status;
	
	if ((status = MxStringBufferInit(&(request->query_string))) != MxStatusOK)
		return status;
	
	if ((status = MxStringBufferInit(&(request->request_data))) != MxStatusOK)
		return status;
	
	return status;
}


MxStatus MxHttpRequestResetWithClientFD(MxHttpRequestRef request, int clientFd)
{
	if (request == NULL)
		return MxStatusNullArgument;
	
	request->client_fd = clientFd;
	request->parser_state = MxHttpParserNotStarted;
	request->parser_consumed = 0;
	
	MxStatus status = MxStatusOK;
	if (request->parser)
	{
		if ((status = HttpParserReset(request->parser)) != MxStatusOK)
			return status;
	}
	
	if ((status = MxStringBufferClear(&(request->request_method))) != MxStatusOK)
		return status;
	
	if ((status = MxStringBufferClear(&(request->request_url))) != MxStatusOK)
		return status;
	
	if ((status = MxStringBufferClear(&(request->fragment))) != MxStatusOK)
		return status;
	
	if ((status = MxStringBufferClear(&(request->request_path))) != MxStatusOK)
		return status;
	
	if ((status = MxStringBufferClear(&(request->query_string))) != MxStatusOK)
		return status;
	
	if ((status = MxStringBufferClear(&(request->request_data))) != MxStatusOK)
		return status;
	
	if ((status = MxHashtableClear(&(request->headers))) != MxStatusOK)
		return status;
	
	return status;
}

inline MxHashtableRef MxHttpRequestGetAllHeaders(MxHttpRequestRef request)
{
	if (request)
		return &(request->headers);
	
	return NULL;
}

inline MxStringBufferRef MxHttpRequestGetMethod(MxHttpRequestRef request)
{
	if (request)
		return &(request->request_method);
    
	return NULL;
}

inline MxStringBufferRef MxHttpRequestGetUrl(MxHttpRequestRef request)
{
	if (request)
		return &(request->request_url);
    
	return NULL;	
}

inline MxStringBufferRef MxHttpRequestGetFragment(MxHttpRequestRef request)
{
	if (request)
		return &(request->fragment);
    
	return NULL;
}

inline MxStringBufferRef MxHttpRequestGetPath(MxHttpRequestRef request)
{
	if (request)
		return &(request->request_path);
    
	return NULL;
}

inline MxStringBufferRef MxHttpRequestGetQueryString(MxHttpRequestRef request)
{
	if (request)
		return &(request->query_string);
    
	return NULL;
}

inline MxStringBufferRef MxHttpRequestGetHttpVersionString(MxHttpRequestRef request)
{
	if (request)
		return &(request->http_version);
    
	return NULL;
}

inline MxStringBufferRef MxHttpRequestGetRequestData(MxHttpRequestRef request)
{
	if (request)
		return &(request->request_data);
    
	return NULL;	
}


MxStatus MxHttpRequestParse(MxHttpRequestRef request)
{
	if (request == NULL)
		return MxStatusNullArgument;
	
	if (request->client_fd < 0 || &(request->request_data) == NULL)
		return MxStatusIllegalArgument;
	
	if (request->parser == NULL)
	{
		request->parser = HttpParserCreate();
		if (request->parser == NULL)
			return MxStatusNoMemory;
	}
	
	
	
	// Read the whole of the request into the request_data buffer...
	MxStatus result = MxIOReadUntilBlankLine(request->client_fd, &(request->request_data));
	if (result != MxStatusOK)
		return result;
    
	
	// this is crappy...
	request->parser->data = (void *)request;
	const char *cSrc = MxStringBufferAsCStr(&(request->request_data));
	size_t len = MxStringBufferGetByteCount(&(request->request_data));
	if (cSrc == NULL)
		len = 0; // something went wrong with the char * conversion - should never happen
	
	http_parser_execute(request->parser, cSrc, len, 0);
	
	if (http_parser_has_error(request->parser))
		request->parser_state = MxHttpParserErrored;
	else if (http_parser_is_finished(request->parser))
		request->parser_state = MxHttpParserFinished;
	else
		; // errr...
	
	request->parser_consumed = http_parser_nread(request->parser);
	
	
	return result;
}


static MxStatus CopyToResultBuffer(MxStringBufferRef source, MxStringBufferRef result)
{
	MxStatus status = MxStringBufferClear(result);
	MxStatusCheck(status);
	
	status = MxStringBufferAppend(result, MxStringBufferAsCStr(source));
	
	return status;
}


MxStatus MxHttpRequestGetHeader(MxHttpRequestRef request, const char *header_name, MxStringBufferRef result)
{
	MxAssertPointer(request);
	MxAssertPointer(header_name);
	MxAssertPointer(result);
	
	if (request->parser_state != MxHttpParserFinished)
		return MxStatusIllegalArgument;
    
	MxStringBufferRef value;
	MxStatus status = MxHashtableGet(&(request->headers), (const void *)header_name, (void **)&value);
	
	if (status == MxStatusOK)
		status = CopyToResultBuffer(value, result);
	
	return status;
}


MxStatus MxHttpRequestGetParameter(MxHttpRequestRef request, const char *param_name, MxStringBufferRef result)
{
	// TODO: Neet to parse out parameters...
	return MxStatusNotImplemented;
}

MxStatus MxHttpRequestGetResource(MxHttpRequestRef request, MxStringBufferRef result)
{
	MxAssertPointer(request);
	MxAssertPointer(result);
	
	if (request->parser_state != MxHttpParserFinished)
		return MxStatusIllegalArgument;
	
	return CopyToResultBuffer(&(request->request_url), result);
}



MxStatus MxHttpRequestFragment(MxHttpRequestRef request, MxStringBufferRef result)
{
	MxAssertPointer(request);
	MxAssertPointer(result);
	
	if (request->parser_state != MxHttpParserFinished)
		return MxStatusIllegalArgument;
	
	return CopyToResultBuffer(&(request->fragment), result);
}


MxStatus MxHttpRequestGetPostBody(MxHttpRequestRef request, MxStringBufferRef result)
{
	MxAssertPointer(request);
	MxAssertPointer(result);
	
	if (request->parser_state != MxHttpParserFinished)
		return MxStatusIllegalArgument;
	
	return CopyToResultBuffer(&(request->request_data), result);
}

MxStatus MxHttpRequestWipe(MxHttpRequestRef request)
{
	if (request == NULL)
		return MxStatusNullArgument;
    
	request->client_fd = -1;
	
	MxStatus result = MxStatusOK;
	if ((result = HttpParserDestroy(request->parser)) != MxStatusOK)
		return result;
	
	if ((result = MxHashtableWipe(&(request->headers))) != MxStatusOK)
		return result;
	
	if ((result = MxStringBufferWipe(&(request->request_method))) != MxStatusOK)
		return result;
    
	if ((result = MxStringBufferWipe(&(request->request_url))) != MxStatusOK)
		return result;	
	
	if ((result = MxStringBufferWipe(&(request->fragment))) != MxStatusOK)
		return result;
	
	if ((result = MxStringBufferWipe(&(request->request_path))) != MxStatusOK)
		return result;
	
	if ((result = MxStringBufferWipe(&(request->query_string))) != MxStatusOK)
		return result;
	
	if ((result = MxStringBufferWipe(&(request->http_version))) != MxStatusOK)
		return result;
	
	if ((result = MxStringBufferWipe(&(request->request_data))) != MxStatusOK)
		return result;
	
	
	return result;
}

MxStatus MxHttpRequestDelete(MxHttpRequestRef request)
{
	if (request == NULL)
		return MxStatusNullArgument;
	
	MxStatus result = MxHttpRequestWipe(request);
	if (result == MxStatusOK)
		free(request);
	
	return result;
}