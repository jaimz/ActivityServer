//
//  MxHttpStatus.h
//  ActivityServer
//
//  Created by J O'Brien on 27/09/2011.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#ifndef ActivityServer_MxHttpStatus_h
#define ActivityServer_MxHttpStatus_h

#include "MxStatus.h"
#include "MxArrayList.h"

typedef int MxHttpStatus;

MxStatus MxHttpStatusStringify(MxHttpStatus http_status, char **result);

#endif
