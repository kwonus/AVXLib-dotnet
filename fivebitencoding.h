#ifndef ENCODING_5_BIT_H 
#define ENCODING_5_BIT_H 

#include <XVMem_platform.h> 

UINT32 EncodePOS(char* input7charsMaxWithHyphen); // input string must be ascii (max 6 chars; or 7 with hyphen in position 1 through 3)
char* DecodePOS(UINT32 encoding);
UINT64 Hash64(char* nullTerminatedHyphensRemoved); // input string must be ascii lowercase with hyphens-removed (reliability in question when string exceeds length 10
char* getHashedString(UINT64 hash);
UINT64 HashTrivial(char* c); // input string must be ascii and cannot exceed 8
int getHashedTrivialString(UINT64 hash, char* buffer, int len);

#endif
