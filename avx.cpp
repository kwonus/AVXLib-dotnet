#include "avx.h"
#include "fivebitencoding.h"

// Uses shared memory
static XVMem<AVWrit> Writ;
static XVMem<BYTE> Lexicon;
static XVMem<BYTE> Lemma;
static XVMem<BYTE> LemmaOOV;

// Uses memory on the heap
XVMem<AVBook> allocAVBook;
XVMem<AVChapter> allocAVChapter;
XVMem<AVVerse> allocAVVerse;
XVMem<BYTE> allocAVName;
XVMem<BYTE> allocAVWordClass;


static AVWrit*	 writ;		// [1+0xC0C93];
static AVBook*	 books;		// [66];
static AVChapter*chapters;	// [1+0x4A4];
static AVVerse*	 verses;	// [1+0x797D];
// Maps
static std::unordered_map<UINT64, AVLemma*> lemma;
static std::unordered_map<UINT16, char*> lemmaOOV;
static std::unordered_map<UINT16, AVLexicon*> lexicon;
static std::unordered_map<UINT16, AVWordClass*> wclass;
static std::unordered_map<UINT16, AVName*> names;

static std::unordered_map<UINT16, char*> forwardLemma;
static std::unordered_map<UINT64, Bucket*> reverseLemma;
static std::unordered_map<UINT64, Bucket*> reverseModern;
static std::unordered_map<UINT64, UINT16> reverseSearch;
static std::unordered_map<UINT64, UINT16> reverseName;

static std::unordered_map<UINT64, BYTE> slashBoundaries; // examples: /BoV/ /BoC/ /BoB/ /EoV/ /EoC/ /EoB/
static std::unordered_map<UINT64, BYTE> slashPunc;       // examples: /;/ /./ /?/ /'/
static std::unordered_map<UINT64, BYTE> poundWordSuffix; // examples: #kjv[1] #av[1] #exact[1] #avx[2] #modern[2] #any[3] #fuzzy[3]
static std::unordered_map<UINT64, BYTE> poundWordless;   // examples: #diff
// Additional Patterns:
//                                      Discrete POS     // examples: /av/ /dt/ /n2-vhg/   [morh-adorner]
//                                      bitwise POS      // examples: /-01-/ [noun] /-1--/ [verb] /6028/ [pronoun+2PS+genetive] /-03-/ [proper noun]
//                                      GREEK:           // examples: G12345 12345G
//                                      HEBREW:          // examples: H12345 12345H
//                                      ENGLISH:         // examples: E12345 12345E  // from AV-Lexicon
//                                      LEMMA:           // examples: #run
//                                      WILDCARDS:       // examples: run* you*#modern
//                                      COMBINATIONS:    // examples: t*#av&/pn#2ps/&#diff

//  Fixed-Length binary files:
#define AVTEXT		"AV-Writ.dx"
#define AVBOOK		"AV-Book.ix"
#define AVCHAPTER	"AV-Chapter.ix"
#define AVVERSE		"AV-Verse.ix"

//  variable-Length binary files:
#define AVLEXICON	"AV-Lexicon.dxi"
#define AVLEMMA		"AV-Lemma.dxi"
#define AVLEMMAOOV	"AV-Lemma-OOV.dxi"
#define AVNAMES		"AV-Names.dxi"
#define AVWCLASS	"AV-WordClass.dxi"

// Only works on intel-byte-order
//
inline UINT16 uint16(BYTE* buffer) {
    return *((UINT16*)buffer);
}
// Only works on intel-byte-order
//
inline UINT32 uint32(BYTE* buffer) {
    return *((UINT32*)buffer);
}
// Only works on intel-byte-order
//
inline UINT64 uint64(BYTE* buffer) {
    return *((UINT64*)buffer);
}
extern "C" void release()
{
    lemma.clear();
    lemmaOOV.clear();
    lexicon.clear();
    wclass.clear();
    names.clear();

    forwardLemma.clear();

    for (auto it = reverseLemma.begin(); it != reverseLemma.end(); ++it)
        delete it->second;
    reverseLemma.clear();

    for (auto it = reverseModern.begin(); it != reverseModern.end(); ++it)
        delete it->second;
    reverseModern.clear();

    Writ.Release();
    Lexicon.Release();
    Lemma.Release();
    LemmaOOV.Release();
    allocAVBook.Release();
    allocAVChapter.Release();
    allocAVVerse.Release();
    allocAVName.Release();
    allocAVWordClass.Release();

    writ = NULL;
    books = NULL;
    chapters = NULL;
    verses = NULL;
}
extern "C" UINT32 getWritCnt()
{
    return Writ.GetCnt(); // 1+0xC0C93
}
extern "C" UINT16 getBookCnt()
{
    return allocAVBook.GetCnt(); // 66;
}
extern "C" UINT32 getChapterCnt()
{
    return allocAVChapter.GetCnt(); // 1+0x4A4
}
extern "C" UINT32 getVerseCnt()
{
    return allocAVVerse.GetCnt(); // 1+0x797D
}
///
extern "C" AVWrit* getWrit(UINT32 idx)
{
    return idx < Writ.GetCnt() ? writ + idx : NULL;
}
extern "C" AVBook& getBook(UINT16 idx)
{
    return idx < allocAVBook.GetCnt() ? books[idx] : books[allocAVBook.GetCnt() - 1];
}
extern "C" AVBook& getBookByNum(UINT16 num)
{
    return num > 0 ? getBook(num-1) : books[0];
}
extern "C" AVChapter& getChapter(UINT16 idx)
{
    return idx < allocAVChapter.GetCnt() ? chapters[idx] : chapters[allocAVChapter.GetCnt()-1];
}
extern "C" AVVerse& getVerse(UINT16 idx)
{
    return idx < allocAVVerse.GetCnt() ? verses[idx] : verses[allocAVVerse.GetCnt()-1];
}
// These are the only variable length entries other than pos.  Record can be used directly for other firelds.
extern "C" char* getLexicalEntry(UINT16 rawkey, BYTE sequence)	// seq=0:search; seq=1:display; seq=2:modern;
{
    UINT16 key = rawkey & 0x3FFF;
    if (key < 1 || key > 12567)
        return NULL;

    auto lex = lexicon.at(key);
    auto entries = (char*)lex;
    char* search = entries + (sizeof(UINT16) + sizeof(UINT16) + (lex->posCnt * sizeof(UINT32)));

    if (sequence == SEARCH)
        return search;

    char* display = search + 1 + strlen(search);

    if (sequence == DISPLAY)
        return (*display != (char)0) ? display : search;

    char* modern = display + 1 + strlen(display);

    if (sequence == MODERN)
        return (*modern != (char)0) ? modern : (*display != (char)0) ? display : search;

    if (sequence == MODERN_WITHOUT_HYPHENS)
        return (*modern != (char)0) ? modern : search;

    return NULL;
}
extern "C" char* getOovEntry(UINT16 key)
{
    return lemmaOOV.count(key) != 0 ? lemmaOOV.at(key) : NULL;
}
extern "C" UINT16 getLemma(UINT32 pos, UINT16 wkey, char* data[], UINT16 arrayLen)
{
    UINT64 hashKey = (((UINT64)pos) << 32) + wkey;
    AVLemma* record = lemma.count(hashKey) != 0 ? lemma.at(hashKey) : NULL;

    if (record != NULL) {
        if (arrayLen < 1 || data == NULL)
            return record->lemmaCnt;

        BYTE* location = (BYTE*)(&record->lemma);
        UINT16 i;
        for (i = 0; i < record->lemmaCnt; i++, location += sizeof(UINT16)) {
            char* lemma;
            UINT16 key = uint16(location);
            if ((key & 0x8000) == 0x8000)   // this lemma is OOV
            {
                lemma = lemmaOOV[key];
            }
            else
            {
                bool modernized = (key & 0x4000) == 0x4000;
                lemma = !modernized ? getLexicalEntry(key, DISPLAY) : getLexicalEntry(key &0x7FFF, MODERN);
            }
            if (i < arrayLen)
                data[i] = lemma;
        }
        for (UINT16 r = i + 1; r < arrayLen; r++)
            data[i] = NULL;
        return record->lemmaCnt;
    }
    return 0;
}
///

extern "C" void initialize(char * folder)
{
    if (folder != NULL)
        Strncpy(g_hSharedHome, folder, MAX_PATH);
    else
        g_hSharedHome[0] = (char)0;

    writ = Writ.Acquire(AVTEXT, false, true);
    books = allocAVBook.Acquire(AVBOOK, false, false);
    chapters = allocAVChapter.Acquire(AVCHAPTER, false, false);
    verses = allocAVVerse.Acquire(AVVERSE, false, false);

    // Process Lexicon
    UINT16 lexnum = 1;
    {
        BYTE* lex = Lexicon.Acquire(AVLEXICON, false, true);
        int cnt = Lexicon.GetCnt(); // last insertion of lexnum should finish be 12567 (size/cnt is MUCH bigger)
        BYTE* last = lex + cnt - 1;

        for (lexnum = 1; lex <= last && ((UINT32*)lex)[1] != 0xFFFFFFFF; lexnum ++) {
            auto record = (AVLexicon*) lex;
            lexicon.insert({ lexnum, record });
            // add slot for entities
            lex += sizeof(UINT16);
            // add slot for size (POS)
            lex += sizeof(UINT16);
            // add slot for each POS
            lex += (sizeof(UINT32) * record->posCnt);
            // Get counts for search, modern, and search ..
            UINT64 hash;
            for (BYTE x = SEARCH; x <= MODERN; x++) {
                char* token = (char*)lex;
                if (*token != 0) switch (x) {
                    case SEARCH:    hash = Hash64(token); 
                                    reverseSearch.insert({ hash, lexnum });
                                    break;
                    case MODERN:    if (*token != 0)
                                        hash = Hash64(token);
                                        if (reverseModern.count(hash) == 0) {
                                            reverseModern.insert({ hash, new Bucket(lexnum) });
                                        }
                                        else {
                                            Bucket* bucket = reverseModern.at(hash);
                                            bucket->AddOverflow(lexnum);
                                        }
                                    break;
                }
                lex += (1 + strlen(token));
            }
        }
    }
    // Process AVLemmaOOV
    {
        BYTE* lemmOOV = LemmaOOV.Acquire(AVLEMMAOOV, false, true);
        int bcnt = LemmaOOV.GetCnt();
        BYTE* last = lemmOOV + bcnt - 1; // last UINT32 (+8 for previous record) of file are sizing data; and ignored here)

        UINT16 len = 0;
        for (/**/; lemmOOV < last; lemmOOV += (sizeof(UINT16) + len)) {
            auto record = (AVLemmaOOV*)lemmOOV;
            lemmaOOV.insert({ record->oovKey, &(record->lemma) });
            if (record->oovKey == 0x8F01)
                break;
            len = 1 + ((record->oovKey & 0x0F00) >> 8);
        }
    }
    // Process AVLemma
    {
        BYTE* lemm = Lemma.Acquire(AVLEMMA, false, true);
        int bcnt = Lemma.GetCnt();
        BYTE* last = lemm + bcnt - 1; // last UINT32 (+8 for previous record) of file are sizing data; and ignored here)

        UINT64 hashKey;
        for (int i = 0; lemm <= last && uint32(lemm) != 0xFFFFFFFF; i++) {
            auto record = (AVLemma*) lemm;

            char* token = NULL;
            switch (record->wordKey & 0xC000) {
                case 0x8000:    token = getOovEntry(record->wordKey); break;
                case 0x4000:    token = getLexicalEntry(record->wordKey, MODERN_WITHOUT_HYPHENS); break;
                case 0x0000:    token = getLexicalEntry(record->wordKey, SEARCH); break;
            }
            if (token != NULL) {
                forwardLemma.insert({ record->wordKey, token });
                UINT64 hash = Hash64(token);
                if (reverseLemma.count(hash) == 0) {
                    reverseLemma.insert({ hash, new Bucket(record->wordKey) });
                }
                else {
                    Bucket* bucket = reverseLemma.at(hash);
                    bucket->AddOverflow(record->wordKey);
                }
            }

            hashKey = ((UINT64)(record->pos) << 32) + (UINT64)(record->wordKey);
            lemma.insert({ hashKey, record });

            // add slot for pos
            lemm += sizeof(UINT32);
            // add slot for wordkey
            lemm += sizeof(UINT16);
            // add slot for wordClass
            lemm += sizeof(UINT16);
            // add slot for Lemma Count and all lemma segment
            lemm += sizeof(UINT16);
            for (UINT16 x = 0; x < record->lemmaCnt; x++) {
                UINT16 key = uint16(lemm);
                lemm += sizeof(UINT16);
            }
        }
    }
    // Process AVNames
    {
        auto bytes = allocAVName;
        AVName* record;
        BYTE* data = bytes.Acquire(AVNAMES, false, false);
        int bcnt = bytes.GetCnt();
        for (auto end = data + bcnt; data < end; /**/) {
            record = (AVName*)data;
            auto nameKey = uint16(data);
            data += sizeof(UINT16);
            for (/**/; *data != '\0'; data++)
                ;
            data++;	// skip the null terminator
        }
    }
    // Process AVWordClass
    {
        auto bytes = allocAVWordClass;

        AVWordClass* record;
        BYTE* data = bytes.Acquire(AVWCLASS, false, false);
        int bcnt = bytes.GetCnt();

        for (auto end = data + bcnt; data < end; /**/) {
            record = (AVWordClass*)data;
            wclass.insert({ record->wordClass, record });
            data += 2 * sizeof(UINT16);
            data += record->width * sizeof(UINT32);
        }
    }
}
static const UINT64 BoV = Hash64("BoV");
static const UINT64 EoV = Hash64("EoV");
static const UINT64 BoC = Hash64("BoC");
static const UINT64 EoC = Hash64("EoC");
static const UINT64 BoB = Hash64("BoB");
static const UINT64 EoB = Hash64("EoB");

std::unordered_map<UINT64, BYTE>* getSlashBoundaryMap() {
    if (slashBoundaries.count(BoV) == 0) {
        slashBoundaries.insert({ BoV, 0x20 });
        slashBoundaries.insert({ EoV, 0x30 });
        slashBoundaries.insert({ BoC, 0x60 });
        slashBoundaries.insert({ EoC, 0x70 });
        slashBoundaries.insert({ BoB, 0xE0 });
        slashBoundaries.insert({ EoB, 0xF0 });
    }
    return &slashBoundaries;
}
std::unordered_map<UINT64, BYTE>* getSlashPuncMap() {
    if (slashPunc.count(Hash64("!")) == 0) {
        slashPunc.insert({ Hash64("!"), 0x80 });
        slashPunc.insert({ Hash64("?"), 0xC0 });
        slashPunc.insert({ Hash64("."), 0xE0 });
        slashPunc.insert({ Hash64("-"), 0xA0 });
        slashPunc.insert({ Hash64(";"), 0x20 });
        slashPunc.insert({ Hash64(","), 0x40 });
        slashPunc.insert({ Hash64(":"), 0x60 });
        slashPunc.insert({ Hash64("'"), 0x10 });
        slashPunc.insert({ Hash64(")"), 0x0C });
        slashPunc.insert({ Hash64("("), 0x04 });
        slashPunc.insert({ Hash64("italics"), 0x02 });
        slashPunc.insert({ Hash64("jesus"), 0x01 });
    }
    return &slashPunc;
}
std::unordered_map<UINT64, BYTE>* getPoundWordSuffixMap() { // examples: #kjv[1] #av[1] #exact[1] #avx[2] #modern[2] #any[3] #fuzzy[3]
    if (poundWordSuffix.count(Hash64("#av")) == 0) {
        poundWordSuffix.insert({ Hash64("#av"), 1 });
        poundWordSuffix.insert({ Hash64("#kjv"), 1 });
        poundWordSuffix.insert({ Hash64("#exact"), 1 });
        poundWordSuffix.insert({ Hash64("#avx"), 2 });
        poundWordSuffix.insert({ Hash64("#modern"), 2 });
        poundWordSuffix.insert({ Hash64("#any"), 3 });
        poundWordSuffix.insert({ Hash64("#fuzzy"), 3 });
    }
    return &poundWordSuffix;
}
const BYTE DIFF = 1;
std::unordered_map<UINT64, BYTE>* getPoundWordlessMap() {   // examples: #diff
    if (poundWordless.count(Hash64("#diff")) == 0) {
        poundWordSuffix.insert({ Hash64("#diff"), DIFF });
    }
    return &poundWordless;
}
std::unordered_map<UINT16, char*>* getLemmaOovMap()
{
    return &lemmaOOV;
}
std::unordered_map<UINT64, AVLemma*>* getLemmaMap()
{
    return &lemma;
}
std::unordered_map<UINT16, AVLexicon*>* getLexiconMap()
{
    return &lexicon;
}
std::unordered_map<UINT16, AVWordClass*>* getWclassMap()
{
    return &wclass;
}
std::unordered_map<UINT16, AVName*>* getNamesMap()
{
    return &names;
}

std::unordered_map<UINT16, char*>* getForwardLemmaMap()
{
    return &forwardLemma;
}
std::unordered_map<UINT64, Bucket*>* getReverseLemmaMap()
{
    return &reverseLemma;
}
std::unordered_map<UINT64, Bucket*>* getReverseModernMap()
{
    return &reverseModern;
}
std::unordered_map<UINT64, UINT16>* getReverseSearchMap()
{
    return &reverseSearch;
}
std::unordered_map<UINT64, UINT16>* getReverseNameMap()
{
    return &reverseName;
}