#ifndef C3D__TRANSFORMDATA_H
#define C3D__TRANSFORMDATA_H
#pragma once

#include <memory>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include <gallium/badge.h>
#include <gallium/core/ctti.h>
#include <gallium/core/uiobject.h>
#include <gallium/core/property.h>

namespace c3d
{
	class GameObject;

	class TransformData final
		: public ga::core::CttiObject
		, public ga::core::UiObject
	{
		GA_CTTI_OBJECT(c3d::TransformData, ga::core::CttiObject);

		struct Internal;
		std::unique_ptr<Internal> m_pImpl;

	public:
		ga::core::Property<glm::vec3>      Position;
		ga::core::Property<glm::quat>      Orientation;
		ga::core::Property<glm::vec3>      Scale;
		ga::core::Property<TransformData*> Parent;

		TransformData();
		TransformData(const ga::Badge<GameObject>&, GameObject* object);
		TransformData(const TransformData& other);
		TransformData(ga::core::CttiDeserializer& deserializer);
		~TransformData() noexcept;

		TransformData& operator=(const TransformData& other);

		GameObject* GetGameObject();

		bool HasChanged() const;
		bool IsScaleUniform() const;

		const bool       IsDirty() const;
		void             UpdateMatrices();
		const glm::mat4& LocalMatrix();
		const glm::mat4& GlobalMatrix();

		void Rotate(const glm::quat& rotation);
		void Rotate(const glm::vec3& axis, float angle);
		void LookAt(const TransformData& target, const glm::vec3& up);
		void LookAt(const glm::vec3& target, const glm::vec3& up);

		glm::vec3 TransformPoint(const glm::vec3& p) const;
		glm::vec3 TransformVector(const glm::vec3& v) const;
		glm::vec3 DetransformPoint(const glm::vec3& p) const;
		glm::vec3 DetransformVector(const glm::vec3& v) const;

		void Serialize(ga::core::CttiSerializer& serializer) const override;
		void Deserialize(ga::core::CttiDeserializer& deserializer) override;
		void ResolvePointers(ga::core::CttiDeserializer& deserializer) override;

		void RenderUi() override;
	};
}

#endif /* C3D__TRANSFORMDATA_H */
