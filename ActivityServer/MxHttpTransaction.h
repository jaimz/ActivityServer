//
//  MxHttpTransaction.h
//  ActivityServer
//
//  Encapsulates an HTTP transaction for a connection handler.
//
//  Created by J O'Brien on 27/09/2011.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#ifndef ActivityServer_MxHttpTransaction_h
#define ActivityServer_MxHttpTransaction_h

#include "MxStatus.h"
#include "MxHttpRequest.h"
#include "MxHttpResponse.h"


typedef struct _MxHttpTransaction
{
	int is_buffered;
	MxStatus is_open;
	
	
	MxHttpRequest request;
	MxHttpResponse response;
} MxHttpTransaction, *MxHttpTransactionRef;


// Signature of a transaction handler function
// NOTE: Should this really be in this file?
typedef MxStatus (*MxHttpConnectionHandler)(MxHttpTransactionRef transaction);


MxHttpTransactionRef MxHttpTransactionCreate(void);
MxHttpTransactionRef MxHttpTransactionCreateWithClientFD(int clientFd);

MxStatus MxHttpTransactionInit(MxHttpTransactionRef transaction);
MxStatus MxHttpTransactionInitWithClientFD(MxHttpTransactionRef transaction, int clientFd);


MxStatus MxHttpTransactionReset(MxHttpTransactionRef transaction);
MxStatus MxHttpTransactionResetWithClientFD(MxHttpTransactionRef transaction, int clientFd);

MxStatus MxHttpTransactionSetBuffered(MxHttpTransactionRef transaction, int buffered);
MxStatus MxHttpTransactionIsBuffered(MxHttpTransactionRef transaction, int *result);

MxStatus MxHttpTransactionGetResponseRef(MxHttpTransactionRef transaction, MxHttpResponseRef *result);
MxStatus MxHttpTransactionGetRequestRef(MxHttpTransactionRef transaction, MxHttpRequestRef *result);


MxStatus MxHttpTransactionOpen(MxHttpTransactionRef transaction);
MxStatus MxHttpTransactionClose(MxHttpTransactionRef transaction);

MxStatus MxHttpTransactionWipe(MxHttpTransactionRef transaction);
MxStatus MxHttpTransactionDelete(MxHttpTransactionRef transaction);

#endif
