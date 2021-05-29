#include <slicable.h>
#include <vector>
#pragma once
/*
// These are redundantly defined here (also in managed space):
const UINT16 typeNOT		= 0x8000;
const UINT16 typeWord		= 0x0800;
const UINT16 typeWordAV		= 0x1800;
const UINT16 typeWordAVX	= 0x2800;
const UINT16 typeWordAny	= 0x3800;
const UINT16 typeWordSame	= 0x4800;
const UINT16 typeWordDiff	= 0xC800; // not same

const UINT16 typeEnglish	= 0x0400;
const UINT16 typeHebrew		= 0x0200;
const UINT16 typeGreek		= 0x0100;
const UINT16 typeLemma		= 0x0080;
const UINT16 typePOSBits	= 0x0040;
const UINT16 typePOS		= 0x0020;
const UINT16 typePunct		= 0x0010;
const UINT16 typeBounds		= 0x0008;
const UINT16 typeReserved4	= 0x0004;
const UINT16 typeReserved2	= 0x0002;
const UINT16 typeReserved1	= 0x0001;
*/
struct FeatureStruct
{
	char*	feature;
	char	featureType;
	UINT16* featureVector;
};

class CFeature: slicable
{
public:
	CFeature(slicable& feature, UINT32 begin, UINT32 end, char type)
		: slicable(feature, begin, end), featureType{ type }
	{
		this->featureVector = NULL;
	}
	CFeature(slicable& feature, UINT32 begin, UINT32 end, char type, UINT16* vector)
		: slicable(feature, begin, end), featureType{ type }
	{
		this->featureVector = vector;
	}
	virtual ~CFeature()
	{
		if (this->featureVector != NULL)
			free(this->featureVector);
	}
	const char	featureType;
	inline const UINT16* const getFeatureVector()
	{
		if (featureVector == NULL) {
			;
		}
		return featureVector;
	}
private:
	UINT16* featureVector;
};