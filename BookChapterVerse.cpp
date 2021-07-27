#include "BookChapterVerse.h"
#include <AVLCLR.h>

using namespace System;
using namespace System::Linq;

namespace AVXCLI {

	BookChapterVerse::BookChapterVerse() {
		this->Matches = gcnew HashSet<UInt64>();	// this could be managed by Maganimity
		this->Tokens = gcnew Dictionary<UInt32, UInt64>();
	}

	bool BookChapterVerse::AddMatch(UInt16 segmentIdx, UInt32 wstart, UInt32 wlast)
	{
		auto encoded = SegmentElement::Create(wstart, wlast, segmentIdx);
		if (!this->Matches->Contains(encoded))
			this->Matches->Add(encoded);

		return true;
	}
	bool BookChapterVerse::AddMatch(UInt16 segmentIdx, UInt32 wstart, UInt16 wcnt)
	{
		auto encoded = SegmentElement::Create(wstart, wcnt, segmentIdx);
		if (!this->Matches->Contains(encoded))
			this->Matches->Add(encoded);

		return true;
	}
	bool BookChapterVerse::SubtractMatch(UInt32 wstart, UInt32 wlast)
	{
		auto list = gcnew List<UInt64>();

		for each (auto encoded in this->Matches)
		{
			auto estart = SegmentElement::GetStart(encoded);
			auto elast = SegmentElement::GetStart(encoded);

			if (estart >= wstart && estart <= wlast && elast <= wlast && elast >= wstart)
				list->Add(encoded);
		}
		for each (auto encoded in list)
		{
			this->Matches->Remove(encoded);
		}
		return true;
	}
	bool BookChapterVerse::SubstractMatch(UInt32 wstart, UInt16 wcnt)
	{
		auto list = gcnew List<UInt64>();
		UInt32 wlast = wstart + wcnt - 1;

		for each (auto encoded in this->Matches)
		{
			auto estart = SegmentElement::GetStart(encoded);
			auto elast = SegmentElement::GetStart(encoded);

			if (estart >= wstart && estart <= wlast && elast <= wlast && elast >= wstart)
				list->Add(encoded);
		}
		for each (auto encoded in list)
		{
			this->Matches->Remove(encoded);
		}
		return true;
	}

	void BookChapterVerse::SearchClause(QClauseSearch^ clause, QSearchControls^ controls)
	{
		if (clause->quoted)
		{
			if (controls->span == 0)
				this->SearchClauseQuoted_ScopedUsingVerse(clause, controls);
			else
				this->SearchClauseQuoted_ScopedUsingSpan(clause, controls);
		}
		else
		{
			if (controls->span == 0)
				this->SearchClauseUnquoted_ScopedUsingVerse(clause, controls);
			else
				this->SearchClauseUnquoted_ScopedUsingSpan(clause, controls);
		}
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
	UInt32 BookChapterVerse::SearchClauseQuoted_ScopedUsingSpan(QClauseSearch^ clause, QSearchControls^ controls)
	{
		return 0;
	}
	UInt32 BookChapterVerse::SearchClauseQuoted_ScopedUsingVerse(QClauseSearch^ clause, QSearchControls^ controls)
	{
		UInt32 matchCnt = 0;
		UInt64 found = 0;
		auto verseIdx = 0;

		AVSDK::Writ176 prev;
		AVSDK::Writ176 writ;
		UInt32 start = AVMemMap::CURRENT;
		UInt32 end   = AVMemMap::CURRENT;

		UInt32 cursor = AVMemMap::FIRST;

		for (Byte b = 1; b <= 66; b++)
		{
			auto book = AVLCLR::XBook->books[b - 1];
			auto cidx = book.chapterIdx;
			auto ccnt = book.chapterCnt;

			for (auto c = cidx; c < UInt32(cidx + ccnt); c++)
			{
				auto chapter = AVLCLR::XChapter->chapters[c];
				auto until = chapter->writIdx + chapter->wordCnt - 1;
				Byte span;
				for (cursor = chapter->writIdx; cursor <= until; cursor += span)
				{
					start = cursor;
					if (AVLCLR::XWrit->GetRecord(cursor, writ))
					{
						span = AVLCLR::XVerse->GetWordCnt(writ.verseIdx);
						UInt64 required = 0;
						auto matched = false;

						for each (QSearchFragment ^ fragment in clause->fragments) {
							try {
								auto position = this->SearchSequentiallyInSpan(span, fragment);
								matched = (position != 0xFFFFFFFF);
								end = start + position;

								if (!matched)
									break;
							}
							catch (...)
							{
								return 0;
							}
						}
						if (matched) {
							if (clause->polarity == '-')
								this->SubtractMatch(start, end);
							else if (clause->polarity == '+')
								this->AddMatch(clause->index, start, end);

							matchCnt++;
						}
					}
					else
					{
						break;
					}
				}
			}
		}
		return matchCnt;
	}
	UInt32 BookChapterVerse::SearchSequentiallyInSpan(UInt16 span, QSearchFragment^ frag)
	{
		auto cursor = AVLCLR::XWrit->cursor;
		AVSDK::Writ176 writ;
		
		for (Int32 i = 0; i < span; i++) {
			AVLCLR::XWrit->GetRecord(cursor++, writ);
			if (this->IsMatch(writ, frag)) {
				auto existing = this->Tokens->ContainsKey(cursor) ? this->Tokens[cursor] : UInt64(0);
				this->Tokens[cursor] = existing | frag->bit;
				return ++i;
			}
		}
		return 0xFFFFFFFF;
	}
	UInt32 BookChapterVerse::SearchClauseUnquoted_ScopedUsingSpan(QClauseSearch^ clause, QSearchControls^ controls)
	{
		UInt32 matchCnt = 0;
		UInt64 found = 0;
		auto verseIdx = 0;

		UInt32 localspan = 0;
		AVSDK::Writ176 prev;
		AVSDK::Writ176 writ;
		UInt32 cursor = AVMemMap::FIRST;
		UInt32 until = AVMemMap::FIRST;
		UInt32 start = AVMemMap::CURRENT;

		for (Byte b = 1; b <= 66; b++)
		{
			auto book = AVLCLR::XBook->books[b - 1];
			auto cidx = book.chapterIdx;
			auto ccnt = book.chapterCnt;
			auto chapter = AVLCLR::XChapter->chapters[cidx];
			auto chapterLast = AVLCLR::XChapter->chapters[cidx + ccnt - 1];

			cursor = chapter->writIdx;
			until = cursor + chapterLast->writIdx + chapterLast->wordCnt - 1;

			UInt32 start = AVMemMap::CURRENT;
			auto span = controls->span;

			for (bool ok = AVLCLR::XWrit->GetRecord(cursor, writ); ok && (cursor <= until); ok = AVLCLR::XWrit->GetRecord(AVMemMap::NEXT, writ), cursor = AVLCLR::XWrit->cursor)
			{
				localspan = span > 0 ? span : AVLCLR::XVerse->GetWordCnt(writ.verseIdx);
				UInt64 required = 0;
				Byte f = 0;
				for each (QSearchFragment ^ fragment in clause->fragments) {
					UInt64 bits = (0x1 << f++);
					required |= bits;
					bool matched = (this->SearchUnorderedInSpan(localspan, fragment) != 0xFFFFFFFF);
					if (matched)
					{
						found |= bits;
						if (start == AVMemMap::CURRENT)
							start = cursor;
					}
				}
				if (found == required) {
					if (clause->polarity == '-')
						this->SubtractMatch(start, cursor);
					else if (clause->polarity == '+')
						this->AddMatch(clause->index, start, cursor);

					start = AVMemMap::CURRENT;
					found = 0;
					matchCnt++;
				}
				cursor += (localspan - 1);
				AVLCLR::XWrit->SetCursor(cursor);
				AVLCLR::XWrit->GetRecord(AVMemMap::CURRENT, writ);

				// Next 4 lines are a bit of a hack.  Still trying to figure out how cursor gets misaligned
				//while (writ.trans == 0) // Repair bug of overstepping the next verse
					//break;
				if ((writ.trans & (Byte)Transitions::EndOfBook) == (Byte)Transitions::EndOfBook)
					break; // Next bbok
//				if ((Byte)(writ.trans & (Byte) Transitions::ChapterTransition) != Byte(0))
//					break; // Error!!!
			}
		}
		return matchCnt;
	}
	UInt32 BookChapterVerse::SearchClauseUnquoted_ScopedUsingVerse(QClauseSearch^ clause, QSearchControls^ controls)
	{
		UInt32 matchCnt = 0;
		UInt64 found = 0;
		auto verseIdx = 0;

		AVSDK::Writ176 prev;
		AVSDK::Writ176 writ;
		UInt32 start  = AVMemMap::CURRENT;
		UInt32 cursor = AVMemMap::FIRST;

		for (Byte b = 1; b <= 66; b++)
		{
			auto book = AVLCLR::XBook->books[b-1];
			auto cidx = book.chapterIdx;
			auto ccnt = book.chapterCnt;

			for (auto c = cidx; c < UInt32(cidx + ccnt); c++)
			{
				auto chapter = AVLCLR::XChapter->chapters[c];
				auto until = chapter->writIdx + chapter->wordCnt - 1;
				Byte span = 0;
				for (cursor = chapter->writIdx; cursor <= until; cursor += span)
				{
					if (AVLCLR::XWrit->GetRecord(cursor, writ))
					{
						span = AVLCLR::XVerse->GetWordCnt(writ.verseIdx);
						UInt64 required = 0;
						
						for each (QSearchFragment ^ fragment in clause->fragments) {
							UInt64 bit = (0x1 << (fragment->bit-1));
							required |= bit;
							bool matched = (this->SearchUnorderedInSpan(span, fragment) != 0xFFFFFFFF);
							if (matched)
							{
								found |= bit;
								if (start == AVMemMap::CURRENT)
									start = cursor;
							}
						}
						if (found == required) {
							if (clause->polarity == '-')
								this->SubtractMatch(start, cursor);
							else if (clause->polarity == '+')
								this->AddMatch(clause->index, start, cursor);

							start = AVMemMap::CURRENT;
							found = 0;
							matchCnt++;
						}
					}
					else
					{
						break;
					}
				}
			}
		}
		return matchCnt;
	}
	// These methods used to include book, and chapter
	// There is a missing loop that creates the moving span window
	//
	UInt32 BookChapterVerse::SearchUnorderedInSpan(UInt16 span, QSearchFragment^ frag)
	{
		UInt32 cursor = AVLCLR::XWrit->cursor;
		UInt32 last = cursor + span - 1;
		AVSDK::Writ176 writ;

		for (Int32 i = 0; cursor <= last; cursor++, i++) {
			AVLCLR::XWrit->GetRecord(cursor, writ);
			if (this->IsMatch(writ, frag)) {
				if (this->Tokens->ContainsKey(cursor))
					this->Tokens[cursor] |= frag->bit;
				else
					this->Tokens[cursor] = frag->bit;
				return i;
			}
		}
		return 0xFFFFFFFF;
	}
}
