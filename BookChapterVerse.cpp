#include "BookChapterVerse.h"

BookChapterVerse::BookChapterVerse() {
	this->bcv = gcnew Dictionary<Byte, Dictionary<Byte, array<UInt16>^>^>();
}

bool BookChapterVerse::AddVerse(BYTE b, BYTE c, BYTE v) {
	auto book = this->bcv->ContainsKey(b) ? this->bcv[b] : nullptr;
	if (book == nullptr) {
		book = gcnew Dictionary<Byte, array<UInt16>^>();
		this->bcv[b] = book;
	}
	auto chapter = book->ContainsKey(c) ? book[c] : nullptr;

	if (chapter == nullptr) {
		XBitArray255 verses;
		verses.SetBit(v);

		UINT16 compactedVerses[2];
		if (verses.GetCompactBitArray(compactedVerses, 2)) {
			chapter = gcnew array<UInt16>(2) { compactedVerses[0], compactedVerses[1] };
			book[c] = chapter;
			return true;
		}
	}
	else {
		UINT16 compactedVerses[17];
		int i = 0;
		for each (auto segment in chapter) {
			compactedVerses[i++] = segment;
		}
		XBitArray255 verses(compactedVerses);
		verses.SetBit(v);

		auto cnt = verses.GetCompactBitArray(compactedVerses, 17);
		auto oldCnt = 1 + XBitArray255::CountBits(compactedVerses[0]);
		if (oldCnt == cnt) {
			for (int c = 0; c < cnt; c++)
				chapter[c] = compactedVerses[c];
			return true;
		}
		else {
			chapter = gcnew array<UInt16>(cnt+1);
			for (int c = 0; c <= cnt; c++)
				chapter[c] = compactedVerses[c];
			book[c] = chapter;
			return true;
		}
	}
	return false;
}
