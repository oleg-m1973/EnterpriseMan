#include "stdafx.h"
#include "EnterpriseMan.h"
#include "csv.h"

#include <cctype>
#include <thread>
#include <conio.h>

static
void WaitKeyPress()
{
	printf("Press any key...\n");
	while (!_kbhit())
		std::this_thread::sleep_for(100ms);

	_getch();
}

enum TRequest : int
{
	req_empty = -2,
	req_exit = -1,
	req_anybody = 0,
};


static
int DoRequest(const char *req)
{
	std::cout << std::endl << req << ">";

	std::string s;
	std::getline(std::cin, s);
	if (s.empty())
		return req_empty;

	const char ch = s[0];
	switch (ch)
	{
	case 'x':
	case 'X':
		return req_exit;
	default:
		if (std::isdigit(ch))
			return atoi(s.c_str());
	}

	return req_empty;
}

template <typename T>
auto LoadObject(const std::string &name)
{
	std::list<T> res;

	auto items = CSV::ReadFile(name);
	if (items.empty())
		RAISE_ERROR("File not found or empty", name);

	std::map<std::string, size_t> header;
	size_t i = 0;

	for (auto &item: items.front())
		header.emplace(std::move(item), i++);

	for (auto it = ++items.begin(), end = items.end(); it != end; ++it)
	{
		res.emplace_back();

		auto &row = *it;
		res.back().ForEachField([&header, &row](const std::string &name, std::string &val) mutable
		{
			auto it = header.find(name);
			if (it != header.end() && it->second < row.size())
				val = std::move(row[it->second]);
		});
	}

	return res;

	
}

void CCompany::Load(const std::string &db_path)
{
#define TS_ITEM(name) auto name = LoadObject<db::name>(db_path + "\\" #name ".csv"); 
	TS_TABLES
#undef TS_ITEM

	std::set<CDuty::ID> duties2;
	m_duties.reserve(duties.size());
	for (auto &item: duties)
	{
		if (!duties2.emplace(item.m_id).second)
			RAISE_ERROR("duties: Duplicate duty id", item.m_id);

		auto sp = std::make_unique<CDuty>(std::move(item));
		m_duties.emplace_back(std::move(sp));
	}

	std::map<CPosition::ID, CPosition *> positions2;
	m_positions.reserve(positions.size());
	for (auto &item: positions)
	{
		auto sp = std::make_unique<CPosition>(std::move(item));
		if (!positions2.emplace(sp->m_id, sp.get()).second)
			RAISE_ERROR("positions: Duplicate position", sp->m_id);

		m_positions.emplace_back(std::move(sp));
	}

	for (auto &item: position_duties)
	{
		if (duties2.find(item.m_duty_id) == duties2.end())
			RAISE_ERROR("position_duties: Invalid duty id", item.m_duty_id)

		auto it = positions2.find(item.m_pos_id);
		if (it == positions2.end())
			RAISE_ERROR("position_duties: Invalid position id", item.m_pos_id)

		it->second->m_duties.emplace(item.m_duty_id);
	}

	std::map<CDivision::ID, CDivision *> divisions2;
	while (!divisions.empty())
	{
		const size_t n = divisions.size();
		divisions.remove_if([this, &divisions2](db::divisions &item)
		{
			CUnit *p = this;
			if (!item.m_div_id.empty())
			{
				auto it = divisions2.find(item.m_div_id);
				if (it == divisions2.end())
					return false;

				p = it->second;
			}

			auto sp = std::make_unique<CDivision>(std::move(item), *this, *p);
			if (!divisions2.emplace(sp->m_id, sp.get()).second)
				RAISE_ERROR("divisions: Duplicate division", sp->m_id);

			p->m_units.emplace_back(std::move(sp));
			return true;
		});

		if (n == divisions.size())
			RAISE_ERROR("divisions: Root division not found");
	}

	for (auto &item: employees)
	{
		auto pos = positions2.find(item.m_pos_id);
		if (pos == positions2.end())
			RAISE_ERROR("employees: Position not found", item.m_pos_id);

		CUnit *div = this;
		if (!item.m_div_id.empty())
		{
			auto it = divisions2.find(item.m_div_id);
			if (it == divisions2.end())
				RAISE_ERROR("employees: Division not found", item.m_div_id);

			div = it->second;
		}

		auto sp = std::make_unique<CPerson>(*pos->second, std::move(item), *this, *div);
		div->m_units.emplace_back(std::move(sp));
	}
}

bool CCompany::QueryTask()
{
	system("cls");

	size_t i = 0;
	for (auto &item: m_duties)
		std::cout << (++i) << ": " << item->m_name << std::endl;

	std::cout << "0: Показать структуру предпиятия" << std::endl;
	std::cout << "x: Выход" << std::endl;
	

	auto req = DoRequest("Выберите задачу");
	switch (req)
	{
	case req_exit: return false;
	case req_empty: break;
	case 0:
		{
			system("cls");
			FormatVal(std::cout);
			WaitKeyPress();
		}
		break;
	default: 
		if (size_t(req - 1) < m_duties.size())
		{
			CTask task(*(m_duties[req - 1]));
			if (!DoTask(task, true))
				Print("Задача не выполнена", task->m_name);

			WaitKeyPress();
		}

	}

	return true;
}

bool CUnit::DoTask(const CTask &task, bool prompt)
{
	auto doers = GetDoers(*task);
	if (doers.empty())
		return false;

	if (doers.size() == 1)
		return doers.front()->DoTask(task, prompt);

	const bool common = task->IsCommon();
	int n = 0;
	while (prompt)
	{
		const auto &name = GetName();
		system("cls");

		std::cout << "Задача: " << task->m_name << std::endl;
		if (!name.empty())
			std::cout << "Исполнитель: " << name << std::endl;
		
		std::cout << std::endl;

		size_t i = 0;
		for (auto &item: doers)
			std::cout << (++i) << ": " << item->GetName() << std::endl;

		std::cout << (common? "0: Все": "0: Любой") << std::endl;
		std::cout << "x: Отменить" << std::endl;
		
		n = DoRequest("Выберите исполнителя");
		switch (n)
		{
		case req_exit: return false;
		case req_empty: continue;
		case 0: break;
		default: 
			if (size_t(n - 1) > doers.size())
				continue;
		}

		break;
	}

	std::cout << std::endl;

	if (n != 0)
		return doers[n - 1]->DoTask(task, true);

	bool done = false;
	
	for (auto &item: doers)
		if (item->DoTask(task, false))
		{
			if (!common)
				return true;
			
			done = true;
		}

	return done;
}

int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		std::cout << "usage: EnerpriseMan db_path" << std::endl;
		return -1;
	}

	setlocale(LC_ALL, "Russian");
	try
	{
		CCompany company;
		company.Load(argv[1]);
		//company.FormatVal(std::cout);

		while (company.QueryTask())
		{
		}
		return 0;
	}
	catch (const std::exception &e)
	{
		Print("ERROR", e.what());
	}
	catch (...)
	{
		Print("ERROR", "Unknown");
	}
    return -1;
}
