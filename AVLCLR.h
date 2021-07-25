#pragma once
#pragma managed(push, off)
#define AVX_MASKED_ACCESS
#include <avx.h>
#include <fivebitencoding.h>
#pragma managed(pop)

using namespace System;
using namespace System::Collections;
using namespace System::Collections::Generic;

using namespace System::Diagnostics;
using namespace QuelleHMI;
using namespace QuelleHMI::Tokens;
using namespace QuelleHMI::Interop;

using namespace AVSDK;

ref class AVXSearchResult;

namespace AVXCLI {

	/// <summary>
	/// Summary for AVLCLR
	/// </summary>
	public ref class AVLCLR : public AbstractQuelleSearchProvider
	{
	public:
		static AVLCLR^ SELF = nullptr;
		static MMWritDX11^ XWrit;
		static IXBook^ XBook;
		static IXChapter^ XChapter;
		static IXVerse^ XVerse;

		static array<AVSDK::Chapter^>^ Chapters;
		static array<AVSDK::Book>^ Books;
		static array<UInt32>^ Verses;

		AVLCLR()
		{
			//
			//TODO: Add the constructor code here (and get rif of hard-coded path)
			//
			char* path = "C:\\src\\Digital-AV\\z-series\\";
			auto spath = gcnew String(path);
			XWrit = gcnew AVSDK::MMWritDX11(spath);
			XBook = gcnew AVSDK::IXBook(spath);
			XVerse = gcnew AVSDK::IXVerse(spath);
			XChapter = gcnew AVSDK::IXChapter(spath);

			initialize(path);

			Books = XBook->books;
			Chapters = XChapter->chapters;
			Verses = XVerse->verses;

			SELF = this;
		}
		~AVLCLR()
		{
			release();
		}
		static AVSDK::Book^ GetBookByNum(Byte num) {
			if (num >= 1 && num <= 66) {
				return AVLCLR::XBook->books[num - 1];
			}
			return nullptr;
		}
		static String^ GetBookNameByNum(Byte num) {
			if (num >= 1 && num <= 66) {
				return AVLCLR::XBook->books[num - 1].name;
			}
			return nullptr;
		}
		static Byte GetChapterCount(Byte num) {
			if (num >= 1 && num <= 66) {
				return AVLCLR::XBook->books[num - 1].chapterCnt;
			}
			return 0;
		}
		static UInt16 GetChapterIndex(Byte num) {
			if (num >= 1 && num <= 66) {
				return AVLCLR::XBook->books[num - 1].chapterIdx;
			}
			return 0;
		}
		static String^ GetLexicalEntry(UInt16 key, Byte sequence)
		{
			const char* lex = getLexicalEntry(key & 0x3FFF, sequence);
			return gcnew String(lex);
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

			auto len = strlen(chr);
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

		AbstractQuelleSearchResult^ CompileSearchRequest(QRequestSearch^ request);
		void ExecuteSearchRequest(QClauseSearch^ clause, QSearchControls^ controls, AbstractQuelleSearchResult^ result);

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
	};
}
