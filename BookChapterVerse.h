#pragma once
#pragma managed(push, off)
#include <avx.h>
#include <XBitArray.h> 
//using namespace std;
#pragma managed(pop)

using namespace System;
using namespace System::Collections::Generic;

ref class BookChapterVerse // managed version of BookChapterVerseMap
{
public:
	Dictionary<Byte, Dictionary<Byte, array<UInt16>^>^>^ bcv;
	BookChapterVerse();
	bool AddVerse(BYTE b, BYTE c, BYTE v);
};

