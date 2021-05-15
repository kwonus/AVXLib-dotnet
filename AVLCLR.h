#pragma once
#pragma managed(push, off)
#include <avx.h>
#include <CLexIndex.h>
#include <fivebitencoding.h>
using namespace std;
#pragma managed(pop)

using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Collections::Generic;
using namespace System::Diagnostics;
using namespace QuelleHMI;
using namespace QuelleHMI::Tokens;

namespace AVXCLI {

	/// <summary>
	/// Summary for AVLCLR
	/// </summary>
	public ref class AVLCLR :  public AbstractQuelleSearchProvider
	{
	private:
		static std::unordered_map<UINT64, AVLemma*>			&lemma		= *getLemmaMap();
		static std::unordered_map<UINT16, char*>			&lemmaOOV	= *getLemmaOovMap();
		static std::unordered_map<UINT16, AVLexicon*>		&lexicon	= *getLexiconMap();
		static std::unordered_map<UINT16, AVWordClass*>		&wclass		= *getWclassMap();
		static std::unordered_map<UINT16, AVName*>			&names		= *getNamesMap();

		static std::unordered_map<UINT16, char*>			&forwardLemma  = *getForwardLemmaMap();
		static std::unordered_map<UINT64, Bucket*>			&reverseLemma  = *getReverseLemmaMap();
		static std::unordered_map<UINT64, Bucket*>			&reverseModern = *getReverseModernMap();
		static std::unordered_map<UINT64, UINT16>			&reverseSearch = *getReverseSearchMap();
		static std::unordered_map<UINT64, UINT16>			&reverseName   = *getReverseNameMap();

		static std::unordered_map<UINT64, BYTE>				&boundaryMap   = *getSlashBoundaryMap();   // examples: /BoV/ /BoC/ /EoB/
		static std::unordered_map<UINT64, BYTE>				&punctuationMap = *getSlashPuncMap();      // examples: /;/ /./ /?/ /'/
		static std::unordered_map<UINT64, BYTE>				&suffixMap	   = *getPoundWordSuffixMap(); // examples: #kjv[1] #av[1] #exact[1] #avx[2] #modern[2] #any[3] #fuzzy[3]
		static std::unordered_map<UINT64, BYTE>				&globalMap     = *getPoundWordlessMap();   // examples: #diff

	public:
		static AVLCLR^ SELF = nullptr;
		AVLCLR(void)
		{
			//
			//TODO: Add the constructor code here
			//
			char* path = "C:\\src\\Digital-AV\\z-series\\";
			initialize(path);

			SELF = this;
		}
		char* StrToChr(String^ str, char* chr, int len)
		{
			if (len >= str->Length + 1) {
				for (int i = 0; i < str->Length; i++)
					chr[i] = str[i];
				chr[str->Length] = char(0);
				return chr;
			}
		}
		Tuple<List<UInt16>^, Byte, Byte, UInt32, String^>^ EncodeAny(String^ token)
		{
			Tuple<List<UInt16>^, Byte, Byte, String^>^ result;
			Tuple<Byte, Byte, UInt32>^ result32;
			UInt32 discretePOS = 0;
			if (token->StartsWith("/") && token->EndsWith("/")) {
				if (token->Length > 2) {
					String^ test = token->Substring(0, token->Length - 2);
					UINT64 hash = HashTrivialFromString(test);

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
				UINT64 hash = HashTrivialFromString(token);
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
		Tuple<List<UInt16>^, Byte, Byte, String^>^ EncodeGlobalTest(UINT64 hash)
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
		Tuple<List<UInt16>^, Byte, Byte, String^>^ EncodeBoundary(UINT64 hash)
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
		Tuple<List<UInt16>^, Byte, Byte, String^>^ EncodePunctuation(UINT64 hash)
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
		Tuple<List<UInt16>^, Byte, Byte, String^>^ EncodeLanguageNumeric(String^ token)
		{
			Tuple<List<UInt16>^, Byte, Byte, String^>^ result;
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
		Tuple<Byte, Byte, UInt32>^ EncodeDiscretePOS(String^ token)
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
		Tuple<List<UInt16>^, Byte, Byte, String^>^ EncodeBitwisePOS(String^ token)
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
		Tuple<List<UInt16>^, Byte, Byte, String^>^ EncodeLemma(String^ token)
		{
			if (!token->StartsWith("#")) {
				return nullptr;
			}
			String^ error = nullptr;
			Byte tokenType = FIND_Lemma;
			Byte otherType = 0;
			auto list = gcnew List<UInt16>();

			UINT64 test = Hash64FromString(token);
			if (reverseLemma.count(test) != 0) {
				auto head = reverseLemma.at(test);
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

		Tuple<List<UInt16>^, Byte, Byte, String^>^ EncodeWord(String^ token)
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
				char result[24];
				char* chr = StrToChr(token->Substring(pound)->ToLower(), result, 24);
				auto chk = HashTrivial(chr);
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
					UINT64 hash = Hash64FromString(token);
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
		static bool StartsWith(char* chr, String^ str)
		{
			if (str == nullptr || chr == NULL)
				return false;

			if (str->IndexOf('-') >= 0)	// ignore hyphens
				return StartsWith(chr, str->Replace("-", ""));

			for (int i = 0; i < str->Length; i++)
			{
				if (*chr == 0)
					return false;
				else if (*chr == '-') // ignore hyphens
					return (gcnew String(chr))->Replace("-", "")->StartsWith(str);
				else if (*chr++ != str[i])
					return false;
			}
			return true;
		}
		static bool EndsWith(char* chr, String^ str)
		{
			if (str == nullptr || chr == NULL)
				return false;

			if (str->IndexOf('-') >= 0)	// ignore hyphens
				return EndsWith(chr, str->Replace("-", ""));

			int len = strlen(chr);
			if (len < str->Length)
				return false;
			auto tst = chr + len - 1;

			for (int i = str->Length-1; i >= 0; i--)
			{
				if (*tst == '-') // ignore hyphens
					return (gcnew String(chr))->Replace("-", "")->EndsWith(str);
				if (*tst-- != str[i])
					return false;
			}
			return true;
		}
		void EncodeWordModernWildcard(List<UInt16>^ list, String^ start, String^ end)
		{
			for (auto it = reverseModern.begin(); it != reverseModern.end(); ++it) {
				auto hashed = it->first;
				char* token = getHashedString(hashed);
				
				if ((start == nullptr || StartsWith(token, start))
				&&  (end == nullptr   || EndsWith(token, end)) ) {
					auto bucket = it->second;
					list->Add(bucket->value);
					for (BucketOverflow* next = bucket->overflow; next != NULL; next = next->next)
						list->Add(next->value);
				}
			}
		}
		void EncodeWordSearchWildcard(List<UInt16>^ list, String^ start, String^ end)
		{
			for (auto it = reverseSearch.begin(); it != reverseSearch.end(); ++it) {
				auto hashed = it->first;
				char* token = getHashedString(hashed);

				if ((start == nullptr || StartsWith(token, start))
					&& (end == nullptr || EndsWith(token, end))) {
					list->Add(it->second);
				}
			}
		}
		void EncodeWordSearch(List<UInt16>^ list, UINT64 word)
		{
			if (reverseSearch.count(word) != 0)
			{
				auto list = gcnew List<UInt16>();
				if (reverseSearch.count(word) != 0) {
					list->Add(reverseSearch.at(word));
				}
			}
		}
		void EncodeWordModern(List<UInt16>^ list, UINT64 word)
		{
			if (reverseModern.count(word) != 0) {
				auto list = gcnew List<UInt16>();
				if (reverseModern.count(word) != 0) {
					auto head = reverseModern.at(word);
					list->Add(head->value);
					for (BucketOverflow* next = head->overflow; next != NULL; next = next->next)
						list->Add(next->value);
				}
			}
		}
		AbstractQuelleSearchResult^ Search(QRequestSearch^ request) override
		{
			auto result = gcnew AbstractQuelleSearchResult();

			for each (auto clause in request->clauses) {
				for each (auto fragment in clause->fragments) {
					for each (auto spec in fragment->spec) {
						for each (auto match in spec->matchAny) {
							for each (auto feature in match->features) {
								auto tuple = EncodeAny(feature->feature);
								if (tuple->Item5 != nullptr)
									result->messages->Add("error", tuple->Item5);
								else if (tuple->Item2 == 0 && tuple->Item3 == 0)
									result->messages->Add("error", "Could not parse feature token: '" + feature->feature + "'");
								else if (tuple->Item4 != 0) {
									((Feature^)feature)->type = tuple->Item2;
									((Feature^)feature)->subtype = tuple->Item3;
									((Feature^)feature)->discretePOS = tuple->Item4;
									((Feature^)feature)->featureMatchVector = nullptr;
								}
								else if (tuple->Item1 != nullptr && tuple->Item1->Count > 0) {
									((Feature^)feature)->type = tuple->Item2;
									((Feature^)feature)->subtype = tuple->Item3;
									((Feature^)feature)->discretePOS = tuple->Item4;
									((Feature^)feature)->featureMatchVector = tuple->Item1;
								}
								else if (tuple->Item2 == 0 && tuple->Item3 == 0)
									result->messages->Add("error", "Could not parse feature token: '" + feature->feature + "'");
							}
						}
					}
					// Now we can execute the search (micro-parsing is complete)
					//
					if (result->messages->Count < 1) {

					}
				}
			}
			return result;
		}
		AbstractQuellePageResult^ Page(QRequestPage^ request) override
		{
			return nullptr;
		}
		String^ Test(String^ request) override
		{
			return nullptr;
		}
		String^ GetLex(UInt16 wkey, BYTE sequence)
		{
			char* lex = getLexicalEntry(wkey, sequence);
			String^ str = lex != NULL ? gcnew String(lex) : nullptr;

			return str;
		}
		String^ GetOOV(UInt16 okey)
		{
			char* oov = getOovEntry(okey);
			String^ str = oov != NULL ? gcnew String(oov) : nullptr;

			return str;
		}
		UInt64 Hash64FromString(String^ token)
		{
			char buffer[32];
			buffer[31] = '\0';
			if (token->Length <= 32)
				buffer[token->Length-1] = '\0';
			for (int i = 0; i < token->Length; i++)
			{
				if (i >= 31)
					break;
				buffer[i] = (char) token[i];
			}
			return Hash64(buffer);
		}
		UInt64 HashTrivialFromString(String^ token)
		{
			if (token->Length > 8)
				return 0;

			char buffer[9];
			buffer[token->Length] = '\0';
			for (int i = 0; i < token->Length; i++)
			{
				buffer[i] = tolower((char)token[i]);
			}
			return HashTrivial(buffer);
		}

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~AVLCLR()
		{
			release();
		}
	};
}
