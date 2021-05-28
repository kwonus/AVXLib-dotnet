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
    AVXSearchResult(Dictionary<Byte, Dictionary<Byte, array<UInt16>^>^>^ results) {
        this->results = results;
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

