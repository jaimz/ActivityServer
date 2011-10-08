//
//  MxIO.c
//  ActivityServer
//
//  Created by J O'Brien on 27/09/2011.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h> // strncmp

#include "MxStatus.h"
#include "MxStringBuffer.h"
#include "MxIO.h"


MxStatus MxIODrainToBuffer(int fd, MxStringBufferRef buffer)
{
	if (buffer == NULL) return MxStatusNullArgument;
	if (fd == -1) return MxStatusIllegalArgument;
    
	MxStatus result = MxStatusOK;
	size_t numRead = 0;
	
	char buf[BUFSIZ];
	
	while ((result == MxStatusOK) && (numRead = read(fd, buf, BUFSIZ)) > 0)
		result = MxStringBufferInsertCharacters(buffer, buf, numRead);
    
	if (result != MxStatusOK)
		return result;
	
	if (numRead == -1)
		return MxStatusReadError;
	
	return MxStatusOK;
}


MxStatus MxIOReadIntoBuffer(int fd, MxStringBufferRef buffer, size_t amount)
{
	if (buffer == NULL)
		return MxStatusNullArgument;
	
	if (fd < 0)
		return MxStatusIllegalArgument;
	
	if (amount == 0)
		return MxStatusOK;
	
	MxStatus result = MxStatusOK;
	char buf[BUFSIZ];
	ssize_t numRead = 0;
	size_t accumRead = 0;
	
	// The number of BUFSIZ sized chunks we can read from the fd...
	size_t iterations = amount / BUFSIZ;
	
	// The amount that will be left over when we've consumed iterations * BUFSIZ bytes
	size_t remainder = amount % BUFSIZ;
	
	if (iterations > 0)
	{
		int i = 0;
		while ((i < iterations) && ((numRead = read(fd, buf, BUFSIZ)) > 0)) {
			result = MxStringBufferInsertCharacters(buffer, buf, numRead);
			if (result != MxStatusOK) break;
			
			accumRead += numRead;
			i++;
		}
		
		if (numRead < 0)
			result = MxStatusReadError;
		else if (numRead == 0 && accumRead != amount)
			result = MxStatusEOF;
	}
	
	if ((result == MxStatusOK) && (remainder > 0))
	{
		if ((numRead = read(fd, buf, remainder)) >= 0)
		{
			result = MxStringBufferInsertCharacters(buffer, buf, numRead);
			accumRead += numRead;
		}
	}
	
	// NOTE: This code seems awfully verbose & redundant - check
	if (numRead == -1)
		result = MxStatusReadError;
	else if (numRead == 0 && accumRead != amount)
		result = MxStatusEOF;
	
	return result;
}

MxStatus MxIOReadUntilBlankLine(int channel, MxStringBufferRef buffer)
{
	MxAssertPointer(buffer);
    
	if (channel < 0)
		return MxStatusIllegalArgument;
	
	MxStatus result = MxStatusOK;
	ssize_t  numRead = 0;
	int done = 0;
	char buf[BUFSIZ];
	while ((!done) && (result == MxStatusOK) && (numRead = read(channel, buf, BUFSIZ)))
	{
		/*
         Penultimate-header: bar\n
         Last-header: foo\n
         \n
		 */
		result = MxStringBufferInsertCharacters(buffer, buf, numRead);
		
		if (result != MxStatusOK)
			break;
		
		/* 
         Check for '\r\n\r\n'
         
         TODO:
         MxStringBufferAsCStr should be pretty cheap since we're
         always appending onto the buffer - may still want to optimise
         this though...
		 */
		size_t len = MxStringBufferGetByteCount(buffer);
        
		if (len >= 4) {
			char *sofar = MxStringBufferAsCStr(buffer);
			
			done = (strncmp((sofar + (len - 4)), "\r\n\r\n", 4) == 0);
		}
		
	}
    
	if (result != MxStatusOK)
        return result;
	
	if (numRead == -1)
		return MxStatusReadError;
	
	return MxStatusOK;
}

MxStatus MxIODrainToChannel(int channel, MxStringBufferRef buffer)
{
	if (buffer == NULL)
		return MxStatusNullArgument;
	
	if (channel < 0)
		return MxStatusIllegalArgument;
	
	char *cBuf = MxStringBufferAsCStr(buffer);
	if (cBuf == NULL)
		return MxStatusOK; // empty string
	
	size_t count = MxStringBufferGetByteCount(buffer);
	if (count == -1)
		return MxStatusUnknownError;
    
	if (write(channel, cBuf, count) == -1)
		return MxStatusWriteError;
    
	return MxStatusOK;
}

MxStatus MxIOWriteToChannel(int channel, MxStringBufferRef buffer, size_t amount)
{
	if (buffer == NULL)
        return MxStatusNullArgument;
    
	if (channel < 0)
        return MxStatusIllegalArgument;
    
	if (amount == 0) return MxStatusOK;
	
	char *cBuf = MxStringBufferAsCStr(buffer);
	if (!cBuf) 
		return MxStatusOK; // empty string
	
	if (write(channel, cBuf, amount) == -1)
		return MxStatusWriteError;
	
	return MxStatusOK;
}

MxStatus MxIOFillBufferFromFile(const char *filename, MxStringBufferRef buffer)
{
	if (buffer == NULL || filename == NULL)
		return MxStatusNullArgument;
	
	int fd = open(filename, O_RDONLY);
	if (fd == -1)
		return MxStatusCouldNotOpen;
	
	MxStatus result = MxIODrainToBuffer(fd, buffer);
	close(fd);
	
	return result;
}