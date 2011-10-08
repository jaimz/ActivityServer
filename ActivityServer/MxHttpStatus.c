//
//  MxHttpStatus.c
//  ActivityServer
//
//  HTTP status codes.
//
//  Created by J O'Brien on 27/09/2011.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#include <stdlib.h>
#include "MxHttpStatus.h"

static char *HttpInformational[] = {
	"Continue",
	"Switching Protocols",
	NULL
};

static char *HttpSuccessful[] = {
	"OK",
	"Created",
	"Accepted",
	"Non-Authoritative Information",
	"No Content",
	"Reset Content",
	"Partial Content",
	NULL
};

static char *HttpRedirection[] = {
	"Multiple Choices",
	"Moved Permanently",
	"Found",
	"See Other",
	"Not Modified",
	"Use Proxy",
	" - unused - ",
	"Temporary Redirect",
	NULL
};

static char *HttpClientError[] = {
	"Bad request",
	"Unauthorised",
	"Payment Required",
	"Forbidden",
	"Not Found",
	"Method Not Allowed",
	"Not Acceptable",
	"Proxy Authentication Required",
	"Request Timeout",
	"Conflict",
	"Gone",
	"Length Required",
	"Precondition Failed",
	"Request Entity Too Large",
	"Request-URI Too Long",
	"Unsupported Media Type",
	"Requested Range Not Satisfyable",
	"Expectation Failed",
	NULL
};

static char *HttpServerError[] = {
	"Internal Server Error",
	"Not Implemented",
	"Bad Gateway",
	"Service Unavailable",
	"Gateway Timeout",
	"HTTP Version Not Supported",
	NULL
};



/*static char *(*HttpStatAll[])[] = { */
static char **HttpStatAll[] = {
	HttpInformational,
	HttpSuccessful,
	HttpRedirection,
	HttpClientError,
	HttpServerError,
	NULL
};


static MxArrayList http_codes;
static MxStatus have_codes = MxStatusFalse;

static void FreeTable(void *table)
{
	MxArrayListDelete((MxArrayListRef)table);
}

static MxStatus CreateMsgTable(char *msgs[], MxArrayListRef list)
{
	MxStatus s = MxStatusOK;
	if (list)
	{
		int idx = 0;
		
		while (msgs[idx])
		{
			s = MxArrayListAppend(list, msgs[idx]);
			if (s != MxStatusOK)
				break;
			
			idx += 1;
		}
	}
	else
	{
		s = MxStatusNoMemory;
	}
	
	return s;
}


static MxStatus InitHttpCodeTable()
{
	MxStatus st = MxStatusOK;
	
	if (!have_codes)
	{
		st = MxArrayListInitWithFunctions(&http_codes, FreeTable, NULL);
		if (st != MxStatusOK)
			return st;
		
		MxArrayListRef curr;
		int idx = 0;
		
		while (HttpStatAll[idx])
		{
			curr = MxArrayListCreateWithFunctions(NULL, NULL);
			if (curr)
			{
				st = CreateMsgTable(HttpStatAll[idx], curr);
				if (st != MxStatusOK)
					break;
				
				st = MxArrayListAppend(&http_codes, curr);
			}
			else
			{
				st = MxStatusNoMemory;
				break;
			}
			
			idx++;
		}
		
		have_codes = MxStatusTrue;
	}
	
	return st;
}

MxStatus MxHttpStatusStringify(MxHttpStatus http_status, char **result)
{
	*result = NULL;
	
	int http_cat = http_status / 100;
	int msg_idx = http_status % 100;
	
	MxStatus st = MxStatusOK;
	if (!have_codes)
	{
		st = InitHttpCodeTable();
		if (st != MxStatusOK)
			return st;
	}
	
	MxArrayListRef category;
	st = MxArrayListItemAt(&http_codes, http_cat-1, (void **)&category);
	if (st != MxStatusOK)
		return st;
	
	if (category == NULL)
		return MxStatusUnknownError;
	
	st = MxArrayListItemAt(category, msg_idx, (void **)result);
	if (st != MxStatusOK)
		*result = NULL;
	
	return st;
}

