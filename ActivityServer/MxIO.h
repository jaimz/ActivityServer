//
//  MxIO.h
//  ActivityServer
//
//  Utility IO functions
//
//  Created by J O'Brien on 27/09/2011.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#ifndef ActivityServer_MxIO_h
#define ActivityServer_MxIO_h

#include "MxStatus.h"
#include "MxStringBuffer.h"

// Copy characters from the given fd to the given buffer until we receive EOF on the fd
MxStatus MxIODrainToBuffer(int fd, MxStringBufferRef buffer);

// Copy 'amount' characters from the given fd to the given buffer. Will return MxStatusEOF
// if we reach EOF before n characters are read.
MxStatus MxIOReadIntoBuffer(int fd, MxStringBufferRef buffer, size_t amount);

MxStatus MxIOReadUntilBlankLine(int fd, MxStringBufferRef buffer);

// Copy everything in the given buffer to the given fd. 
MxStatus MxIODrainToChannel(int channel, MxStringBufferRef buffer);

// Write n characters from the given buffer to the given fd.
MxStatus MxIOWriteToChannel(int fd, MxStringBufferRef buffer, size_t amount);

// Read the contents of the given filen into the given buffer.
MxStatus MxIOFillBufferFromFile(const char *filename, MxStringBufferRef buffer);

#endif
