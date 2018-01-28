#pragma once
#include "Format.h"
#include <list>
#include <set>
#include <map>
#include <vector>
#include <memory>
#include <algorithm>

#define RAISE_ERROR(err, ...) RaiseError(err, ##__VA_ARGS__, __FILE__, __LINE__);

template <typename T, typename... TT>
void RaiseError(T &&err, TT&&... args)
{
	auto s = FormatStr(std::forward<T>(err), std::forward<TT>(args)...);
	throw std::logic_error(s.str());
}

#define TS_TABLES \
	TS_ITEM(divisions) \
	TS_ITEM(duties) \
	TS_ITEM(employees) \
	TS_ITEM(position_duties) \
	TS_ITEM(positions) \

#define TS_DUTIES \
	TS_ITEM(id) \
	TS_ITEM(name) \
	TS_ITEM(common) \

#define TS_POSITIONS \
	TS_ITEM(id) \
	TS_ITEM(name) \

#define TS_POSITION_DUTIES \
	TS_ITEM(pos_id) \
	TS_ITEM(duty_id) \

#define TS_DIVISIONS \
	TS_ITEM(id) \
	TS_ITEM(div_id) \
	TS_ITEM(name) \

#define TS_EMPLOYEES \
	TS_ITEM(name) \
	TS_ITEM(div_id) \
	TS_ITEM(pos_id) \

namespace db
{
#define DECL_RECORD(name, ...) struct name {__VA_ARGS__ \
	template <typename TFunc, typename... TT> void ForEachField(TFunc &&func, TT&&... args);};

#define TS_ITEM(name) std::string m_##name;
DECL_RECORD(duties, TS_DUTIES)
DECL_RECORD(divisions, TS_DIVISIONS)
DECL_RECORD(employees, TS_EMPLOYEES)
DECL_RECORD(position_duties, TS_POSITION_DUTIES)
DECL_RECORD(positions, TS_POSITIONS)
#undef TS_ITEM
#undef DECL_RECORD

#define DECL_RECORD(name, ...) template <typename TFunc, typename... TT> void name::ForEachField(TFunc &&func, TT&&... args) {__VA_ARGS__};
#define TS_ITEM(name) std::invoke(func, args..., #name##s, m_##name);
DECL_RECORD(duties, TS_DUTIES)
DECL_RECORD(divisions, TS_DIVISIONS)
DECL_RECORD(employees, TS_EMPLOYEES)
DECL_RECORD(position_duties, TS_POSITION_DUTIES)
DECL_RECORD(positions, TS_POSITIONS)
#undef TS_ITEM
#undef DECL_RECORD
}

class CCompany;

class CDuty
: public db::duties
{
public:
	typedef std::string ID;
	CDuty(db::duties &&src)
	: db::duties(std::move(src))
	{
	}

	bool IsCommon() const
	{
		return m_common;
	}

	void FormatVal(std::ostream &out) const
	{
		FormatVals(out, m_id, m_name, m_common);
	}

protected:
	const bool m_common{db::duties::m_common == "1"s};
};

class CTask
{
public:
	CTask(const CDuty &duty)
	: m_duty(duty)
	{
	}

	const CDuty &operator *() const
	{
		return m_duty;
	}
	const CDuty *operator ->() const
	{
		return &m_duty;
	}
protected:
	const CDuty &m_duty;
};
class CPosition
: public db::positions
{
friend class CCompany;
public:
	typedef std::string ID;
	
	CPosition(db::positions &&src)
	: db::positions(std::move(src))
	{
	}

	bool HasDuty(const CDuty &duty) const 
	{
		return duty.IsCommon() || m_duties.find(duty.m_id) != m_duties.end();
	}

	void FormatVal(std::ostream &out) const
	{
		FormatVals(out, m_id, m_name) << ", Duties:";
		for (auto &item: m_duties)
			out << item << "|";
	}


protected:
	std::set<CDuty::ID> m_duties;
};

class CUnit
{
friend class CCompany;
public:
	CUnit(CUnit *base = nullptr)
	: m_base(base)
	{
	}

	virtual ~CUnit()
	{
	}

	virtual const std::string &GetName() const
	{
		static const std::string _name;
		return _name;
	}
	
	virtual bool HasDuty(const CDuty &duty) const 
	{
		if (duty.IsCommon())
			return true;

		auto it = std::find_if(m_units.begin(), m_units.end(), [&duty](const auto &item)
		{
			return item->HasDuty(duty);
		});

		return it != m_units.end();
	}

	virtual void FormatVal(std::ostream &out, size_t tabs = 0) const
	{
		for (auto &item: m_units)
			item->FormatVal(out, tabs);
	}

	virtual bool DoTask(const CTask &task, bool prompt);

protected:
	auto GetDoers(const CDuty &duty) const
	{
		std::vector<CUnit *> res;
		res.reserve(m_units.size());
		for (auto &item: m_units)
			if (item->HasDuty(duty))
				res.emplace_back(item.get());
		
		return res;
	}

	CUnit *m_base = nullptr;
	std::list<std::unique_ptr<CUnit>> m_units;
};

template <typename T>
class CCompanyUnit
: public T
, public CUnit
{
public:
	CCompanyUnit(T &&src, CCompany &company, CUnit &base)
	: T(std::move(src))
	, CUnit( &base)
	, m_company(company)
	{
	}

protected:
	CCompany &m_company;
};

class CPerson
: public CCompanyUnit<db::employees>
{
public:
	template <typename... TT>
	CPerson(CPosition &pos, TT&&... args)
	: CCompanyUnit<db::employees>(std::forward<TT>(args)...)
	, m_pos(pos)
	{
	}

	virtual const std::string &GetName() const override
	{
		return m_name2;
	}

	virtual bool HasDuty(const CDuty &duty) const 
	{
		return m_pos.HasDuty(duty);
	}

	virtual bool DoTask(const CTask &task, bool prompt) override
	{
		std::cout << "Задача " << task->m_name << " выполнена. Исполнитель: ";
		FormatVals(std::cout, m_name, m_pos.m_name, m_base->GetName()) << std::endl;
		return true;
	}

	virtual void FormatVal(std::ostream &out, size_t tabs = 0) const override
	{
		if (tabs)
			std::fill_n(std::ostream_iterator<char>(out), tabs, '\t');

		FormatVals(out, GetName(), m_base->GetName()) << std::endl;
	}

protected:
	CPosition &m_pos;
	std::string m_name2{m_name + ", "s + m_pos.m_name};
};

class CDivision
: public CCompanyUnit<db::divisions>
{
friend class CCompany;
public:
	typedef std::string ID;
	using CCompanyUnit<db::divisions>::CCompanyUnit;
	template <typename... TT>

	CDivision(CPosition &pos, TT&&... args)
	: CCompanyUnit<db::divisions>(std::forward<TT>(args)...)
	{
	}

	virtual const std::string &GetName() const override
	{
		return m_name2;
	}

	virtual void FormatVal(std::ostream &out, size_t tabs = 0) const override
	{
		if (tabs)
			std::fill_n(std::ostream_iterator<char>(out), tabs, '\t');
		
		out << m_name << std::endl;

		CCompanyUnit<db::divisions>::FormatVal(out, ++tabs);
	}
protected:
	
	std::string m_name2{m_base->GetName() + "/"s + m_name};
};

class CCompany
: protected CUnit
{
public:
	CCompany()
	{
	}

	void Load(const std::string &db_path);

	bool QueryTask();

	void FormatVal(std::ostream &out) const
	{
		out << "Duties:" << std::endl;
		for (auto &item : m_duties)
		{
			item->FormatVal(out << '\t');
			out << std::endl;
		}

		out << "Positions:" << std::endl;
		for (auto &item : m_positions)
		{
			item->FormatVal(out << '\t');
			out << std::endl;
		}

		out << "Divisions:" << std::endl;
		CUnit::FormatVal(out, 1);
	}

protected:

	std::vector<std::unique_ptr<CDuty>> m_duties;
	std::vector<std::unique_ptr<CPosition>> m_positions;
};

