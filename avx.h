#ifndef AVX_HEADER
#define AVX_HEADER
#include <XVMem.h>
#include <unordered_map>
#pragma pack(push)  /* push current alignment to stack */
#pragma pack(1)     /* set alignment to 1 byte boundary */

extern "C"
{
using AVWrit = struct AVWritDX
{
    UINT64 srclang;
    UINT16 verseIdx;
    UINT16 wordKey;
    BYTE punc;
    BYTE transition;
    UINT16 pnwc;
    UINT32 pos;
    UINT16 lemma;
};

const BYTE NAME_LEN = 16;
const BYTE ABBR_LEN = 12;
using AVBook = struct AVBookIX
{
    BYTE book;
    BYTE chapterCnt;
    UINT16 chapterIdx;
//		char name[16];
//		char abbreviations[12];
    UINT64 name;
    UINT64 name_part2;
    UINT64 abbreviations;
    UINT32 abbreviations_part2;
};

using AVChapter = struct AVChapterIX
{
    UINT32 writIdx;
    UINT16 verseIdx;
    UINT16 wordCnt;
};

using AVVerse = struct AVVerseIX
{
    BYTE book;
    BYTE chapter;
    BYTE verse;
    BYTE wordCnt;
};

using AVLemma = struct AVLemmaDXI
{
    UINT32 pos;
    UINT16 wordKey;
    UINT16 wordClass;
    UINT16 lemmaCnt;
    UINT16* lemma;
};

using AVLemmaOOV = struct AVLemmaOovDXI
{
    UINT16 oovKey;
    char lemma;
};

using AVLexicon = struct AVLexiconDXI
{
    UINT16 entities;
    UINT16 posCnt;
    UINT32* pos;
//		char* search;
//		char* display;
//		char* modern;
};

using AVWordClass = struct AVWordClassDXI
{
    UINT16 wordClass;
    UINT16 width;
    UINT32* pos;
};

using AVName = struct AVNameDXI
{
    UINT16 wordKey;
    BYTE   meanings;
};

class BucketOverflow
{
public:
    const UINT16 value;

    const BucketOverflow* const GetNext()
    {
        return this->next;
    }
    BucketOverflow(UINT16 value) : value(value), next(NULL)
    {
        ;
    }
    ~BucketOverflow()
    {
        if (this->next)
            delete this->next;
    }
    BucketOverflow* next;
};
class Bucket
{
private:
    UINT32 count;
public:
    const UINT16 value;
    BucketOverflow* overflow;
    BucketOverflow* terminal;

    Bucket(UINT16 value): value(value), count(1), overflow(NULL), terminal(NULL)
    {
        ;
    }
    ~Bucket()
    {
        if (this->overflow)
            delete this->overflow;
    }
    UINT32 AddOverflow(UINT16 value)
    {
        if (this->terminal != NULL)
        {
            this->terminal->next = new BucketOverflow(value);
            this->terminal = this->terminal->next;
        }
        else
        {
            this->terminal = new BucketOverflow(value);
        }
        this->count++;
        return this->count;
    }
    UINT32 GetCount()
    {
        return this->count;
    }
    const BucketOverflow* const GetOverflow()
    {
        return this->overflow;
    }
};

void initialize(char* folder);
void release();
UINT32 getWritCnt();
UINT16 getBookCnt();
UINT32 getChapterCnt();
UINT32 getVerseCnt();
AVWrit* getWrit(UINT32 idx);
AVBook& getBook(UINT16 idx);
AVBook& getBookByNum(UINT16 num);
AVChapter& getChapter(UINT16 idx);
AVVerse& getVerse(UINT16 idx);
const BYTE SEARCH = 1;	// sequence
const BYTE DISPLAY = 2;	// sequence
const BYTE MODERN = 3;	// sequence
const BYTE MODERN_WITHOUT_HYPHENS = 4;	// sequence
char* getLexicalEntry(UINT16 key, BYTE sequence);
char* getOovEntry(UINT16 key);
UINT16 getLemma(UINT32 pos, UINT16 wkey, char* data[], UINT16 arrayLen);

std::unordered_map<UINT16, char*>* getLemmaOovMap();
std::unordered_map<UINT64, AVLemma*>* getLemmaMap();
std::unordered_map<UINT16, AVLexicon*>* getLexiconMap();
std::unordered_map<UINT16, AVWordClass*>* getWclassMap();
std::unordered_map<UINT16, AVName*>* getNamesMap();

std::unordered_map<UINT16, char*>* getForwardLemmaMap();
std::unordered_map<UINT64, Bucket*>* getReverseLemmaMap();
std::unordered_map<UINT64, Bucket*>* getReverseModernMap();
std::unordered_map<UINT64, UINT16>* getReverseSearchMap();
std::unordered_map<UINT64, UINT16>* getReverseNameMap();

std::unordered_map<UINT64, BYTE>* getSlashBoundaryMap();   // examples: /BoV/ /BoC/ /EoB/
std::unordered_map<UINT64, BYTE>* getSlashPuncMap();       // examples: /;/ /./ /?/ /'/
std::unordered_map<UINT64, BYTE>* getPoundWordSuffixMap(); // examples: #kjv[1] #av[1] #exact[1] #avx[2] #modern[2] #any[3] #fuzzy[3]
std::unordered_map<UINT64, BYTE>* getPoundWordlessMap();   // examples: #diff
}
// Primary Byte (orthography-based features):
const BYTE FIND_Token_MASK          = 0xC0;
const BYTE FIND_Token               = 0x80; // examples: ran walked I
const BYTE FIND_Token_WithWildcard  = 0xC0; // examples: run* you*#modern
const BYTE FIND_Suffix_MASK         = 0x30;
const BYTE FIND_Suffix_Exact        = 0x10; // examples: #exact #kjv #av
const BYTE FIND_Suffix_Modern       = 0x20; // examples: #avx #modern
const BYTE FIND_Suffix_Either       = 0x30; // examples: #any #fuzzy
const BYTE FIND_Suffix_None         = 0x00; // use global setting search.exact, search.fuzzy, or search.modern
const BYTE FIND_Lemma               = 0x08; // examples: #run
const BYTE FIND_GlobalTest          = 0x04; // examples: #diff
const BYTE FIND_LANGUAGE_NUMERIC    = 0x03; // regex: #[0-9]*[ehg]
const BYTE FIND_English             = 0x03; // examples: #12345e
const BYTE FIND_Greek               = 0x02; // examples: #12345g
const BYTE FIND_Hebrew              = 0x01; // examples: #12345h

// Secondary Byte (other linguistic features /delimited by slashes/):
const BYTE SLASH_Boundaries         = 0x08; // examples: /BoV/ /BoC/ /EoB/
const BYTE SLASH_Puncuation         = 0x04; // examples: /;/ /./ /?/ /'/
const BYTE SLASH_DiscretePOS        = 0x02; // examples: /av/ /dt/ /n2-vhg/
const BYTE SLASH_BitwisePOS         = 0x01; // examples: /-01-/

const BYTE SLASH_RESERVED_80        = 0x80;
const BYTE SLASH_RESERVED_40        = 0x40;
const BYTE SLASH_RESERVED_20        = 0x20;
const BYTE SLASH_RESERVED_10        = 0x10;


#pragma pack(pop)   /* restore original alignment from stack */
#endif