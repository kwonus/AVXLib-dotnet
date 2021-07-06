#pragma once
#pragma managed(push, off)
#define AVX_MASKED_ACCESS
#include <avx.h>
#pragma managed(pop)

#include <AVLCLR.h>

using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Collections::Generic;

using namespace System::Diagnostics;
using namespace QuelleHMI;
using namespace QuelleHMI::Tokens;

using namespace AVXCLI;

namespace AVXCLI {
    ref class AVXSearchResult : public AbstractQuelleSearchResult
    {
    private:
        Dictionary<Byte, Dictionary<Byte, UInt32>^>^ results;
    public:
        AVXSearchResult(Dictionary<Byte, Dictionary<Byte, UInt32>^>^ results, Char polarity)
            : positive(polarity == '+') {
            this->results = results;
        }
        const bool positive;
        // We used to add/subtract whole bible at a time; new interface (to constrain RAM usage is a chapter at a time
        Boolean Subtract(Dictionary<Byte, Dictionary<Byte, UInt32>^>^ bibleMatches, Byte b, Byte c, Dictionary<Byte, UInt64> versesMatches) {
            if (b < 1 || b > 66 || c < 1)
                return false;
            auto bk = AVXCLI::AVLCLR::GetBookByNum(b);
            if (bk->chapterCnt > c)
                return false;

            auto book = bibleMatches->ContainsKey(b) ? bibleMatches[b] : nullptr;
            if (book == nullptr) {
                book = gcnew Dictionary<Byte, UInt32>();
                bibleMatches[b] = book;
            }
            UInt16 wordIdx = 0;
            if (!book->ContainsKey(c)) {
                auto chap = bk->chapterIdx;
                AVSDK::Chapter^ chapter = AVLCLR::XChapter->chapters[chap++];
                auto wordIdx = chapter->verseIdx;
                for (Byte ch = 2; ch <= c; ch++) {
                    wordIdx += chapter->wordCnt;
                    chapter = AVXCLI::AVLCLR::XChapter->chapters[chap++];
                }
                book[c] = wordIdx;
            }
            else wordIdx = book[c];

            return true;
        }
        // We used to add/subtract whole bible at a time; new interface is per chapter (to constrain RAM usage is a chapter at a time
        Boolean Add(Dictionary<Byte, Dictionary<Byte, UInt32>^>^ bibleMatches, Byte b, Byte c, Dictionary<Byte, UInt64> versesMatches) {
            if (b < 1 || b > 66 || c < 1)
                return false;
            auto bk = AVLCLR::GetBookByNum(b);
            if (bk->chapterCnt > c)
                return false;

            auto book = bibleMatches->ContainsKey(b) ? bibleMatches[b] : nullptr;
            if (book == nullptr) {
                book = gcnew Dictionary<Byte, UInt32>();
                bibleMatches[b] = book;
            }
            UInt16 wordIdx = 0;
            if (!book->ContainsKey(c)) {
                auto chap = bk->chapterIdx;
                auto chapter = AVLCLR::XChapter->chapters[chap++];
                auto wordIdx = chapter->writIdx;
                for (Byte ch = 2; ch <= c; ch++) {
                    wordIdx += chapter->wordCnt;
                    chapter = AVLCLR::XChapter->chapters[chap++];
                }
                book[c] = wordIdx;
            }
            else wordIdx = book[c];

            return true;
        }
        /*
        //                  b                c     v [compact bit array]
        property Dictionary<Byte, Dictionary<Byte, UInt32>^>^ matches {
            Dictionary<Byte, Dictionary<Byte, UInt32>^>^ get() override {
                return this->results;
            }
        }
        */
        /*
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
        */
    };
}

