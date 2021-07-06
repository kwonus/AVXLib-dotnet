#pragma once
#pragma managed(push, off)
#define AVX_MASKED_ACCESS
#include <avx.h>
#pragma managed(pop)

using namespace System;
using namespace System::Collections::Generic;

using namespace QuelleHMI;
using namespace QuelleHMI::Tokens;
using namespace QuelleHMI::Interop;

namespace AVXCLI {
	public ref class BookChapterVerse : public Dictionary<Byte, Dictionary<Byte, UInt32>^> // managed version of BookChapterVerseMap
	{
	public:
		array<UInt64>^ Matched;

		BookChapterVerse();
		bool AddChapter(Byte b, Byte c);
		bool SubtractChapter(Byte b, Byte c, Byte v);
		UInt32 GetVerseCount(Byte b, Byte c);
		Dictionary<Byte, UInt32>^ GetVerses(Byte b, Byte c);  // hash is verse-num to writIdx for verse
		void SearchClause(Byte b, Byte c, QClauseSearch^ clause, QSearchControls^ controls);

	private:
		bool SearchClauseQuoted(Byte b, Byte c, QClauseSearch^ clause, QSearchControls^ controls);
		bool SearchClauseUnquoted(Byte b, Byte c, QClauseSearch^ clause, QSearchControls^ controls);
		Int32 SearchUnorderedInSpan(UInt64 bits, UInt32 writIdx, UInt16 span, QSearchFragment^ frag);
		Int32 SearchSequentiallyInSpan(UInt64 bits, UInt32 writIdx, UInt16 span, QSearchFragment^ frag);
		bool IsMatch(const AVSDK::Writ176 const% writ, QSearchFragment^ frag);
		bool IsMatch(const AVSDK::Writ176 const% writ, Feature^ feature);
	};
}

