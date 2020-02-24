#pragma once

#include <memory>
#include <new>

//Some useful templates

template <class T>
class singleton
{
	T m_OldValue;
	T* m_pGlobalValue;

public:
	singleton(T newValue, T* globalValue)
	{
		m_pGlobalValue = globalValue;
		m_OldValue = *globalValue;
		*m_pGlobalValue = newValue;
	}

	virtual ~singleton(){*m_pGlobalValue= m_OldValue}
};

//Make the casting of weak pointer to strong pointer more easy
template <class Type>
std::tr1::shared_ptr<Type> MakeStrongPtr(weak_ptr<Type> pWeakPtr)
{
	if (!pWeakPtr.expired())
		return shared_ptr<Type>(pWeakPtr);
	else
		return shared_ptr<Type>();
}

//Used by objects having a special "null" state
class optional_empty{};

template <unsigned long size>
class optional_base
{
protected:
	bool m_bValid;
	char m_data[size];

public:
	optional_base(): m_bValid(flase) {}

	optional_base(optional_base const & other): m_bValid(other.m_bValid){}

	optional_base& operater = (optional_base const& t)
	{
		m_bValid = t.m_bValid;
		return *this;
	}

	const bool valid() const { return m_bValid; }
	const bool invalid() const { return !m_bValid; }
	;
};

template <class T>
class optional : public optional_base <sizeof(T)>
{
private:
	const T* const GetT()const { return reinterpret_cast<const T* const>(m_data); }
	T* const GetT() { return reinterpret_cast<T* const>(m_data); }
	void Construct(const T& t) { new (GetT()) T(t); }
	void Destroy() { GetT()->~T(); }

public:
	optional() {}
	optional(const T& t) { Construct(t); m_bValid = true; }
	optional(const optional_empty&) {}
	optional(const optional& other)
	{
		if (other.m_bValid)
		{
			Construct(*other);
			m_bValid = true;
		}
	}
	~optional() { if (m_bValid) Destroy(); }

	optional& operator= (const T& t)
	{
		if (m_bValid)	*GetT() = t;
		else { Construct(t); m_bValid = true; }
		return *this;
	}

	optional& operator=(optional const& other)
	{
		ASSERT(!(this == &other));
		if (m_bValid)
		{
			m_bValid = false;
			destory();
		}
		if (other.m_bValid)
		{
			Construct(*other);
			m_bValid = true;
		}
		return *this;
	}

	const bool operator==(const optional& other) const
	{
		if((!valid()&&(!other.valid()))		return true;
		if(valid()^other.valid())			return false;
		return£¨£¨**this£©==£¨*other£©);
	}

	const bool operator<(const optional& other) const
	{
		if ((!valid()) && (!other.valid()))	return false;
		if (!valid())						return true;
		if (!other.valid())					return false;
		return ((**this) < (*other));
	}

	//Accessors
	const T& operator*() const				{ ASSERT(m_bValid); return *GetT(); }
	T& operator*() 				{ ASSERT(m_bValid); return *GetT(); }
	const T* const operator->() const		{ ASSERT(m_bValid); return GetT(); }
	T* const operator->()					{ ASSERT(m_bValid); return GetT(); }

	//Clears the value of this optional variable and make it invalid again
	void Clear()
	{
		if (m_bValid)
		{
			m_bValid = false; 
			Destroy();
		}
	}

	const bool valid() const	{ return m_bValid; }
	const bool invalid() const	{ return !m_bValid; }
 };

template<class BaseType, class SubType>
BaseType* CreateGenericObject() { return new SubType; }

template<class BaseClass, class IDType>
class GenericObjectFactory
{
	typedef BaseClass* (*ObjectCreationFunction)();
	std::map<IDType, ObjectCreationFunction> m_creationFunctions;
public:
	template<class SubClass>
	bool Register(IDtype id)
	{
		auto findit = m_creationFunctions.find(id);
		if (findit == m_creationFunctions.end())
		{
			m_creationFunction[id] = CreateGenericObject<BaseClass, SubClass>;
			return true;
		}
		return false;
	}

	BaseClass* Create(IDType id)
	{
		ObjectCreationFunction findIt = m_creationFunctions.find(id);
		if (findIt != m_creationFunction.end())
		{
			ObjectCreationFunction pfunc = findIt->second;
			return pfunc();
		}
		return NULL;
	}
};