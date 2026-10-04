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
		ga::core::Property<float>     StretchFactor;
		ga::core::Property<float>     Fov;
		ga::core::Property<glm::vec2> ZBounds;
		ga::core::Property<uint32_t>  RendererIndex;

		explicit Camera(GameObject* owner);
		virtual ~Camera() noexcept;

		void             UpdateMatrix();
		const glm::mat4& ProjectionMatrix();
		std::pair<glm::vec3 /* ro */, glm::vec3 /* rd */> Deproject(const glm::vec2& screenCoords); // assumes screenCoords is in PIXELS

		void Update(float t, float dt, c3d::GameSubsystems& subsystems) override;

		void Serialize(ga::core::CttiSerializer& serializer) const override;
		void Deserialize(ga::core::CttiDeserializer& deserializer) override;

		void OnWindowResize(ga::core::MessageDataBase* msg);
		void RenderUi() override;

		void RenderGizmo() override;
	};
}

#endif /* C3D__COMPONENTS__RENDERING__CAMERA_H */
