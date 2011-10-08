//
//  MxHttpResponse.c
//  ActivityServer
//
//  Created by J O'Brien on 27/09/2011.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "MxDebug.h"
#include "MxStatus.h"
#include "MxStringBuffer.h"
#include "MxHashtable.h"
#include "MxIO.h"

#include "MxHttpStatus.h"
#include "MxHttpResponse.h"

static int WipeHeaders(void *header_pair, void *state);
static int InsertHeaderPair(const void *key, const void *value, void *state);
static MxStatus CreateDefaultSuccessHeaders(MxHttpResponseRef response);

// return the header table from 'response', NULL if response is NULL
static MxHashtableRef GetHeaderRef(MxHttpResponseRef response)
{
	if (response)
		return &(response->headers);
	
	return NULL;
}

// Return the content buffer from 'response'. NULL if 'response' is NULL
static MxStringBufferRef GetContentRef(MxHttpResponseRef response)
{
	if (response)
		return &(response->content);
	
	return NULL;
}



MxHttpResponseRef MxHttpResponseCreate(void)
{
	return MxHttpResponseCreateWithClientFD(-1);
}



MxHttpResponseRef MxHttpResponseCreateWithClientFD(int client_fd)
{
	MxHttpResponseRef result = (MxHttpResponseRef)malloc(sizeof(MxHttpResponse));
	if (result)
	{
		if (MxHttpResponseInitWithClientFD(result, client_fd) != MxStatusOK)
		{
			free(result);
			result = NULL;
		}
	}
	
	return result;
}



MxStatus MxHttpResponseInit(MxHttpResponseRef response)
{
	return MxHttpResponseInitWithClientFD(response, -1);
}



MxStatus MxHttpResponseInitWithClientFD(MxHttpResponseRef response, int client_fd)
{
	MxAssertPointer(response);
	
	MxStringBufferRef response_buf = GetContentRef(response);
	if (response_buf == NULL)
		return MxStatusNullArgument;
    
	memset((void *)response_buf, 0, sizeof(MxStringBuffer));
    
	MxStatus status = MxStringBufferInit(GetContentRef(response));
	MxStatusCheck(status);
	
	
	MxHashtableRef headers = GetHeaderRef(response);
	if (headers == NULL)
		return MxStatusNullArgument;
	
	memset((void *)headers, 0, sizeof(MxHashtable));
	
	status = MxHashtableInitWithAllFunctions(headers, MxDefaultStringHashFunction, MxDefaultCStrEqualsFunction, NULL, (MxFreeFunction)MxStringBufferDestroy);
	MxStatusCheck(status);
	
	
	response->http_status = MxHttpResponseStatusNotServiced;
	response->client_fd = client_fd;
	
	return MxStatusOK;
}



MxStatus MxHttpResponseReset(MxHttpResponseRef response)
{
	return MxHttpResponseResetWithClientFD(response, -1);
}



MxStatus MxHttpResponseResetWithClientFD(MxHttpResponseRef response, int client_fd)
{
	MxAssertPointer(response);
	
	MxStatus result = MxStringBufferClear(GetContentRef(response));
	if (result != MxStatusOK)
		return result;
	
	
	if (result == MxStatusOK)
	{
		response->http_status = MxHttpResponseStatusNotServiced;
		response->client_fd = client_fd;
	}
	
	
	return result;
}


inline MxStatus MxHttpResponseSetBuffered(MxHttpResponseRef response, int buffered)
{
	if (response == NULL)
		return MxStatusNullArgument;
	
	response->is_buffered = buffered;
	
	return MxStatusOK;
}


inline MxStatus MxHttpResponseIsBuffered(MxHttpResponseRef response, int *result)
{
	if (response == NULL)
		return MxStatusNullArgument;
	
	*result = response->is_buffered;
	
	return MxStatusOK;
}


MxStatus MxHttpResponseSetStatus(MxHttpResponseRef response, MxHttpStatus status)
{
	MxAssertPointer(response);
	response->http_status = status;
	
	MxStatus result = MxStatusOK;
	if (!response->is_buffered)
	{
		result = MxHttpResponseFlush(response);
		
		// reset the status - otherwise the status string would be re-created on every flush
		response->http_status = -1;
	}
	
	return result;
}


MxStatus MxHttpResponseSetHeader(MxHttpResponseRef response, const char *name, const char *value)
{
	MxAssertPointer(response);
	MxAssertPointer(name);
	MxAssertPointer(value);
    
	MxHashtableRef headers = GetHeaderRef(response);
	if (headers == NULL)
		return MxStatusNullArgument;
	
	MxStringBufferRef valueBuffer;
	MxStatus result = MxHashtableContainsKey(headers, name);
	if (result == MxStatusTrue)
	{
		result = MxHashtableGet(headers, name, (void **)&valueBuffer);
		if (result != MxStatusOK)
		{
			MxDebug("Couldn't find apparently present key %s in http response headers\n", name);
			return result;
		}
        
		result = MxStringBufferClear(valueBuffer);
		if (MxStatusFailed(result))
		{
			MxDebug("Could not clear header value buffer: %s", MxStatusMsg(result));
			return result;
		}
		
		result = MxStringBufferAppend(valueBuffer, value);
		if (MxStatusFailed(result))
		{
			MxDebug("Could not append characters");
			return result;
		}
	}
	else if (result == MxStatusFalse)
	{
		valueBuffer = MxStringBufferCreate();
		if (valueBuffer)
		{
			result = MxHashtablePut(headers, name, valueBuffer);
			if (MxStatusFailed(result))
			{
				MxDebug("Could not put value buffer in headers table", MxStatusMsg(result));
				return result;
			}
			
			result = MxStringBufferAppend(valueBuffer, value);
			if (MxStatusFailed(result))
			{
				MxDebug("Could not append to header value buffer in HTTP response");
				return result;
			}
		}
		else
		{
			return MxStatusNoMemory;
		}
	}
	else
	{
		MxDebug("Problem ascertaining whether header %s is already in HTTP response", name);
	}
	
	
	if (!response->is_buffered)
		result = MxHttpResponseFlush(response);
	
	return result;
}

MxStatus MxHttpResponseGetHeader(MxHttpResponseRef response, const char *name, MxStringBufferRef result)
{
	MxAssertPointer(response);
	MxAssertPointer(name);
	MxAssertPointer(result);
	
	MxStringBufferRef in_table;
	
	MxStatus s = MxHashtableGet(GetHeaderRef(response), name, (void **)&in_table);
	if (s != MxStatusOK)
		return s;
	
	s = MxStringBufferClear(result);
	if (s != MxStatusOK)
		return s;
    
	if (in_table != NULL)
	{
		size_t len = MxStringBufferGetByteCount(in_table);
		if (len > 0)
		{
			const char *chars = MxStringBufferAsCStr(in_table);
			
			if (chars != NULL)
			{
				s = MxStringBufferAppendCharacters(result, chars, len);
			}
			else
			{
				MxDebug("Header value gave non-zero length but a NULL cstr!");
				s = MxStatusUnknownError; // Should never happen...
			}
		}
	}
	
	return s;
}


inline MxStatus MxHttpResponseClearContent(MxHttpResponseRef response)
{
	MxAssertPointer(response);
	return MxStringBufferClear(GetContentRef(response));
}

MxStatus MxHttpResponseAppendContent(MxHttpResponseRef response, const char *content)
{
	MxAssertPointer(response);
	MxStatus s = MxStringBufferAppend(GetContentRef(response), content);
	if (s != MxStatusOK)
		return s;
	
	if (!response->is_buffered)
		s = MxHttpResponseFlush(response);
	
	return s;
}

MxStatus MxHttpResponseSendFile(MxHttpResponseRef response, const char *filename)
{
    // TODO: Proxy to 'sendfile' sys call...
	return MxStatusNotImplemented;
}


static MxStatus InsertStatusString(MxHttpResponseRef response)
{
	MxStringBufferRef contentRef = GetContentRef(response);
	if (contentRef == NULL)
		return MxStatusNullArgument;
	
	MxStatus result = MxStringBufferSetPoint(contentRef, 0);
	if (result != MxStatusOK)
		return result;
	
	result = MxStringBufferInsert(contentRef, "HTTP/1.0 ");
	MxStatusCheck(result);
	
	result = MxStringBufferInsertInt(contentRef, response->http_status);
	MxStatusCheck(result);
	
	result = MxStringBufferInsertCharacters(contentRef, " ", 1);
	MxStatusCheck(result);
	
	char *status_string;
	result = MxHttpStatusStringify(response->http_status, &status_string);
	MxStatusCheck(result);
	
	result = MxStringBufferInsert(contentRef, status_string);
	MxStatusCheck(result);
	
	result = MxStringBufferInsertCharacters(contentRef, "\r\n", 2);
	MxStatusCheck(result);
	
	return result;
}

MxStatus MxHttpResponseFlush(MxHttpResponseRef response)
{
	MxAssertPointer(response);
	
	MxStringBufferRef contentRef = GetContentRef(response);
	if (contentRef == NULL)
		return MxStatusNullArgument;
	
	
	MxStatus result = MxStringBufferInsert(contentRef, "\n");
	MxStatusCheck(result);
	
	MxHashtableRef headers = GetHeaderRef(response);
	if (MxHashtableGetCount(headers) == 0)
	{
		// Handler didn't set any headers - create some default ones...
		result = CreateDefaultSuccessHeaders(response);
		MxStatusCheck(result);
	}
    
	
	if (response->http_status == MxHttpResponseStatusNotServiced)
	{
		// No status set by the handler.
		// Assume if the handler has set some content then the status
		// is 'success' otherwise set an 'internal server error'
		if (MxStringBufferGetCount(contentRef) > 0)
			response->http_status = 200;
		else
			response->http_status = 500;
	}
	
	
	InsertStatusString(response);
	
	result = MxHashtableIteratePairs(headers, InsertHeaderPair, contentRef);
	MxStatusCheck(result);
    
	result = MxStringBufferInsert(contentRef, "\n");
	MxStatusCheck(result);
    
	
	result = MxStringBufferInsert(contentRef, "\n");
	MxStatusCheck(result);
	
	// ::DEBUG::
    //	fprintf(stdout, "%s", MxStringBufferAsCStr(contentRef));
	// ::END DEBUG::
	
	result = MxIODrainToChannel(response->client_fd, contentRef);
	MxStatusCheck(result);
	
	result = MxStringBufferClear(contentRef);
	MxStatusCheck(result);
	
	return result;
}


static MxStatus CreateDateHeader(MxHttpResponseRef response)
{
	time_t t;
	time(&t);
	
	struct tm local_time;
	localtime_r(&t, &local_time);
	
	char buf[30];
	size_t len = strftime(buf, 30, "%a, %d %b %Y %T %Z", &local_time);
	if (len != 29)
		return MxStatusInvalidStructure;
	
	buf[29] = '\0';
	return MxHttpResponseSetHeader(response, "Date", buf);
}

static MxStatus CreateDefaultSuccessHeaders(MxHttpResponseRef response)
{
	MxStatus status = CreateDateHeader(response);
	if (status != MxStatusOK && status != MxStatusInvalidStructure)
		return status;
    
	MxStringBuffer scratch;
	memset(&scratch, 0, sizeof(scratch));
	status = MxStringBufferInitWithValues(&scratch, 20, 20);
	MxStatusCheck(status);
	
	size_t len = MxStringBufferGetByteCount(GetContentRef(response));
	if (len > 0) {
		// ::DEBUG::
        //		MxDebug("\nContent length: %d\n", len);
		// ::END DEBUG::
		
		status = MxStringBufferInsertInt(&scratch, ((int)len));
		MxStatusCheck(status);
        
		status = MxHttpResponseSetHeader(response, "Content-Length", MxStringBufferAsCStr(&scratch));
		MxStatusCheck(status);
	}
	
	status = MxHttpResponseSetHeader(response, "Content-Type", "text/html");
	MxStatusCheck(status);
	
	status = MxHttpResponseSetHeader(response, "Server", "Metro 0.1");
	MxStatusCheck(status);
	
	status = MxStringBufferWipe(&scratch);
	MxStatusCheck(status);
	
	return status;
}

MxStatus MxHttpResponseWipe(MxHttpResponseRef response)
{
	MxAssertPointer(response);
    
	
	MxStatus result = MxStringBufferWipe(GetContentRef(response));
	if (result != MxStatusOK)
		return result;
	
	result = MxHashtableWipe(GetHeaderRef(response));
    
	response->http_status = MxHttpResponseStatusNotServiced;
	
	return result;
}

MxStatus MxHttpResponseDestroy(MxHttpResponseRef response)
{
	MxAssertPointer(response);
	MxStatus status = MxHttpResponseWipe(response);
	if (status == MxStatusOK)
		free(response);
	else
		MxDebug("Problem wiping MxHttpResponse - will not free it: %s", MxStatusMsg(status));
	
	return status;
}

static int WipeHeaders(void *header_pair, void *state)
{
	int result = 0;
	if (header_pair)
	{
		MxPairRef pair = (MxPairRef)header_pair;
		free(pair->key);
		MxStatus status = MxStatusOK;
		if ((status = MxStringBufferDestroy(pair->value)) != MxStatusOK)
		{
			MxDebug("Problem destroying value buffer when wiping HTTP response headers: %s\n", MxStatusMsg(status));
			result = -1;
		}
	}
	else
	{
		MxDebug("Handed a NULL pointer when wiping headers in HTTP response");
		result = -2;
	}
	
	return result;
}


static MxStatus InsertHeaderPair(const void *key, const void *value, void *state)
{
	MxStringBufferRef content_buf = (MxStringBufferRef)state;
	MxStringBufferRef value_buf = (MxStringBufferRef)value;
    
	MxStatus result = MxStringBufferInsert(content_buf, (const char *)key);
	MxStatusCheck(result);
	
	result = MxStringBufferInsertCharacters(content_buf, ": ", 2);
	MxStatusCheck(result);
	
	result = MxStringBufferInsert(content_buf, MxStringBufferAsCStr(value_buf));
	MxStatusCheck(result);
	
	result = MxStringBufferInsertCharacters(content_buf, "\n", 1);
	return result;
}