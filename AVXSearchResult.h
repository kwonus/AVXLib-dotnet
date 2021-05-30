#pragma once
#pragma managed(push, off)
#include <XBitArray.h>
#pragma managed(pop)

using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Collections::Generic;

using namespace System::Diagnostics;
using namespace QuelleHMI;
using namespace QuelleHMI::Tokens;

ref class AVXSearchResult : public AbstractQuelleSearchResult
{
private:
    Dictionary<Byte, Dictionary<Byte, array<UInt16>^>^>^ results;
public:
    AVXSearchResult(Dictionary<Byte, Dictionary<Byte, array<UInt16>^>^>^ results, Char polarity)
    : positive(polarity == '+') {
        this->results = results;
    }
    const bool positive;
    void Add(Dictionary<Byte, Dictionary<Byte, array<UInt16>^>^>^ incoming) {
        UINT16 nativeVerseBits[17];
        for each (auto book in incoming) {
            if (!this->results->ContainsKey(book.Key)) {
                this->results[book.Key] = book.Value;
            }
            else
            {
                auto collatedBook = this->results[book.Key];
                for each (auto chapter in book.Value) {
                    if (!collatedBook->ContainsKey(chapter.Key)) {
                        collatedBook[chapter.Key] = chapter.Value;
                    }
                    else
                    {
                        auto collatedChapter = collatedBook[chapter.Key];
                        for (int i = 0; i < collatedChapter->Length; i++)
                            nativeVerseBits[i] = collatedChapter[i];
                        auto collatedVerses = new XBitArray255(nativeVerseBits);
                        for (int i = 0; i < chapter.Value->Length; i++)
                            nativeVerseBits[i] = chapter.Value[i];
                        auto incomingVerses = new XBitArray255(nativeVerseBits);    
                        collatedVerses->Add(*incomingVerses);
                        auto cnt = collatedVerses->GetCompactBitArray(nativeVerseBits, 17);
                        if (cnt > collatedChapter->Length) {
                            collatedBook->Remove(chapter.Key);
                            collatedBook[chapter.Key] = gcnew array<UInt16>(cnt);
                        }
                        for (int i = 0; i < cnt; i++) {
                            collatedChapter[i] = nativeVerseBits[i];
                        }
                    }
                }
            }
        }
        this->results = results;
    }
    void Subtract(Dictionary<Byte, Dictionary<Byte, array<UInt16>^>^>^ incoming) {
        UINT16 nativeVerseBits[17];
        for each (auto book in incoming) {
            if (this->results->ContainsKey(book.Key)) {
                auto collatedBook = this->results[book.Key];
                for each (auto chapter in book.Value) {
                    if (collatedBook->ContainsKey(chapter.Key)) {
                        auto collatedChapter = collatedBook[chapter.Key];
                        for (int i = 0; i < collatedChapter->Length; i++)
                            nativeVerseBits[i] = collatedChapter[i];
                        auto collatedVerses = new XBitArray255(nativeVerseBits);
                        for (int i = 0; i < chapter.Value->Length; i++)
                            nativeVerseBits[i] = chapter.Value[i];
                        auto incomingVerses = new XBitArray255(nativeVerseBits);
                        collatedVerses->Subtract(*incomingVerses);
                        collatedVerses->GetCompactBitArray(nativeVerseBits, 17);
                        for (int i = 0; i < collatedChapter->Length; i++)
                            collatedChapter[i] = nativeVerseBits[i];
                    }
                }
            }
        }
        // Prune the map where appropriate
        //
        auto bookMap = this->results;
        Byte books[66];
        Byte bookCnt = 0;
        Byte chapters[255];
        Byte chapterCnt = 0;

        for each (auto bookKey in bookMap->Keys)
            books[bookCnt++] = bookKey;

        for (int b = 0; b < bookCnt; b++) {
            auto bookKey = books[b];
            if (this->results->ContainsKey(bookKey)) {
                chapterCnt = 0;
                auto chapterMap = bookMap[bookKey];
                for each (auto chapterKey in chapterMap->Keys)
                    chapters[chapterCnt++] = chapterKey;

                for (int c = 0; c < chapterCnt; c++) {
                    auto chapterKey = chapters[c];
                    auto verses = chapterMap[chapterKey];
                    if (verses == nullptr || verses[0] == 0) {
                        chapterMap->Remove(chapterKey);
                    }
                }
                if (chapterMap->Count < 1) {
                    bookMap->Remove(bookKey);
                }
            }
        }
    }
    //                  b                c     v [compact bit array]        
    property Dictionary<Byte, Dictionary<Byte, array<UInt16>^>^>^ matches {
        Dictionary<Byte, Dictionary<Byte, array<UInt16>^>^>^ get() override {
            return this->results;
        }
    }
    property String^ summary {
        String^ get() override {
            UInt32 vcnt = 0;
            UInt32 ccnt = 0;
            UInt32 bcnt = this->results->Count;
            for each (auto b in this->results->Values) {
                ccnt += b->Count;
                for each (auto cv in b->Values)
                    vcnt += XBitArray255::CountBits(cv[0]);
            }
            return vcnt.ToString() + " verses found in " + ccnt.ToString() + " chapters in " + bcnt.ToString() + " books";
        }
    }
    property UInt64 count {
        UInt64 get() override {
            UInt64 vcnt = 0;
            for each (auto b in this->results->Values) {
                for each (auto cv in b->Values)
                    vcnt += XBitArray255::CountBits(cv[0]);
            }
            return vcnt;
        }
    }
};

