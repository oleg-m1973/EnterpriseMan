#pragma once
#include <string>
#include <fstream>
#include <list>
#include <vector>

namespace CSV
{
typedef std::vector<std::string> TRow;

static
auto ReadFile(const std::string &file_name)
{
	std::list<TRow> res;
	std::ifstream in(file_name);

	size_t n = 0;
	while (in)
	{
		std::string s;
		std::getline(in, s);
		if (s.empty())
			continue;

		std::vector<std::string> vals;
		vals.reserve(n);

		size_t pos = 0;
		for (;;)
		{
			const auto pos2 = s.find(',', pos);
			if (pos2 == std::string::npos)
				break;

			vals.emplace_back(s, pos, pos2 - pos);
			pos = pos2 + 1;
		}

		vals.emplace_back(s, pos);
		
		n = vals.size();
		res.emplace_back(std::move(vals));
	}

	return res;
}
}