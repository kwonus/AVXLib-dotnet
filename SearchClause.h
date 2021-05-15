#include <SearchFragment.h>
#include <slicable.h>
#include <vector>
#pragma once

struct SearchClause
{
	SearchFragment*	fragments;
	char*			segment;
	char			polarity;
};

class CSearchClause: slicable
{
public:
	std::vector<CSearchFragment*> fragments;
	const char		polarity;

	CSearchClause(SearchClause& search) : slicable(search.segment), polarity{ search.polarity }, structure{ search }
	{
		if (this->len >= 0) {
			UINT32 previous = 0;
			for (UINT32 i = 0; i < this->len; i++)
				if (this->input[i] == ' ' || this->input[i] == '\t') {
					if (previous < i)	// eliminate empty items
						fragments.push_back(new CSearchFragment(search.fragments[i], *this, previous, i - previous));
					previous = i + 1;
				}
		}
	}
	virtual ~CSearchClause()
	{
		if (!fragments.empty())
			for (auto obj = fragments.begin(); obj < fragments.end(); obj = obj++)
				delete* obj;
	}

protected:
	SearchClause& structure;

	//	Use this carefully, because calling the destructor on the owner will invalidate memory of all copies
	CSearchClause(const CSearchClause& search) : slicable(search), polarity{ search.polarity }, structure{ search.structure }
	{
		this->structure = search.structure;
	}
};