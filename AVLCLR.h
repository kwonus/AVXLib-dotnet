#pragma once
#pragma managed(push, off)
#include <avx.h>
#include <feature.h>
#include <CLexIndex.h>
#include <fivebitencoding.h>
#ifdef AVX_EXTRA_DEBUG_DIAGNOSTICS
#include <XBitArray.h>
#endif
#pragma managed(pop)

using namespace System;
using namespace System::Collections;
using namespace System::Collections::Generic;

using namespace System::Diagnostics;
using namespace QuelleHMI;
using namespace QuelleHMI::Tokens;

namespace AVXCLI {

	/// <summary>
	/// Summary for AVLCLR
	/// </summary>
	public ref class AVLCLR : public AbstractQuelleSearchProvider
	{
	private:
		static std::unordered_map<UINT64, AVLemma*>& lemma = *getLemmaMap();
		static std::unordered_map<UINT16, char*>& lemmaOOV = *getLemmaOovMap();
		static std::unordered_map<UINT16, AVLexicon*>& lexicon = *getLexiconMap();
		static std::unordered_map<UINT16, AVWordClass*>& wclass = *getWclassMap();
		static std::unordered_map<UINT16, AVName*>& names = *getNamesMap();

		static std::unordered_map<UINT16, char*>& forwardLemma = *getForwardLemmaMap();
		static std::unordered_map<UINT64, Bucket*>& reverseLemma = *getReverseLemmaMap();
		static std::unordered_map<UINT64, Bucket*>& reverseModern = *getReverseModernMap();
		static std::unordered_map<UINT64, UINT16>& reverseSearch = *getReverseSearchMap();
		static std::unordered_map<UINT64, UINT16>& reverseName = *getReverseNameMap();

		static std::unordered_map<UINT64, BYTE>& boundaryMap = *getSlashBoundaryMap();   // examples: /BoV/ /BoC/ /EoB/
		static std::unordered_map<UINT64, BYTE>& punctuationMap = *getSlashPuncMap();      // examples: /;/ /./ /?/ /'/
		static std::unordered_map<UINT64, BYTE>& suffixMap = *getPoundWordSuffixMap(); // examples: #kjv[1] #av[1] #exact[1] #avx[2] #modern[2] #any[3] #fuzzy[3]
		static std::unordered_map<UINT64, BYTE>& globalMap = *getPoundWordlessMap();   // examples: #diff

	public:
		static AVLCLR^ SELF = nullptr;
		AVLCLR()
		{
			//
			//TODO: Add the constructor code here
			//
			char* path = "C:\\src\\Digital-AV\\z-series\\";
			initialize(path);

			SELF = this;

#ifdef AVX_EXTRA_DEBUG_DIAGNOSTICS
			//	Temporary test:
			XBitArray255 test;
			test.SetBit(1);
			test.SetBit(4);
			test.SetBit(33);
			test.SetBit(32);
			test.SetBit(16);
			test.SetBit(15);
			test.SetBit(255);
			auto list = test.CreateByteArray();
			String^ delimiter = "";
			for (auto i = 0; list[i] != 0; i++) {
				Console::Out->Write(delimiter + UInt16(list[i]).ToString());
				delimiter = ", ";
			}
			Console::Out->WriteLine();
			delete list;
#endif
		}
		~AVLCLR()
		{
			release();
		}
		char* StrToChr(String^ str, char* chr, int len);
		Tuple<List<UInt16>^, Byte, Byte, UInt32, String^>^ EncodeAny(String^ token);
		Tuple<List<UInt16>^, Byte, Byte, String^>^ EncodeGlobalTest(UINT64 hash);
		Tuple<List<UInt16>^, Byte, Byte, String^>^ EncodeBoundary(UINT64 hash);
		Tuple<List<UInt16>^, Byte, Byte, String^>^ EncodePunctuation(UINT64 hash);
		Tuple<List<UInt16>^, Byte, Byte, String^>^ EncodeLanguageNumeric(String^ token);
		Tuple<Byte, Byte, UInt32>^ EncodeDiscretePOS(String^ token);
		Tuple<List<UInt16>^, Byte, Byte, String^>^ EncodeBitwisePOS(String^ token);
		Tuple<List<UInt16>^, Byte, Byte, String^>^ EncodeLemma(String^ token);
		Tuple<List<UInt16>^, Byte, Byte, String^>^ EncodeWord(String^ token);
		void EncodeWordModernWildcard(List<UInt16>^ list, String^ start, String^ end);
		void EncodeWordSearchWildcard(List<UInt16>^ list, String^ start, String^ end);
		void EncodeWordSearch(List<UInt16>^ list, UINT64 word);
		void EncodeWordModern(List<UInt16>^ list, UINT64 word);
		IQuelleSearchResult^ Search(QRequestSearch^ request) override;
		IQuellePageResult^ Page(QRequestPage^ request) override;
		String^ Test(String^ request) override;
		String^ GetLex(UInt16 wkey, BYTE sequence);
		String^ GetOOV(UInt16 okey);
		UInt64 Hash64(String^ token);
		UInt64 Encode64(String^ token);
		UInt64 Encode64(String^ token, bool normalize);
		String^ Decode64(UInt64 hash);
		HashSet<UInt32>^ SearchClause(QClauseSearch^ clause, QSearchControls^ controls);
		AVVerse& verseIdxToBcv(UINT16);
		array<Byte>^ ExpandVerseArray(array<UInt16>^ bits);

		static bool StartsWith(const char* chr, String^ str)
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
		static bool EndsWith(const char* chr, String^ str)
		{
			if (str == nullptr || chr == NULL)
				return false;

			if (str->IndexOf('-') >= 0)	// ignore hyphens
				return EndsWith(chr, str->Replace("-", ""));

			int len = strlen(chr);
			if (len < str->Length)
				return false;
			auto tst = chr + len - 1;

			for (int i = str->Length - 1; i >= 0; i--)
			{
				if (*tst == '-') // ignore hyphens
					return (gcnew String(chr))->Replace("-", "")->EndsWith(str);
				if (*tst-- != str[i])
					return false;
			}
			return true;
		}
		private:
			bool SearchClauseQuoted(HashSet<UInt32>^ list, QClauseSearch^ clause, QSearchControls^ controls);
			bool SearchClauseUnquoted(HashSet<UInt32>^ list, QClauseSearch^ clause, QSearchControls^ controls);
			Int32 SearchUnorderedInSpan(const AVWrit* pwrit, UInt16 span, QSearchFragment^ frag);
			Int32 SearchSequentiallyInSpan(AVWrit* &pwrit, UInt16& span, QSearchFragment^ frag);
			bool IsMatch(const AVWrit const& writ, QSearchFragment^ frag);
			bool IsMatch(const AVWrit const& writ, Feature^ feature);
	};
}
