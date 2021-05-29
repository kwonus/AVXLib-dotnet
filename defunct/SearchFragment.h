#include <FeatureSpec.h>
#include <slicable.h>
#include <vector>
#pragma once

struct SearchFragment
{
	UINT32*			positionAspects;
//	FeatureSpec*	matchAll;
	char*			text;
};

class CSearchFragment: slicable
{
public:
	const UINT32* const			positionAspects;
	std::vector<CFeatureSpec*>	spec;	// spec is "All Of" features in the specification

	CSearchFragment(SearchFragment& fragment, slicable& text, UINT32 begin, UINT32 end) : slicable(text, begin, end), positionAspects { fragment.positionAspects }
	{
		if (this->len >= 0) {
			UINT32 previous = 0;
			for (UINT32 i = 0; i < this->len; i++)
				if (this->input[i] == '&') {
					if (previous < i)	// eliminate empty items
						spec.push_back(new CFeatureSpec(*this, previous, i - previous));
					previous = i + 1;
				}
		}
	}
	virtual ~CSearchFragment() 
	{
		if (!spec.empty())
			for (auto obj = spec.begin(); obj < spec.end(); obj = obj++)
				delete* obj;
	}

};
