#ifndef C3D__COMPONENTS__RENDERING__CAMERA_H
#define C3D__COMPONENTS__RENDERING__CAMERA_H
#pragma once

#include <gallium/core/property.h>
#include <gallium/core/messagebus.h>
#include <c3d/component.h>
#include <glm/glm.hpp>
#include <memory>

namespace c3d::components::rendering
{
	class Camera final
		: public c3d::Component
	{
		GA_CTTI_OBJECT(c3d::components::rendering::Camera, c3d::Component);

		struct Internal;
		std::unique_ptr<Internal> m_camImpl;

	public:
		ga::core::Property<float>     AspectRatio;
		ga::core::Property<float>     Fov;
		ga::core::Property<glm::vec2> ZBounds;

		explicit Camera(GameObject* owner);
		virtual ~Camera() noexcept;

		void             UpdateMatrix();
		const glm::mat4& ProjectionMatrix();

		void Serialize(ga::core::CttiSerializer& serializer) const override;
		void Deserialize(ga::core::CttiDeserializer& deserializer) override;

		void OnWindowResize(ga::core::MessageDataBase* msg);
		void RenderUi() override;

		void RenderGizmo() override;
	};
}

#endif /* C3D__COMPONENTS__RENDERING__CAMERA_H */
