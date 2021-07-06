#include "BookChapterVerse.h"
#include <AVLCLR.h>

namespace AVXCLI {

	BookChapterVerse::BookChapterVerse() : Dictionary<Byte, Dictionary<Byte, UInt32>^>() {
		this->Matched = gcnew array<UInt64>(0xC0C93);	// this needs to be managed by Maganimity
	}

	bool BookChapterVerse::AddChapter(Byte b, Byte c) {
		if (b < 1 || b > 66 || c < 1)
			return false;

		auto book = this->ContainsKey(b) ? this[b] : nullptr;
		if (book == nullptr) {
			book = gcnew Dictionary<Byte, UInt32>();
			this[b] = book;
		}
		else if (book->ContainsKey(c))
			return true;

		AVSDK::Book^ bk = AVLCLR::GetBookByNum(b);
		if (c > bk->chapterCnt)
			return false;
		AVSDK::Chapter ^ ch = AVLCLR::XChapter->chapters[bk->chapterIdx + c - 1];

		book[c] = ch->writIdx;

		return true;
	}

	bool BookChapterVerse::SubtractChapter(Byte b, Byte c, Byte v) {
		if (b < 1 || b > 66 || c < 1 || v < 1)
			return false;

		auto book = this->ContainsKey(b) ? this[b] : nullptr;
		if (book == nullptr) {
			return true;
		}
		else if (!book->ContainsKey(c))
			return true;

		auto bk = AVLCLR::GetBookByNum(b);
		if (c > bk->chapterCnt)
			return false;
		auto ch = AVLCLR::XChapter->chapters[bk->chapterIdx + c - 1];

		if (v > ch->wordCnt)
			return false;

		auto vs = AVLCLR::XVerse->GetVerse(ch->verseIdx + v - 1);
		auto writ = AVLCLR::XWrit->SetCursor(ch->writIdx);

		bool keep = false;
		UInt32 last = ch->writIdx + ch->wordCnt - 1;
		for (UInt32 writIdx = ch->writIdx; writIdx <= last; writIdx++) {
			if ((this->Matched[writIdx] & 0x00FFFFFFFFFFFFFF) != 0 && (this->Matched[writIdx] & 0xC000000000000000) == 0x4000000000000000)
				keep = false;
		}
		if (!keep) {
			book->Remove(c);
			if (book->Count < 1)
				this->Remove(b);
		}
		return !keep;
	}

	UInt32 BookChapterVerse::GetVerseCount(Byte b, Byte c) {
		if (b < 1 || b > 66 || c < 1)
			return 0;

		auto book = this->ContainsKey(b) ? this[b] : nullptr;
		if (book == nullptr) {
			return 0;
		}
		else if (!book->ContainsKey(c))
			return 0;

		return 0;
	}
	Dictionary<Byte, UInt32>^ BookChapterVerse::GetVerses(Byte b, Byte c) { // hash is verse-num to writIdx for verse
		if (b < 1 || b > 66 || c < 1)
			return nullptr;

		auto book = this->ContainsKey(b) ? this[b] : nullptr;
		if (book == nullptr) {
			return nullptr;
		}
		else if (!book->ContainsKey(c))
			return nullptr;

		return nullptr;
	}

	void BookChapterVerse::SearchClause(Byte b, Byte c, QClauseSearch^ clause, QSearchControls^ controls)
	{
		auto list = gcnew BookChapterVerse();

		if (clause->quoted)
			this->SearchClauseQuoted(b, c, clause, controls);
		else
			this->SearchClauseUnquoted(b, c, clause, controls);
	}
	bool BookChapterVerse::IsMatch(const AVSDK::Writ176 const% writ, Feature^ feature)
	{
		if (feature->discretePOS != 0)
			return feature->not == false
			? writ.pos == feature->discretePOS
			: writ.pos != feature->discretePOS;

		bool slashes = feature->feature->StartsWith("/") && feature->feature->EndsWith("/") && (feature->feature->Length > 2);
		String^ token = slashes
			? feature->feature->Substring(1, feature->feature->Length - 2)
			: feature->feature;

		bool not = feature->not;

		if (slashes) {
			switch (feature->subtype)
			{
			case SLASH_BitwisePOS:	for each (UInt16 value in feature->featureMatchVector)
				if ((value & writ.pnwc) == value)
					return !not;
				return not;
			case SLASH_Puncuation:	for each (UInt16 value in feature->featureMatchVector)
				if ((value & writ.punc) == value)
					return !not;
				return not;
			case SLASH_Boundaries:	for each (UInt16 value in feature->featureMatchVector)
				if ((value & writ.trans) == value)
					return !not;
				return not;
			case SLASH_RESERVED_80:
			case SLASH_RESERVED_40:
			case SLASH_RESERVED_20:
			case SLASH_RESERVED_10:
			default:			return not;
			}
		}
		else
		{
			// incomplete!!!
			switch (feature->type & (FIND_Token | FIND_Lemma | FIND_GlobalTest | FIND_LANGUAGE_NUMERIC))
			{
				// TODO: add suffix support:
				/*
				* FIND_Suffix_MASK
				* FIND_Suffix_Exact
				* FIND_Suffix_Modern
				* FIND_Suffix_Either
				* FIND_Suffix_None
				*/
			case FIND_Token:	for each (UInt16 value in feature->featureMatchVector)
				if (value == (writ.word & 0x3FFF))
				{
					//											Console::Out->WriteLine("Found: " + feature->feature);
					return !not;
				}
						   return not;
			case FIND_English:	for each (UInt16 value in feature->featureMatchVector)
				if (value == (writ.word & 0x3FFF))
					return !not;
				return not;

			case FIND_Hebrew:	if (AVLCLR::XVerse->GetBook(writ.verseIdx) <= 39) {
				int i = 0;
				auto strongs = (UINT16*)writ.strongs;
				for (auto strongs = (UINT16*)writ.strongs; *strongs != 0; strongs++)
					for each (UInt16 value in feature->featureMatchVector)
					{
						if (value == (UInt16)*strongs)
							return !not;
						if (++i == 4)
							return not;
					}
			}
							return not;
			case FIND_Greek:	if (AVLCLR::XVerse->GetBook(writ.verseIdx) >= 40) {
				int i = 0;
				auto strongs = (UINT16*)writ.strongs;
				for (auto strongs = (UINT16*)writ.strongs; *strongs != 0; strongs++)
					for each (UInt16 value in feature->featureMatchVector)
					{
						if (value == (UInt16)*strongs)
							return !not;
						if (++i == 3)
							return not;
					}
			}
						   return not;
			case FIND_Lemma:	for each (UInt16 value in feature->featureMatchVector)
				if (value == writ.lemma)
					return !not;
				return not;
			case FIND_GlobalTest: // #diff or #same // TODO: determine which one

				/*case typeWordSame:*/ {
				UINT16 key = writ.word & 0x3FFF;
				auto srch = getLexicalEntry(key, SEARCH);
				auto mdrn = getLexicalEntry(key, MODERN);
				auto disp = getLexicalEntry(key, DISPLAY);
				if (srch == NULL)
					return not;
				else
					return mdrn == disp ? !not : not;
			}
			}
		}
		return false;
	}
	bool BookChapterVerse::IsMatch(const AVSDK::Writ176 const% writ, QSearchFragment^ frag)
	{
		for each (auto spec in frag->specifications) {
			bool matchedAny = false;
			for each (auto feature in spec->matchAny) {
				bool matchedAll = true;
				for each (auto feature in feature->features) {
					matchedAll = this->IsMatch(writ, (Feature^)feature);
					if (!matchedAll)
						break;
				}
				matchedAny = matchedAll;
			}
			if (matchedAny)
				return true;
		}
		return false;
	}
	bool BookChapterVerse::SearchClauseQuoted(Byte b, Byte c, QClauseSearch^ clause, QSearchControls^ controls)
	{
		if (b < 1 || b > 66 || c < 1)
			return false;

		auto book = AVLCLR::GetBookByNum(b);
		if (c > book->chapterCnt)
			return false;

		auto chapter = AVLCLR::Chapters[book->chapterIdx + c - 1];

		bool found = false;
		UInt16 span = controls->span;
		UInt32 cursor = chapter->writIdx;
		AVSDK::Writ176 writ;

		while (AVLCLR::XWrit->GetRecord(cursor++, writ))
		{
			auto verseWordCnt = AVLCLR::XVerse->GetWordCnt(writ.verseIdx);
			bool matched = true;
			UInt16 currentspan = (span == 0) ? verseWordCnt : span;
			for each (QSearchFragment ^ fragment in clause->fragments) {
				try {
					UInt64 bits = fragment->bit + (0x1 << clause->index);
					auto position = this->SearchSequentiallyInSpan(bits, cursor, currentspan, fragment);
					matched = (position > 0);
					cursor += position > 0 ? position : currentspan;
					if (!matched)
						break;
				}
				catch (...)
				{
					return false;
				}
			}
			if (matched) {
				Byte begin = AVLCLR::XVerse->GetVerse(writ.verseIdx);
			/* TODO:
			if (matched) {
				UINT32 begin = *((UINT32*)&verse);
				list->Add(begin);
				verse = getVerse(writ[cursor].verseIdx);
				UINT32 end = *((UINT32*)&verse);
				if (end != begin)
					list->Add(end);
			}
			
			if (span == 0 && matched) { // advance to the next verse
				while (writ[cursor - chapter->writIdx].trans & 0x30 != 0x20)
					cursor++;
			*/
			}
		}
		return found;
	}
	Int32 BookChapterVerse::SearchSequentiallyInSpan(UInt64 bits, UInt32 writIdx, UInt16 span, QSearchFragment^ frag)
	{
		auto cursor = writIdx;
		AVSDK::Writ176 writ;
		
		for (Int32 i = 0; i < span; i++) {
			AVLCLR::XWrit->GetRecord(cursor++, writ);
			if (this->IsMatch(writ, frag)) {

				this->Matched[writIdx] |= bits;
				return ++i;
			}
		}
		return 0;
	}
	bool BookChapterVerse::SearchClauseUnquoted(Byte b, Byte c, QClauseSearch^ clause, QSearchControls^ controls)
	{
		if (b < 1 || b > 66 || c < 1)
			return false;

		auto book = AVLCLR::GetBookByNum(b);
		if (c > book->chapterCnt)
			return false;

		auto chapter = AVLCLR::Chapters[book->chapterIdx + c - 1];

		UInt64 found = 0;
		UInt16 span = controls->span;
		auto cursor = chapter->writIdx;
		UInt32 localspan = 0;
		AVSDK::Writ176 prev;
		AVSDK::Writ176 writ;
		while (AVLCLR::XWrit->GetRecord(cursor++, writ))
		{
			UInt64 required = 0;
			Byte f = 0;
			for each (QSearchFragment ^ fragment in clause->fragments) {
				UInt64 bits = (0x1 << f++) | (0x1 << 48 + clause->index);
				required |= bits;
				bool matched = (this->SearchUnorderedInSpan(bits, chapter->writIdx, localspan, fragment) >= 0);
				if (matched)
					found |= bits;
			}
			if (found == required) {
				Byte begin = AVLCLR::XVerse->GetVerse(writ.verseIdx);
				// And segment bits and polarity bits:
				found |= (0x1 << (48 + clause->index));
				if (clause->polarity == '-') {
					found <<= (0x1 << 63);
					this->SubtractChapter(b, c, begin);
				}
				else if (clause->polarity == '+') {
					found <<= (0x1 << 62);
					this->AddChapter(b, c);
				}
			}
			cursor += (localspan-1);
			if (!AVLCLR::XWrit->GetRecord(cursor, prev))
				break;
			if (!AVLCLR::XWrit->GetRecord(cursor, writ))
				break;

			// Next 4 lines are a bit of a hack.  Still trying to figure out how cursor gets misaligned
			while (writ.trans == 0) // Repair bug of overstepping the next verse
				if (!AVLCLR::XWrit->GetRecord(cursor, writ))
					break;
			if ((writ.trans & 0xFC) == 0xFC) // EoBible
				break;
			/*
			 *		THIS WOULD BE SLIGHTLY FASTER, BUT WOULD NOT MARKS ALL MATCHES
			 *
			 *		if (span == 0 && matched) { // advance to the next verse
			 *			while ((cursor->transition & 0x30) != 0x20)
			 *				cursor++;
			 *		}
			*/
			if ((Byte)(writ.trans & (Byte)AVSDK::Transitions::ChapterTransition) != Byte(0))
				break;
		}
		return found;
	}
	Int32 BookChapterVerse::SearchUnorderedInSpan(UInt64 bits, UInt32 writIdx, UInt16 span, QSearchFragment^ frag)
	{
		auto cursor = writIdx;
		AVSDK::Writ176 writ;

		for (Int32 i = 0; i < span; i++) {
			AVLCLR::XWrit->GetRecord(cursor++, writ);
			if (this->IsMatch(writ, frag)) {

				this->Matched[writIdx] |= bits;
				return ++i;
			}
		}
		return 0;
	}
}
