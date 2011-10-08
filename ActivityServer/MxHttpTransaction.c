//
//  MxHttpTransaction.c
//  ActivityServer
//
//  Created by J O'Brien on 27/09/2011.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#include <stdlib.h>
#include "MxStatus.h"
#include "MxHttpRequest.h"
#include "MxHttpResponse.h"
#include "MxHttpTransaction.h"

static inline MxHttpRequestRef GetRequestRef(MxHttpTransactionRef transaction)
{
	if (transaction != NULL)
		return &(transaction->request);
	
	return NULL;
}

static inline MxHttpResponseRef GetResponseRef(MxHttpTransactionRef transaction)
{
	if (transaction != NULL)
		return &(transaction->response);
	
	return NULL;
}


MxHttpTransactionRef MxHttpTransactionCreate(void)
{
	return MxHttpTransactionCreateWithClientFD(-1);
}

MxHttpTransactionRef MxHttpTransactionCreateWithClientFD(int clientFd)
{
	MxHttpTransactionRef result = (MxHttpTransactionRef)malloc(sizeof(MxHttpTransaction));
	if (result)
	{
		if (MxHttpTransactionInitWithClientFD(result, clientFd) != MxStatusOK)
		{
			free(result);
			result = NULL;
		}
	}
	
	return result;
}

MxStatus MxHttpTransactionInit(MxHttpTransactionRef transaction)
{
	return MxHttpTransactionInitWithClientFD(transaction, -1);
}

MxStatus MxHttpTransactionInitWithClientFD(MxHttpTransactionRef transaction, int clientFd)
{
	if (transaction == NULL)
		return MxStatusNullArgument;
	
	MxStatus result = MxHttpRequestInitWithClientFD(GetRequestRef(transaction), clientFd);
	if (result != MxStatusOK)
		return result;
	
	if ((result = MxHttpResponseInit(GetResponseRef(transaction))) != MxStatusOK)
		return result;
	
	
	return result;
}


MxStatus MxHttpTransactionReset(MxHttpTransactionRef transaction)
{
	return MxHttpTransactionResetWithClientFD(transaction, -1);
}


MxStatus MxHttpTransactionResetWithClientFD(MxHttpTransactionRef transaction, int clientFd)
{
	if (transaction == NULL)
		return MxStatusNullArgument;
	
	
	MxStatus result = MxHttpRequestResetWithClientFD(GetRequestRef(transaction), clientFd);
	if (result != MxStatusOK)
		return result;
	
	if ((result = MxHttpResponseResetWithClientFD(GetResponseRef(transaction), clientFd)) != MxStatusOK)
		return result;
	
	transaction->is_buffered = 1;
	
	return result;
}

MxStatus MxHttpTransactionGetResponseRef(MxHttpTransactionRef transaction, MxHttpResponseRef *result)
{
	MxAssertPointer(transaction);
	MxAssertPointer(result);
	
	*result = GetResponseRef(transaction);
	
	return MxStatusOK;
}

MxStatus MxHttpTransactionGetRequestRef(MxHttpTransactionRef transaction, MxHttpRequestRef *result)
{
	MxAssertPointer(transaction);
	MxAssertPointer(result);
	
	*result = GetRequestRef(transaction);
	
	return MxStatusOK;
}


MxStatus MxHttpTransactionOpen(MxHttpTransactionRef transaction)
{
	MxAssertPointer(transaction);
	
	MxStatus status = MxHttpRequestParse(GetRequestRef(transaction));
	if (status == MxStatusOK)
		transaction->is_open = MxStatusTrue;
	
	return status;
}

MxStatus MxHttpTransactionClose(MxHttpTransactionRef transaction)
{
	MxAssertPointer(transaction);
	if (transaction->is_open == MxStatusFalse)
		return MxStatusOK;
	
	MxStatus status = MxHttpResponseFlush(GetResponseRef(transaction));
	if (status == MxStatusOK)
		transaction->is_open = MxStatusFalse;
	
	return status;
}



MxStatus MxHttpTransactionWipe(MxHttpTransactionRef transaction)
{
	MxAssertPointer(transaction);
	
	MxStatus status = MxHttpRequestWipe(GetRequestRef(transaction));
	MxStatusCheck(status);
	
	status = MxHttpResponseWipe(GetResponseRef(transaction));
	
	return status;
}


MxStatus MxHttpTransactionDelete(MxHttpTransactionRef transaction)
{
	MxAssertPointer(transaction);
	
	MxStatus status = MxHttpRequestDelete(GetRequestRef(transaction));
	MxStatusCheck(status);
	
	status = MxHttpResponseDestroy(GetResponseRef(transaction));
	
	return status;
}
