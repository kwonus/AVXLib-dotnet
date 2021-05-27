#include <BookChapterVerseMap.h>

BookChapterVerseMap::BookChapterVerseMap() {
	;
}

bool BookChapterVerseMap::AddVerse(BYTE b, BYTE c, BYTE v) {

	auto book = this->bcv.count(b) != 0 ? &(this->bcv.at(b)) : NULL;
	if (book == NULL) {
		book = new std::unordered_map<BYTE, UINT16*>();
		this->bcv.insert({ b, *book });
	}
	auto chapter = book->count(c) != 0 ? book->at(c) : NULL;

	
	if (chapter == NULL) {
		XBitArray255 verses;
		verses.SetBit(v);

		UINT16* bits = (UINT16*)malloc(2 * sizeof(UINT16));
		verses.GetCompactBitArray(bits, 2);
		book->insert({ c, bits });
	}
	else {
		XBitArray255 verses(chapter);
		verses.SetBit(v);
		UINT16 compactedVerses[17];
		auto cnt = verses.GetCompactBitArray(compactedVerses, 17);
		auto oldCnt = 1 + XBitArray255::CountBits(*chapter);
		if (oldCnt == cnt) {
			for (int c = 0; c < cnt; c++)
				chapter[c] = compactedVerses[c];
		}
		else {
			free(chapter);
			chapter = (UINT16*)malloc(cnt * sizeof(UINT16));
			(*book)[c] = chapter;
		}
	}
	return false;
}
