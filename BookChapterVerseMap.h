#ifndef BOOK_CHAPTER_VERSE_MAP_H 
#define BOOK_CHAPTER_VERSE 

#include <XBitArray.h> 
#include <unordered_map>

class BookChapterVerseMap
{
protected:
	std::unordered_map<BYTE, std::unordered_map<BYTE, UINT16*>&> bcv;
public:
	BookChapterVerseMap();
	virtual ~BookChapterVerseMap() {
		for (auto bk = this->bcv.begin(); bk != this->bcv.end(); ++bk) {
			for (auto ch = bk->second.begin(); ch != bk->second.end(); ++ch)
				free(ch->second);
			bk->second.clear();
		}
		this->bcv.clear();
	}
	bool AddVerse(BYTE b, BYTE c, BYTE v);
};

#endif

