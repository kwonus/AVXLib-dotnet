#include <slicable.h>
#include <vector>
#pragma once

struct Feature
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