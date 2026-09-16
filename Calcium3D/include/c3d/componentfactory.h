#ifndef C3D__COMPONENTFACTORY_H
#define C3D__COMPONENTFACTORY_H
#pragma once

#include <gallium/globalinstance.h>
#include <gallium/core/ctti.h>

#include <c3d/component.h>

namespace c3d
{
	class GameObject;

	class ComponentFactory final
		: public ga::GlobalInstance<ComponentFactory>
	{
		struct Internal;
		std::unique_ptr<Internal> m_pImpl;

	public:
		struct List
		{
			std::vector<std::pair<std::string, string_hash_t>> components;
			std::unordered_map<std::string, List>              subcategories;
		};

		explicit ComponentFactory();
		~ComponentFactory() noexcept;

		void Terminate() override;

		void RegisterComponent(const std::string& category, const ga::core::CttiTypeInfo* typeInfo, ComponentConstructorBase* ctor);

		template<typename T>
		void RegisterComponent(const std::string& category = "")
		{
			static_assert(std::is_base_of<Component, T>::value, "T must derive directly or indirectly from Component");
			RegisterComponent(category, T::GetTypeInfoStatic(), new ComponentConstructor<T>);
		}

		Component* CreateComponent(string_hash_t id, GameObject* owner);

		template<typename T>
		T* CreateComponent(string_hash_t id)
		{
			return CreateComponent(id)->As<T>();
		}

		const List& GetComponents(const std::string& category = "");
	};
}

#endif /* C3D__COMPONENTFACTORY_H */
