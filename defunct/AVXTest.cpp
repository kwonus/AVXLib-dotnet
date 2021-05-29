// AVXTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include "AVXTest.h"
#include <SearchRequest.h>

void basicActions()
{
	UINT32 wcnt = getWritCnt();
	UINT16 bcnt = getBookCnt();
	UINT32 ccnt = getChapterCnt();
	UINT32 vcnt = getVerseCnt();
	AVBook rev = getBook(65); // Rev
	AVBook john = getBookByNum(43); // John
	AVChapter gen_1 = getChapter(0);
	AVVerse gen_1_1 = getVerse(0);

	std::cout << "\tBook Count:    " << bcnt << "\n";
	std::cout << "\tChapter Count: " << ccnt << "\n";
	std::cout << "\tVerse Count:   " << vcnt << "\n";
	std::cout << "\tWord Count:    " << wcnt << "\n";
}

void lexicalActions()
{
	char* oh = getLexicalEntry(3, MODERN);
	std::cout << "\tModernized word for KJV 'O': " << oh << "\n";

	AVChapter gen_2 = getChapter(1);
	AVWrit* writ = getWrit(gen_2.writIdx);	// Gen 2:1
	char* word = getLexicalEntry(writ->wordKey & 0x7FFF, MODERN);
	std::cout << "\tFirst word of Genesis 2:1: " << word << "\n";
}

void lemmaticActions()
{
	UINT16 size = getLemma(0x00003A1C, 0x027A, NULL, 0);	// find size of array first
	std::cout << "\tCount of lemma for 'Elim': " << size << "\n";
	char* elim[10];
	size = getLemma(0x00003A1C, 0x027A, elim, size);
	std::cout << "\tFirst and only lemma for 'Elim': " << elim[0] << "\n";
}

void demonstrateVerseIdentity(UINT16 verseIdx)
{
	AVVerse v = getVerse(verseIdx);
	AVBook b = getBookByNum(v.book);
	std::cout << "\t";
	std::cout << (char*)&(b.name);
	std::cout << " ";
	std::cout << (int)v.chapter;
	std::cout << ":";
	std::cout << (int)v.verse;
	std::cout << "\n";
}


void demonstrateVerseExpansion(AVWrit* writ)
{
	int w = 0;
	for (UINT16 verse = writ->verseIdx; verse == writ->verseIdx; writ++)
	{
		auto wkey = writ->wordKey;
		auto word = getLexicalEntry(wkey, MODERN);

		if (w++ > 0)
			std::cout << " ";
		std::cout << word;
	}
	std::cout << "\n";
}

void demonstrateFindingWordsInVerses(CLexIndex& clex)
{
	// find Godhead in the NT:
	UINT16 godhead = clex.find("Godhead");
	AVBook& matt = getBookByNum(40);
	AVChapter& matt_1 = getChapter(matt.chapterIdx);
	AVWrit* writ = getWrit(matt_1.writIdx);
	std::cout << "\tSearch New Testament for 'Godhead':\n";
	do
	{
		if ((writ->wordKey & 0x7FFF) == godhead) {
			AVVerse& verse = getVerse(writ->verseIdx);
			AVBook& b = getBookByNum(verse.book);
			std::cout << "\t\t" << (char*)&(b.name) << " " << (int)verse.chapter << ":" << (int)verse.verse << "\n";
		}
		++writ;
	} while (writ->transition != 0xFC);
}

void mainX()
{
	initialize("C:/src/Digital-AV/z-series/");

	std::cout << "Test Basic Actions:\n";
	basicActions();

	std::cout << "\nTest Lexical Actions:\n";
	lexicalActions();

	std::cout << "\nTest Lemma Actions:\n";
	lemmaticActions();

	AVChapter gen_2 = getChapter(1);
	AVWrit* writ = getWrit(gen_2.writIdx);	// Gen 2:1

	std::cout << "\nDemonstrate Verse identity:\n";
	demonstrateVerseIdentity(gen_2.verseIdx);

	std::cout << "\nDemonstrate Verse expansion:\n";
	demonstrateVerseExpansion(writ);

	CLexIndex clex;
	std::cout << "\nDemonstrate finding words in verses:\n";
	demonstrateFindingWordsInVerses(clex);

	release();

	SearchFragment frag;
	frag.positionAspects = { 0 };
	frag.text = "a&b|c";
	SearchClause clause;
	clause.segment = frag.text;
	clause.fragments = { &frag };
	clause.polarity = '+';
	SearchRequest req;
	req.clauses = { &clause };
	req.count = 1;

	auto search = new CSearchRequest(req);
	std::cout << "ok";
}