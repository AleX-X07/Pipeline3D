#include <c3d/componentfactory.h>
#include <c3d/component.h>

#include <gallium/string_utils.h>
#include <gallium/core/log.h>

using namespace c3d;

struct ComponentFactory::Internal
{
	List list;
	std::unordered_map<string_hash_t, ComponentConstructorBase*> ctors;
};

ComponentFactory::ComponentFactory()
	: m_pImpl(new Internal)
{
}

ComponentFactory::~ComponentFactory() noexcept
{
	Terminate();
}

void ComponentFactory::Terminate()
{
	for (auto& a : m_pImpl->ctors)
		delete a.second;

	m_pImpl->ctors.clear();
	m_pImpl->list.components.clear();
	m_pImpl->list.subcategories.clear();
}

void ComponentFactory::RegisterComponent(const std::string& category, const ga::core::CttiTypeInfo* typeInfo, ComponentConstructorBase* ctor)
{
	List& activeList = const_cast<List&>(GetComponents(category));

	std::string longName  = typeInfo->GetTypeName();
	std::string shortName = ga::string_replace(longName, "::", ":");

	std::vector<std::string> tokens;
	ga::string_split(tokens, shortName, ':');
	shortName = tokens.back();

	activeList.components.push_back(std::make_pair(shortName, typeInfo->GetType()));
	m_pImpl->ctors[typeInfo->GetType()] = ctor;
	GA_LOG_INFO("ga.engine.ComponentFactory", category + ": registered " + shortName);
}

Component* ComponentFactory::CreateComponent(string_hash_t id, GameObject* owner)
{
	if (m_pImpl->ctors.count(id))
		return m_pImpl->ctors[id]->Construct(owner);
	return nullptr;
}

const ComponentFactory::List& ComponentFactory::GetComponents(const std::string& category /* = "" */)
{
	std::vector<std::string> tokens;

	List* activeList = &m_pImpl->list;
	
	if (!category.empty())
	{
		ga::string_split(tokens, category, '/');
		for (auto& t : tokens)
			activeList = &(activeList->subcategories[t]);
	}
	
	return *activeList;
}
