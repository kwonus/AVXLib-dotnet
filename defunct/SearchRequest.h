#include <SearchClause.h>
#include <SearchControls.h>
#include <vector>
#pragma once

struct SearchRequest
{
	SearchClause*	clauses;
	SearchControls	controls;
	UINT64			count;
};

class CSearchRequest
{
public:
	std::vector<CSearchClause*>	clauses;
	const SearchControls&		controls;

	CSearchRequest(SearchRequest& request) : controls{ request.controls }
	{
		for (int i = 0; i < request.count; i++) {
			clauses.push_back(new CSearchClause(request.clauses[i]));
		}
	}
	virtual ~CSearchRequest()
	{
		if (!clauses.empty())
			for (auto obj = clauses.begin(); obj < clauses.end(); obj = obj++)
				delete *obj;
	}
};