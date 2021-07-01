#include "AVLCLR.h"
#include <BookChapterVerse.h>
#include <AVXSearchResult.h>

namespace AVXCLI {

	char* AVLCLR::StrToChr(String^ str, char* chr, int len)
	{
		if (len >= str->Length + 1) {
			for (int i = 0; i < str->Length; i++)
				chr[i] = (char)str[i];
			chr[str->Length] = char(0);
			return chr;
		}
		return NULL;
	}
	Tuple<List<UInt16>^, Byte, Byte, UInt32, String^>^ AVLCLR::EncodeAny(String^ token)
	{
		Tuple<List<UInt16>^, Byte, Byte, String^>^ result;
		Tuple<Byte, Byte, UInt32>^ result32;
		UInt32 discretePOS = 0;
		if (token->StartsWith("/") && token->EndsWith("/")) {
			if (token->Length > 2) {
				String^ test = token->Substring(0, token->Length - 2);
				UINT64 hash = this->Encode64(test);

				result = hash != 0 ? EncodeBoundary(hash) : nullptr;
				if (result == nullptr || (result->Item3 == 0 && result->Item4 != nullptr))
					result = EncodePunctuation(hash);
				if (result == nullptr || (result->Item3 == 0 && result->Item4 != nullptr))
					result = EncodeBitwisePOS(test);
				if (result == nullptr || (result->Item3 == 0 && result->Item4 != nullptr)) {
					result32 = EncodeDiscretePOS(test);
					if (result32 != nullptr) {
						discretePOS = result32->Item3;
						result = gcnew Tuple<List<UInt16>^, Byte, Byte, String^>(nullptr, result32->Item1, result32->Item2, nullptr);
					}
				}
			}
		}
		else if (token->StartsWith("#"))
		{
			UINT64 hash = this->Encode64(token);
			result = hash != 0 ? EncodeGlobalTest(hash) : nullptr;
			if (result == nullptr || (result->Item2 == 0 && result->Item4 != nullptr))
				result = EncodeLemma(token);
			if (result == nullptr || (result->Item2 == 0 && result->Item4 != nullptr))
				result = EncodeLanguageNumeric(token);
		}
		else
		{
			result = EncodeWord(token);
		}
		return gcnew Tuple<List<UInt16>^, Byte, Byte, UInt32, String^>(result->Item1, result->Item2, result->Item3, discretePOS, result->Item4);
	}
	Tuple<List<UInt16>^, Byte, Byte, String^>^ AVLCLR::EncodeGlobalTest(UINT64 hash)
	{
		String^ error = nullptr;
		Byte tokenType = 0;
		Byte otherType = 0;
		auto list = gcnew List<UInt16>();

		if (globalMap.count(hash) != 0)
		{
			list->Add(globalMap.at(hash));
			tokenType = FIND_GlobalTest;
		}
		auto result = gcnew Tuple<List<UInt16>^, Byte, Byte, String^>(list, tokenType, otherType, error);
		return result;
	}
	Tuple<List<UInt16>^, Byte, Byte, String^>^ AVLCLR::EncodeBoundary(UINT64 hash)
	{
		String^ error = nullptr;
		Byte tokenType = 0;
		Byte otherType = 0;
		auto list = gcnew List<UInt16>();

		if (globalMap.count(hash) != 0)
		{
			list->Add(globalMap.at(hash));
			tokenType = FIND_GlobalTest;
		}
		auto result = gcnew Tuple<List<UInt16>^, Byte, Byte, String^>(list, tokenType, otherType, error);
		return result;
	}
	Tuple<List<UInt16>^, Byte, Byte, String^>^ AVLCLR::EncodePunctuation(UINT64 hash)
	{
		String^ error = nullptr;
		Byte tokenType = 0;
		Byte otherType = 0;
		auto list = gcnew List<UInt16>();

		if (punctuationMap.count(hash) != 0)
		{
			list->Add(punctuationMap.at(hash));
			otherType = SLASH_Puncuation;
		}
		auto result = gcnew Tuple<List<UInt16>^, Byte, Byte, String^>(list, tokenType, otherType, error);
		return result;
	}
	Tuple<List<UInt16>^, Byte, Byte, String^>^ AVLCLR::EncodeLanguageNumeric(String^ token)
	{
		auto list = gcnew List<UInt16>();

		if (token->StartsWith("#") && token->Length >= 3)
		{
			BYTE lang = 0;
			switch (tolower((char)token[token->Length - 1]))
			{
			case 'e': lang = FIND_English; break;
			case 'h': lang = FIND_Hebrew; break;
			case 'g': lang = FIND_Greek; break;
			}
			if (lang != 0) {
				for (int i = 1; i < token->Length - 1; i++)
					if (token[i] < '0' || token[i] > '9')
						return nullptr;
				list->Add(UInt16::Parse(token->Substring(1, token->Length)));
				auto result = gcnew Tuple<List<UInt16>^, Byte, Byte, String^>(list, lang, 0, nullptr);
				return result;
			}
		}
		return nullptr;
	}
	Tuple<Byte, Byte, UInt32>^ AVLCLR::EncodeDiscretePOS(String^ token)
	{
		auto list = gcnew List<UInt16>();

		auto hyphen1 = token->IndexOf('-');
		auto hyphen2 = token->LastIndexOf('-');
		auto len = token->Length;
		if (len > 7)
			return nullptr;
		if (hyphen1 < 0 && len > 6)
			return nullptr;
		if (hyphen1 > 0 && hyphen1 != hyphen2)
			return nullptr;
		if (hyphen1 > 3)
			return nullptr;
		char buffer[8];
		int nums = 0;
		int letters = 0;
		for (int i = 0; i < len; i++) {
			char c = tolower((char)token[i]);
			if (c >= '1' && c <= '2')
				nums++;
			else if (c >= 'a' && c <= 'z')
				letters++;
			else if (c != '-')
				return nullptr;
		}
		if (nums > 1 || letters < 1)
			return nullptr;
		auto posHash = EncodePOS(StrToChr(token, buffer, 8));
		auto result = gcnew Tuple<Byte, Byte, UInt32>(0, SLASH_DiscretePOS, posHash);
		return result;
	}
	Tuple<List<UInt16>^, Byte, Byte, String^>^ AVLCLR::EncodeBitwisePOS(String^ token)
	{
		auto list = gcnew List<UInt16>();

		auto len = token->Length;
		if (len != 4)
			return nullptr;
		UInt16 bits = 0;
		for (int i = 0; i < 4; i++) {
			bits <<= 4;
			char c = tolower((char)token[i]);
			if (c != '-' || c != '0') {
				if (c >= 1 && c <= 9)
					bits += (c - '0');
				else if (c >= 'a' && c <= 'f')
					bits += (10 + (c - 'a'));
				else
					return nullptr;
			}
		}
		list->Add(bits);
		auto result = gcnew Tuple<List<UInt16>^, Byte, Byte, String^>(list, SLASH_BitwisePOS, 0, nullptr);
		return result;
	}
	Tuple<List<UInt16>^, Byte, Byte, String^>^ AVLCLR::EncodeLemma(String^ token)
	{
		if (!token->StartsWith("#")) {
			return nullptr;
		}
		String^ error = nullptr;
		Byte tokenType = FIND_Lemma;
		Byte otherType = 0;
		auto list = gcnew List<UInt16>();

		UINT64 test = this->Hash64(token);
		if (this->reverseLemma.count(test) != 0) {
			auto head = this->reverseLemma.at(test);
			list->Add(head->value);
			for (BucketOverflow* next = head->overflow; next != NULL; next = next->next)
				list->Add(next->value);
		}
		else {
			error = gcnew String("Token appears to represent a lemma, but the lemma was not found.");
		}
		auto result = gcnew Tuple<List<UInt16>^, Byte, Byte, String^>(list, tokenType, otherType, error);
		return result;
	}

	Tuple<List<UInt16>^, Byte, Byte, String^>^ AVLCLR::EncodeWord(String^ token)
	{
		auto list = gcnew List<UInt16>();

		String^ error = nullptr;
		BYTE tokenType = FIND_Token;
		BYTE otherType = 0;

		bool good = true;
		auto pound = token->IndexOf('#');
		String^ word;
		if (pound >= 0) {
			good = false;
			auto chk = this->Encode64(token->Substring(pound));
			good = suffixMap.count(chk);
			if (good)
				tokenType |= suffixMap.at(chk);
			else
				error = "A suffix operator is apperently being applied to the word toke, but '" + token->Substring(pound) + "' is an unknown suffix.";
			word = pound > 0 ? token->Substring(0, pound) : "";
		}
		else word = token;
		if (good) {
			auto star = word->IndexOf('*');
			if (star >= 0)
			{
				tokenType |= FIND_Token_WithWildcard;

				auto hyphen = word->IndexOf('-');
				String^ test = hyphen >= 0 ? word->Replace("-", "")->ToLower() : word->ToLower();
				star = test->IndexOf('*');

				int len = test->Length;
				String^ start = star > 0 ? test->Substring(0, star) : nullptr;
				String^ end = star < len - 1 ? test->Substring(star + 1) : nullptr;

				switch ((BYTE)tokenType & FIND_Suffix_MASK) {
				case FIND_Suffix_Modern:	EncodeWordSearchWildcard(list, start, end);
					break;
				case FIND_Suffix_Either:	EncodeWordModernWildcard(list, start, end);
				case FIND_Suffix_Exact:
				default:					EncodeWordSearchWildcard(list, start, end);
				}
				if (list->Count == 0)
					error = "No matches found with wildcard: " + token;
			}
			else
			{
				UINT64 hash = this->Hash64(token); /// <--- Error occurs here
				switch ((BYTE)tokenType & FIND_Suffix_MASK) {
				case FIND_Suffix_Modern:	EncodeWordSearch(list, hash);
					break;
				case FIND_Suffix_Either:	EncodeWordModern(list, hash);
				case FIND_Suffix_Exact:
				default:					EncodeWordSearch(list, hash);
				}
				if (list->Count == 0)
					error = "Token appears to represent a word, but the word was not found: " + token;
			}
		}
		return gcnew Tuple<List<UInt16>^, Byte, Byte, String^>(list, tokenType, otherType, error);
	}
	void AVLCLR::EncodeWordModernWildcard(List<UInt16>^ list, String^ start, String^ end)
	{
		char token[17];
		for (auto it = this->reverseModern.begin(); it != this->reverseModern.end(); ++it) {
			auto hashed = it->first;
			auto found = Decode(hashed, token, 17);
			if ((found > 0)
				&& (start == nullptr || StartsWith(token, start))
				&& (end == nullptr || EndsWith(token, end))) {
				auto bucket = it->second;
				list->Add(bucket->value);
				for (BucketOverflow* next = bucket->overflow; next != NULL; next = next->next)
					list->Add(next->value);
			}
		}
	}
	void AVLCLR::EncodeWordSearchWildcard(List<UInt16>^ list, String^ start, String^ end)
	{
		char token[17];
		for (auto it = this->reverseSearch.begin(); it != this->reverseSearch.end(); ++it) {
			auto hashed = it->first;
			auto found = Decode(hashed, token, 17);
			if ((found > 0)
				&& (start == nullptr || StartsWith(token, start))
				&& (end == nullptr || EndsWith(token, end)))
				list->Add(it->second);
		}
	}
	void AVLCLR::EncodeWordSearch(List<UInt16>^ list, UINT64 word)
	{
		if (this->reverseSearch.count(word) != 0)
		{
			list->Add(this->reverseSearch.at(word));
		}
	}
	void AVLCLR::EncodeWordModern(List<UInt16>^ list, UINT64 word)
	{
		if (this->reverseModern.count(word) != 0) {
			auto head = this->reverseModern.at(word);
			list->Add(head->value);
			for (BucketOverflow* next = head->overflow; next != NULL; next = next->next)
				list->Add(next->value);
		}
	}
	IQuelleSearchResult^ AVLCLR::CompileSearchRequest(QRequestSearch^ request) {
		auto result = gcnew AbstractQuelleSearchResult();
		auto additions = gcnew List<AVXSearchResult^>();
		auto subtractions = gcnew List<AVXSearchResult^>();
		for each (auto clause in request->clauses) {
#ifdef AVX_EXTRA_DEBUG_DIAGNOSTICS
			Console::Out->WriteLine(clause->segment + ": (compilation)");
#endif
			for each (auto fragment in clause->fragments) {
				if (fragment->text->StartsWith("|") || fragment->text->EndsWith("|")) {
					result->AddError("The '|' logical-or operator cannot be used without left and right operands");
					return result;
				}
				else if (fragment->text->StartsWith("&") || fragment->text->EndsWith("&")) {
					result->AddError("The '&' logical operator-and cannot be used without left and right operands");
					return result;
				}
				else if (fragment->text == nullptr || fragment->text->Length < 1) {
					result->AddError("Unable to parse word-token specication");
					return result;
				}
				for each (auto spec in fragment->specifications) {
					for each (auto match in spec->matchAny) {
						for each (auto feature in match->features) {
							auto tuple = EncodeAny(feature->feature);
							if (tuple->Item5 != nullptr)
								result->AddError(tuple->Item5);
							else if (tuple->Item2 == 0 && tuple->Item3 == 0)
								result->AddError("Could not parse feature token: '" + feature->feature + "'");
							else if (tuple->Item4 != 0) {
								((Feature^)feature)->type = tuple->Item2;
								((Feature^)feature)->subtype = tuple->Item3;
								((Feature^)feature)->discretePOS = tuple->Item4;
								((Feature^)feature)->featureMatchVector = nullptr;
							}
							else if (tuple->Item1 != nullptr && tuple->Item1->Count > 0) {
								((Feature^)feature)->type = tuple->Item2;
								((Feature^)feature)->subtype = tuple->Item3;
								((Feature^)feature)->discretePOS = 0;
								((Feature^)feature)->featureMatchVector = tuple->Item1;
							}
							else if (tuple->Item2 == 0 && tuple->Item3 == 0)
								result->AddError("Could not parse feature token: '" + feature->feature + "'");
						}
					}
				}
			}
		}
		return result;
	}
	IQuelleSearchResult^ AVLCLR::ExecuteSearchRequest(QRequestSearch^ request, IQuelleSearchResult^ result, List<AVXSearchResult^>^ additions, List<AVXSearchResult^>^ subtractions) {
		for each (auto clause in request->clauses) {
#ifdef AVX_EXTRA_DEBUG_DIAGNOSTICS
			Console::Out->WriteLine(clause->segment + ": (execution)");
#endif
			BookChapterVerse bcvMatches;
			HashSet<UInt32>^ matches = this->SearchClause(clause, request->controls);
			for each (UInt32 match in matches)
			{
				auto parts = (BYTE*)&match;
				Byte b = *parts++;
				Byte c = *parts++;
				Byte v = *parts++;
				bcvMatches.AddVerse(b, c, v);
#ifdef AVX_EXTRA_DEBUG_DIAGNOSTICS
				auto book = getBookByNum(UINT16(b));
				Console::Out->Write(gcnew String((const char*)(&(book.name))) + " ");
				Console::Out->Write(UInt16(c).ToString() + ":");
				Console::Out->WriteLine(UInt16(v).ToString());
#endif
			}
			auto clauseResult = gcnew AVXSearchResult(bcvMatches.bcv, clause->polarity);
			if (clause->polarity == '+')
				additions->Add(clauseResult);
			else
				subtractions->Add(clauseResult);

#ifdef AVX_EXTRA_DEBUG_DIAGNOSTICS
			UINT16 nativeArray[17];
			for each (auto book in clauseResult->matches) {
				for each (auto chapter in book.Value) {
					auto info = getBookByNum(UINT16(book.Key));
					Console::Out->Write(gcnew String((const char*)(&(info.name))) + "[" + UInt16(book.Key).ToString() + "] ");
					Console::Out->Write(UInt16(chapter.Key).ToString());

					auto compacted = chapter.Value;
					auto cnt = 1 + XBitArray255::CountBits(compacted[0]);
					for (int i = 0; i < cnt; i++)
						nativeArray[i] = compacted[i];
					auto x = new XBitArray255(nativeArray);
					auto test = x->CreateByteArray();
					String^ delimiter = ":";
					for (auto i = 0; test[i] != 0; i++) {
						Console::Out->Write(delimiter + UInt16(test[i]).ToString());
						delimiter = ", ";
					}
					Console::Out->WriteLine();
					delete x;
					free(test);
				}
#endif
			}
		}
		return result;
	}
	IQuelleSearchResult^ AVLCLR::CollateSearchRequest(IQuelleSearchResult^ result, List<AVXSearchResult^>^ additions, List<AVXSearchResult^>^ subtractions) {
#ifdef AVX_EXTRA_DEBUG_DIAGNOSTICS
		Console::Out->WriteLine("collation:");
#endif		
		if (additions->Count > 0) {
			unsigned int found = 0;
			if (additions->Count == 1) {
				found = 1;
				additions[0]->messages = result->messages;
				result = additions[0];
			}
			else {
				for (int i = 0; i < additions->Count; i++) {
					if (additions[i]->matches->Count > 0) {
						if (++found == 1) {
							additions[i]->messages = result->messages;
							result = additions[i];
						}
						else {
							((AVXSearchResult^)result)->Add(additions[i]->matches);
						}
					}
				}
			}
			if (found && (additions[0]->matches->Count > 0))
			{
				for (int i = 0; i < subtractions->Count; i++)
					if (subtractions[i]->matches->Count > 0)
						((AVXSearchResult^)result)->Subtract(subtractions[i]->matches);
			}
		}
		return result;
	}
	// godhead + -- "eternal power"
	IQuelleSearchResult^ AVLCLR::Search(QRequestSearch^ request)
	{
		auto additions = gcnew List<AVXSearchResult^>();
		auto subtractions = gcnew List<AVXSearchResult^>();
		auto result = this->CompileSearchRequest(request);
		result = this->ExecuteSearchRequest(request, result, additions, subtractions);
		result = this->CollateSearchRequest(result, additions, subtractions);

		return result;
	}
	IQuellePageResult^ AVLCLR::Page(QRequestPage^ request)
	{
		return nullptr;
	}
	String^ AVLCLR::Test(String^ request)
	{
		return nullptr;
	}
	String^ AVLCLR::GetLex(UInt16 wkey, BYTE sequence)
	{
		char* lex = getLexicalEntry(wkey, sequence);
		String^ str = lex != NULL ? gcnew String(lex) : nullptr;

		return str;
	}
	String^ AVLCLR::GetOOV(UInt16 okey)
	{
		char* oov = getOovEntry(okey);
		String^ str = oov != NULL ? gcnew String(oov) : nullptr;

		return str;
	}
	UInt64 AVLCLR::Hash64(String^ token)
	{
		char buffer[32];
		buffer[31] = '\0';
		if (token->Length + 1 <= 32)
			buffer[token->Length] = '\0';
		for (int i = 0; i < token->Length; i++)
		{
			if (i >= 31)
				break;
			buffer[i] = (char)token[i];
		}
		return ::Hash64(buffer);
	}
	UInt64 AVLCLR::Encode64(String^ token)
	{
		if (token->Length > 8)
			return 0;

		char buffer[9];
		buffer[token->Length] = '\0';
		for (int i = 0; i < token->Length; i++)
		{
			buffer[i] = tolower((char)token[i]);
		}
		return ::Encode(buffer);
	}
	UInt64 AVLCLR::Encode64(String^ token, bool normalize)
	{
		if (token->Length > 8)
			return 0;

		char buffer[9];
		buffer[token->Length] = '\0';
		for (int i = 0; i < token->Length; i++)
		{
			buffer[i] = tolower((char)token[i]);
		}
		return ::Encode(buffer, normalize);
	}
	String^ AVLCLR::Decode64(UInt64 hash)
	{
		char buffer[32];
		int result = ::Decode(hash, buffer, 32);

		if (result <= 0)
			return "";

		return gcnew String(buffer, 0, strlen(buffer), System::Text::Encoding::ASCII);
	}
	AVVerse& AVLCLR::verseIdxToBcv(UINT16 idx) {
		return getVerse(idx);
	}
	HashSet<UInt32>^ AVLCLR::SearchClause(QClauseSearch^ clause, QSearchControls^ controls)
	{
		auto list = gcnew HashSet<UInt32>();

		if (clause->quoted)
			this->SearchClauseQuoted(list, clause, controls);
		else
			this->SearchClauseUnquoted(list, clause, controls);

		return list;
	}
	bool AVLCLR::IsMatch(const AVWrit const& writ, Feature^ feature)
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
											if ((value & writ.transition) == value)
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
			switch (feature->type & (FIND_Token|FIND_Lemma|FIND_GlobalTest|FIND_LANGUAGE_NUMERIC))
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
										if (value == (writ.wordKey & 0x3FFF))
										{
//											Console::Out->WriteLine("Found: " + feature->feature);
											return !not;
										}
									return not;
				case FIND_English:	for each (UInt16 value in feature->featureMatchVector)
										if (value == (writ.wordKey & 0x3FFF))
											return !not;
									return not;

				case FIND_Hebrew:	if (getVerse(writ.verseIdx).book <= 39) {
										int i = 0;
										auto strongs = (UINT16*)writ.srclang;
										for (auto strongs = (UINT16*)writ.srclang; *strongs != 0; strongs++)
											for each (UInt16 value in feature->featureMatchVector)
											{
												if (value == (UInt16)*strongs)
													return !not;
												if (++i == 4)
													return not;
											}
									}
									return not;
				case FIND_Greek:	if (getVerse(writ.verseIdx).book >= 40) {
										int i = 0;
										auto strongs = (UINT16*)writ.srclang;
										for (auto strongs = (UINT16*)writ.srclang; *strongs != 0; strongs++)
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

				/*case typeWordSame:*/	{
										UINT16 key = writ.wordKey & 0x3FFF;
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
	bool AVLCLR::IsMatch(const AVWrit const& writ, QSearchFragment^ frag)
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
	bool AVLCLR::SearchClauseQuoted(HashSet<UInt32>^ list, QClauseSearch^ clause, QSearchControls^ controls)
	{
		bool found = false;
		UInt16 span = controls->span;
		const AVWrit* writ = getWrit(0);	// ignore search.domain for now
		UInt32 cursor = 0;	// ignore search.domain for now

		do {
			AVVerse& verse = getVerse(writ[cursor].verseIdx);
			bool matched = true;
			UInt16 currentspan = (span == 0) ? getVerse(writ[cursor].verseIdx).wordCnt : span;
			for each (QSearchFragment ^ fragment in clause->fragments) {
				try {
					auto position = this->SearchSequentiallyInSpan(writ+cursor, currentspan, fragment);
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
				UINT32 begin = *((UINT32*)&verse);
				list->Add(begin);
				verse = getVerse(writ[cursor].verseIdx);
				UINT32 end = *((UINT32*)&verse);
				if (end != begin)
					list->Add(end);
			}
			if (span == 0 && matched) { // advance to the next verse
				while (writ[cursor].transition & 0x30 != 0x20)
					cursor ++;
			}
		}	while (cursor <= 0xC0C93);
		return found;
	}
	Int32 AVLCLR::SearchSequentiallyInSpan(const AVWrit* pwrit, UInt16 span, QSearchFragment^ frag)
	{
		for (Int32 i = 0; i < span; i++)
			if (this->IsMatch(pwrit[i], frag))
				return ++i;

		return 0;
	}
	bool AVLCLR::SearchClauseUnquoted(HashSet<UInt32>^ list, QClauseSearch^ clause, QSearchControls^ controls)
	{
		bool found = false;
		UInt16 span = controls->span;
		AVWrit* cursor = getWrit(0);	// ignore search.domain for now
		do {
			AVVerse& verse = getVerse(cursor->verseIdx);
			bool matched = true;
			UInt16 localspan = (span == 0) ? getVerse(cursor->verseIdx).wordCnt : span;
			for each (QSearchFragment ^ fragment in clause->fragments) {
				matched = (this->SearchUnorderedInSpan(cursor, localspan, fragment) >= 0);
				if (!matched)
					break;
				else if (!found)
					found = true;
			}
			if (matched) {
				UINT32 begin = *((UINT32*)&verse);
				list->Add(begin);
				verse = getVerse(cursor->verseIdx);
				UINT32 end = *((UINT32*)&verse);
				if (end != begin)
					list->Add(end);
			}
			cursor += localspan;
			if ((cursor->transition & 0x30) != 0x20) {
				auto transition = (UInt16)((cursor - 1)->transition);
			}
			if (span == 0 && matched) { // advance to the next verse
				while (cursor->transition & 0x30 != 0x20)
					cursor++;
			}
		}	while (((cursor-1)->transition & 0xF8) != 0xF8 && (cursor->transition & 0xF8) != 0xF8);
		return found;
	}
	Int32 AVLCLR::SearchUnorderedInSpan(const AVWrit* pwrit, UInt16 span, QSearchFragment^ frag)
	{
		for (Int32 i = 0; i < span; i++)
			if (this->IsMatch(pwrit[i], frag))
				return i;
		return (Int32) -1;
	}
	// see XBitArray::CreateByteArray() for reference, but this method has different implementation and is dotnet
	array<Byte>^ AVLCLR::ExpandVerseArray(array<UInt16>^ bits) {
		if (bits == nullptr || bits->Length < 1)
			return nullptr;

		BYTE cnt = 0;
		if (bits[0] != 0)
			for (auto s = 1; s < bits->Length; s++)
				cnt += XBitArray255::CountBits(bits[s]);

		auto result = gcnew array<Byte>(cnt);
		if (cnt == 0)
			return result;

		BYTE baseline = 0;
		BYTE current = 0;
		BYTE idx = 0;
		BYTE index = 0;
		UInt16 bitSegment = bits[0];
		do {
			if ((bitSegment & 0x1) != 0) {
				for (UINT16 bit = 0x1; bit != 0; bit <<= 1) {
					if ((bits[current] & bit) != 0) {
						BYTE value = 1 + baseline;
						result[idx++] = value;
					}
					baseline++;
				}
				current++;
			}
			else {
				baseline += 16;
			}
			bitSegment >>= 1;
			index++;
		}	while (bitSegment != 0);

		return result;
	}
	List<AVWritAbbreviated^>^ AVLCLR::GetChapter(Byte b, Byte c)
	{
		if (b >= 1 && b <= 66 && c >= 1)
		{
			AVBook& book = getBookByNum(b);
			if (c <= book.chapterCnt)
			{
				AVChapter& chapter = getChapter(book.chapterIdx + c - 1);
				auto list = gcnew List<AVWritAbbreviated^>();
				AVWrit* writ = getWrit(chapter.writIdx);
				for (int w = 0; w < chapter.wordCnt; w++)
					list->Add(gcnew AVWritAbbreviated(writ[w]));
				return list;
			}
		}
		return nullptr;
	}
}

