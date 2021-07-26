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
	public ref class BookChapterVerse
	{
	public:
		HashSet<UInt64>^ Matches;
		Dictionary<UInt32, UInt64>^ Tokens;

		BookChapterVerse();
		bool AddMatch(UInt16 segmentIdx, UInt32 wstart, UInt32 wlast);
		bool AddMatch(UInt16 segmentIdx, UInt32 wstart, UInt16 wcnt);
		bool SubtractMatch(UInt32 wstart, UInt32 wlast);
		bool SubstractMatch(UInt32 wstart, UInt16 wcnt);
		void SearchClause(QClauseSearch^ clause, QSearchControls^ controls);

	private:
		UInt32 SearchClauseQuoted_ScopedUsingSpan(QClauseSearch^ clause, QSearchControls^ controls);
		UInt32 SearchClauseQuoted_ScopedUsingVerse(QClauseSearch^ clause, QSearchControls^ controls);

		UInt32 SearchClauseUnquoted_ScopedUsingSpan(QClauseSearch^ clause, QSearchControls^ controls);
		UInt32 SearchClauseUnquoted_ScopedUsingVerse(QClauseSearch^ clause, QSearchControls^ controls);

		UInt32 SearchUnorderedInSpan(UInt16 span, QSearchFragment^ frag);
		UInt32 SearchSequentiallyInSpan(UInt16 span, QSearchFragment^ frag);
		bool IsMatch(const AVSDK::Writ176 const% writ, QSearchFragment^ frag);
		bool IsMatch(const AVSDK::Writ176 const% writ, Feature^ feature);
	};
}

