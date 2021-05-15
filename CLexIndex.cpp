#include "CLexIndex.h"
#include <iostream>

CLexIndex::CLexIndex()
{
	for (int i = 0; i < 19; i++)
		this->index[i] = 0; // 0 means no entry

	int size = 1;
	UINT16 start = 0;
	for (UINT16 key = 1; key <= 12567; key++) {
		UINT16 idx = key - 1;
		char* test = getLexicalEntry(key, SEARCH);
		if (test == NULL) {
			std::cout << key;
			std::cout << " not found\n";
			continue;
		}
		int len = strlen(test);
		if (len == size)
			this->index[size++] = key;
		else if (len < size-1) {
			std::cout << test;
			std::cout << ": bad [always increasing] size assumption\n";
		}
		if (size > 18)
			break;
	}
}

const UINT16 CLexIndex::find(const char* word)
{
	int size = strlen(word);
	int limit = size < 18 ? this->index[size+1] - 1 : 12567;
	for (int key = this->index[size]; key <= limit; key++) {
		char* test = getLexicalEntry(key, SEARCH);
		if (strmatch(word, test))
			return key;
	}
	return (UINT16) 0;
};
