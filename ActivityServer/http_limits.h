//
//  http_limits.h
//  ActivityServer
//
//  Limits on the length of various bits of HTTP - pretty much
//  made up on the spot.
//
//  Created by J O'Brien on 27/09/2011.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#ifndef ActivityServer_http_limits_h
#define ActivityServer_http_limits_h

#define FIELD_NAME_LIMIT (256)
#define FIELD_VALUE_LIMIT (80 * 1024)
#define REQUEST_URI_LIMIT (1024 *12)
#define FRAGMENT_LIMIT (1024)
#define REQUEST_PATH_LIMIT (1024)
#define QUERY_STRING_LIMIT (1024 * 10)
#define HEADER_LIMIT (1024 * (80 + 32))

#endif
