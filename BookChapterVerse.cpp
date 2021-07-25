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
			this->SearchClauseQuoted(clause, controls);
		else
			this->SearchClauseUnquoted(clause, controls);
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
	bool BookChapterVerse::SearchClauseQuoted(QClauseSearch^ clause, QSearchControls^ controls)
	{
		bool found = false;
		UInt16 span = controls->span;
		UInt32 cursor = 0;
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
					matched = (position != 0xFFFFFFFF);
					cursor += position != 0xFFFFFFFF ? position : currentspan;
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
	UInt32 BookChapterVerse::SearchSequentiallyInSpan(UInt64 bits, UInt32 writIdx, UInt16 span, QSearchFragment^ frag)
	{
		auto cursor = writIdx;
		AVSDK::Writ176 writ;
		
		for (Int32 i = 0; i < span; i++) {
			AVLCLR::XWrit->GetRecord(cursor++, writ);
			if (this->IsMatch(writ, frag)) {
				auto existing = this->Tokens->ContainsKey(writIdx) ? this->Tokens[writIdx] : UInt64(0);
				this->Tokens[writIdx] = existing | bits;
				return ++i;
			}
		}
		return 0xFFFFFFFF;
	}
	bool BookChapterVerse::SearchClauseUnquoted(QClauseSearch^ clause, QSearchControls^ controls)
	{
		UInt64 found = 0;
		UInt16 span = controls->span;
		auto cursor = 0;
		auto verseIdx = 0;

		UInt32 localspan = 0;
		AVSDK::Writ176 prev;
		AVSDK::Writ176 writ;
		while (AVLCLR::XWrit->GetRecord(cursor++, writ))
		{
			localspan = span > 0 ? span : AVLCLR::XVerse->GetWordCnt(writ.verseIdx);
			UInt64 required = 0;
			Byte f = 0;
			for each (QSearchFragment ^ fragment in clause->fragments) {
				UInt64 bits = (0x1 << f++) | (0x1 << 48 + clause->index);
				required |= bits;
				bool matched = (this->SearchUnorderedInSpan(bits, cursor, localspan, fragment) >= 0);
				if (matched)
					found |= bits;
			}
			if (found == required) {
				Byte begin = AVLCLR::XVerse->GetVerse(writ.verseIdx);
				// And segment bits and polarity bits:
				if (clause->polarity == '-') {
					this->SubtractMatch(cursor, localspan);
				}
				else if (clause->polarity == '+') {
					this->AddMatch(clause->index, cursor, localspan);
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
			if ((Byte)(writ.trans & (Byte)AVSDK::Transitions::ChapterTransition) != Byte(0))
				break;
		}
		return found;
	}
	// These methods used to include book, and chapter
	// There is a missing loop that creates the moving span window
	//
	UInt32 BookChapterVerse::SearchUnorderedInSpan(UInt64 bits, UInt32 writIdx, UInt16 span, QSearchFragment^ frag)
	{
		UInt32 cursor = writIdx;
		UInt32 last = cursor + span - 1;
		AVSDK::Writ176 writ;

		for (Int32 i = 0; cursor <= last; cursor++, i++) {
			AVLCLR::XWrit->GetRecord(cursor, writ);
			if (this->IsMatch(writ, frag)) {
				if (this->Tokens->ContainsKey(cursor))
					this->Tokens[cursor] |= bits;
				else
					this->Tokens[cursor] = bits;
				return i;
			}
		}
		return 0xFFFFFFFF;
	}
}
