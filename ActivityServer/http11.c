//
//  http11.c
//  ActivityServer
//
//  Created by J O'Brien on 27/09/2011.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#include "http_limits.h"
#include "http11_parser.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "MxHashtable.h"
//#include "MxAsciiString.h"
#include "MxDebug.h"
#include "MxStringBuffer.h"
#include "MxHttpRequest.h"

#include "http11.h"



void http_field(void *data, const char *field, size_t field_len, const char *value, size_t value_len)
{
	if (data == NULL)
	{
		MxDebug("http_field given NULL data value\n");
		return;
	}
	
	if (field_len > FIELD_NAME_LIMIT)
	{
		MxDebug("Field name longer than allowed limit.\n");
		return;
	}
	
	if (value_len > FIELD_VALUE_LIMIT)
	{
		MxDebug("Field value longer than allowed limit.\n");
		return;
	}
	
	MxHttpRequestRef request = (MxHttpRequestRef)data;
	char *name = calloc(field_len + 1, 1);
	if (!name)
	{
		MxDebug("Out of memory parsing HTTP header");
		return;
	}
	
	// This will always truncate (the HTTP request is in one long string) so no use checking...
	strlcpy(name, field, field_len + 1);
    
	/*size_t copied;
     if ((copied = strlcpy(name, field, field_len + 1)) >= sizeof(name))
     {
     MxDebug("Copied %d, name_len %d, field_len %d\n", copied, sizeof(name), field_len);
     free(name);
     MxDebug("Field name too long when parsing HTTP header: %s\n", name);
     
     return;
     }*/
	
	MxStringBufferRef valueBuffer = NULL;
	MxHashtableRef headers = MxHttpRequestGetAllHeaders(request);
	if (!headers)
	{
		MxDebug("NULL headers returned from HTTP request");
		free(name);
		return;
	}
	
	MxStatus result = MxHashtableGet(headers, name, (void **)&valueBuffer);
	if (result == MxStatusNotFound)
	{
		// ::DEBUG::
        //		MxDebug("Creating new value buffer for header %s...\n", name);
		// ::END DEBUG::
		
		valueBuffer = MxStringBufferCreateWithValues((value_len + 1), (value_len + 1));
		if (valueBuffer == NULL) {
			free(name);
			MxDebug("Out of memory allocing header value in HTTP request\n");
			return;
		}
        
		// NOTE: The hashtable takes ownership of 'name' and valueBuffer here...
		result = MxHashtablePut(headers, name, valueBuffer);
		if (result != MxStatusOK) {
			MxDebug("Problem placing value buffer in header table when parsing HTTP req.");
			MxStringBufferDestroy(valueBuffer);
			return;
		}
	}
	else {
		// don't need name anymore - hashtable has a copy from a previous invocation of
		// this function.
		MxDebug("Duplicate header in HTTP request: %s\n", name);
		free(name);
	}
    
	
	result = MxStringBufferClear(valueBuffer);
	if (result != MxStatusOK)
	{
		MxStatusError("Problem clearing value buffer when parsing HTTP header.", result);
		
		// no need to clear any memory here - the headers stick around until the parse is over...
		return;
	}
	
	result = MxStringBufferAppendCharacters(valueBuffer, value, value_len);
	if (result != MxStatusOK)
	{
		MxStatusError("Problem filling in value buffer when parsing HTTP request", result);
		return;
	}
	
	// ::DEBUG::
    //	MxDebug("Value for %s is %s\n", name, MxStringBufferAsCStr(valueBuffer));
	// ::END DEBUG::
}


void request_method(void *data, const char *at, size_t length)
{
	if (!data)
	{
		MxDebug("request_method given NULL parameter");
		return; // TODO: warn...
	}
	
	MxHttpRequestRef req = (MxHttpRequestRef)data;
	MxStringBufferRef buf = MxHttpRequestGetMethod(req);
	
	MxStringBufferClear(buf);
	MxStringBufferAppendCharacters(buf, at, length);
}


void request_uri(void *data, const char *at, size_t length)
{
	if (!data)
	{
		MxDebug("request_method given NULL parameter");
		return; // TODO: warn...
	}
	
	MxHttpRequestRef req = (MxHttpRequestRef)data;
	MxStringBufferRef buf = MxHttpRequestGetUrl(req);
	MxStringBufferClear(buf);
	if (length <= REQUEST_URI_LIMIT)
		MxStringBufferAppendCharacters(buf, at, length);
}


void fragment(void *data, const char *at, size_t length)
{
	if (!data)
	{
		MxDebug("request_method given NULL parameter");
		return; // TODO: warn...
	}
	
	MxHttpRequestRef req = (MxHttpRequestRef)data;
	MxStringBufferRef buf = MxHttpRequestGetFragment(req);
	MxStringBufferClear(buf);
	if (length <= FRAGMENT_LIMIT)
		MxStringBufferAppendCharacters(buf, at, length);
}

void request_path(void *data, const char *at, size_t length)
{
	if (!data)
	{
		MxDebug("request_method given NULL parameter");
		return; // TODO: warn...
	}
	
	MxHttpRequestRef req = (MxHttpRequestRef)data;
	MxStringBufferRef buf = MxHttpRequestGetPath(req);
	MxStringBufferClear(buf);
    
	if (length <= REQUEST_PATH_LIMIT) 
		MxStringBufferAppendCharacters(buf, at, length);
}

void query_string(void *data, const char *at, size_t length)
{
	if (!data)
	{
		MxDebug("request_method given NULL parameter");
		return; // TODO: warn...
	}
	
	MxHttpRequestRef req = (MxHttpRequestRef)data;
	MxStringBufferRef buf = MxHttpRequestGetQueryString(req);
	MxStringBufferClear(buf);
	
	if (length <= QUERY_STRING_LIMIT)
		MxStringBufferAppendCharacters(buf, at, length);	
}

void http_version(void *data, const char *at, size_t length)
{
	
	MxHttpRequestRef req = (MxHttpRequestRef)data;
	MxStringBufferRef buf = MxHttpRequestGetHttpVersionString(req);
	MxStringBufferClear(buf);
	MxStringBufferAppendCharacters(buf, at, length);
}

void header_done(void *data, const char *at, size_t length)
{
	/* 
     The header is complete - if content len is set then we should 
     read in the rest of the content and parse appropriately */
}


http_parser *HttpParserCreate(void)
{
	http_parser *result = calloc(1, sizeof(http_parser));
	if (result)
	{
		result->http_field = http_field;
		result->request_method = request_method;
		result->request_uri = request_uri;
		result->fragment = fragment;
		result->request_path = request_path;
		result->query_string = query_string;
		result->http_version = http_version;
		result->header_done = header_done;
		http_parser_init(result);
	}
	
	return result;
}

MxStatus HttpParserDestroy(http_parser *parser)
{
	if (parser == NULL)
		return MxStatusNullArgument;
	
	free(parser);
	
	return MxStatusOK;
}

MxStatus HttpParserReset(http_parser *parser)
{
	if (parser == NULL)
		return MxStatusNullArgument;
	
	http_parser_init(parser);
	
	return MxStatusOK;
}

/*
 MxStatus HttpParserExecute(http_parser *parser, MxHttpRequestRef request)
 {
 if (parser == NULL || request == NULL || request->request_data == NULL)
 return MxStatusNullArgument;
 
 if (request->request_data == NULL)
 return MxStatusIllegalArgument;
 
 
 parser->data = (void *)request;
 const char *cSrc = MxStringBufferAsCStr(request->request_data);
 size_t len = MxStringBufferGetByteCount(request->request_data);
 
 http_parser_execute(parser, cSrc, len, 0);
 
 if (http_parser_has_error(parser))
 return MxStatusUnknownError;
 
 if (http_parser_nread(parser) > HEADER_LIMIT)
 return MxStatusIllegalArgument;
 
 return MxStatusOK;
 }
 */

MxStatus HttpParserHasError(http_parser *parser)
{
	if (parser == NULL)
		return MxStatusNullArgument;
	
	return http_parser_has_error(parser) ? MxStatusTrue : MxStatusFalse;
}

MxStatus HttpParserIsFinished(http_parser *parser)
{
	if (parser == NULL)
		return MxStatusNullArgument;
    
	return http_parser_is_finished(parser);
}