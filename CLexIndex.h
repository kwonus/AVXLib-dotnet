#ifndef CLEXINDEX_HEADER
#define CLEXINDEX_HEADER
#include "avx.h"

class CLexIndex
{
public:
	CLexIndex();
	const UINT16 find(const char* word);
private:
	UINT16 index[19];
};

inline bool strmatch(const char* w, const char* t) {
	for (/**/; *w && *t; w++, t++)
		if (tolower(*w) != tolower(*t))
			return false;
	return *w == *t;
}
#endif

