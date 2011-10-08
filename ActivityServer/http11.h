#ifndef __HTTP11__
#define __HTTP11__

/*
 *  http11.h
 *  project
 *
 *  Created by J O'BRIEN on 04/07/2009.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "MxStatus.h"
#include "http11_parser.h"
#include "MxHttpRequest.h"

http_parser *HttpParserCreate(void);
MxStatus HttpParserDestroy(http_parser *parser);
MxStatus HttpParserReset(http_parser *parser);
/*MxStatus HttpParserExecute(http_parser *parser, MxHttpRequestRef request);*/
MxStatus HttpParserHasError(http_parser *parser);
MxStatus HttpParserIsFinished(http_parser *parser);


void http_field(void *data, const char *field, size_t field_len, const char *value, size_t value_len);
void request_method(void *data, const char *at, size_t length);
void request_uri(void *data, const char *at, size_t length);
void fragment(void *data, const char *at, size_t length);
void request_path(void *data, const char *at, size_t length);
void query_string(void *data, const char *at, size_t length);
void http_version(void *data, const char *at, size_t length);
void header_done(void *data, const char *at, size_t length);



#endif