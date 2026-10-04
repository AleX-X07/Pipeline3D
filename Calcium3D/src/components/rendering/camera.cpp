#include <c3d/components/rendering/camera.h>

#include <gallium/platform/platform.h>

#include <c3d/gamescene.h>
#include <c3d/gameobject.h>
#include <c3d/transformdata.h>

#include <glm/gtc/matrix_transform.hpp>

#include <imgui/imgui.h>

using namespace c3d::components::rendering;

struct Camera::Internal
{
	bool      projectionMatrixDirty;
	glm::mat4 projectionMatrix;

	ga::core::MessageHandler* resizeHandler         = nullptr;
	glm::uvec2                lastKnownWindowSize   = glm::uvec2(1600, 900);
	bool                      autoAdjustAspectRatio = true;
};

Camera::Camera(GameObject* owner)
	: Component(owner)
	, m_camImpl(new Internal)
{
	Fov = 60.0f;
	AspectRatio = 1.7f;
	ZBounds = glm::vec2(0.1f, 100.0f);

	m_camImpl->resizeHandler = new ga::core::ObjectMemberDelegate(this, &Camera::OnWindowResize);
	ga::core::MessageBus::Global().AddRecipient("window:resize"_h, m_camImpl->resizeHandler);
}

Camera::~Camera() noexcept
{
	ga::core::MessageBus::Global().RemoveRecipient("window:resize"_h, m_camImpl->resizeHandler);
	delete m_camImpl->resizeHandler;
}

void Camera::UpdateMatrix()
{
	if (m_camImpl->projectionMatrixDirty)
	{
		m_camImpl->projectionMatrix = glm::perspective(glm::radians(Fov()), AspectRatio(), ZBounds->x, ZBounds->y);
		m_camImpl->projectionMatrixDirty = false;
	}
}

const glm::mat4& Camera::ProjectionMatrix()
{
	UpdateMatrix();
	return m_camImpl->projectionMatrix;
}

std::pair<glm::vec3, glm::vec3> Camera::Deproject(const glm::vec2& screenCoords)
{
	const glm::vec2 size = glm::vec2(m_camImpl->lastKnownWindowSize);
	const float     ndcX = (screenCoords.x / size.x) * 2.0f - 1.0f;
	const float     ndcY = -(screenCoords.y / size.y) * 2.0f + 1.0f;

	const glm::mat4 view  = glm::inverse(GetGameObject()->Transform().GlobalMatrix());
	const glm::mat4 invVP = glm::inverse(ProjectionMatrix() * view);

	glm::vec4 nearPt = invVP * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
	glm::vec4 farPt = invVP * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);

	nearPt /= nearPt.w;
	farPt /= farPt.w;

	const glm::vec3 ro = glm::vec3(nearPt);
	const glm::vec3 rd = glm::normalize(glm::vec3(farPt) - glm::vec3(nearPt));

	return { ro, rd };
}

void Camera::Serialize(ga::core::CttiSerializer& serializer) const
{
	Component::Serialize(serializer);

	Fov.Serialize(serializer);
	serializer << m_camImpl->autoAdjustAspectRatio;
	AspectRatio.Serialize(serializer);
	ZBounds.Serialize(serializer);
}

void Camera::Deserialize(ga::core::CttiDeserializer& deserializer)
{
	Component::Deserialize(deserializer);

	Fov.Deserialize(deserializer);
	deserializer >> m_camImpl->autoAdjustAspectRatio;
	AspectRatio.Deserialize(deserializer);
	ZBounds.Deserialize(deserializer);

	m_camImpl->projectionMatrixDirty = true;
}

void Camera::OnWindowResize(ga::core::MessageDataBase* msg)
{
	ga::platform::SurfaceResizeData* resize = reinterpret_cast<ga::platform::SurfaceResizeData*>(msg);

	if (m_camImpl->autoAdjustAspectRatio)
		AspectRatio = float(resize->data.surfaceSize.x) / float(resize->data.surfaceSize.y);

	m_camImpl->lastKnownWindowSize = resize->data.surfaceSize;
}

void Camera::RenderUi()
{
	Component::RenderUi();

	if (ImGui::DragFloat("Field of view", &Fov.NonConstRef_Unguarded(), 0.25f, 15.0f, 160.0f, "%.2f"))
		Fov.TriggerChanged();
	
	if (ImGui::Checkbox("Auto-adjust aspect ratio", &m_camImpl->autoAdjustAspectRatio))
	{
		if (m_camImpl->autoAdjustAspectRatio)
			AspectRatio = float(m_camImpl->lastKnownWindowSize.x) / float(m_camImpl->lastKnownWindowSize.y);
	}
	
	if (m_camImpl->autoAdjustAspectRatio)
		ImGui::BeginDisabled();
	
	if (ImGui::DragFloat("Aspect ratio", &AspectRatio.NonConstRef_Unguarded(), 0.1f, 0.5f, 5.0f, "%.3f"))
		AspectRatio.TriggerChanged();
	
	if (m_camImpl->autoAdjustAspectRatio)
		ImGui::EndDisabled();
	
	if (ImGui::DragFloat2("Z bounds", (float*)(&ZBounds.NonConstRef_Unguarded()), 0.1f, 0.01f, 10000.0f, "%.3f"))
		ZBounds.TriggerChanged();
}

void Camera::RenderGizmo()
{
	glm::mat4 model    = GetGameObject()->Transform.NonConstRef_Unguarded().GlobalMatrix();
	glm::mat4 view     = glm::inverse(GetGameObject()->Transform().GlobalMatrix());
	glm::mat4 proj     = ProjectionMatrix();
	
	glm::vec4 originCS = proj * view * model * glm::vec4(0.0f, 0.0f, 1000.0f, 1.0f);
	
	glm::vec3 originNDC(
		originCS.x / originCS.w,
		originCS.y / originCS.w,
		originCS.z / originCS.w
	);
	
	glm::vec2 originWS = glm::vec2(
		(originNDC.x * 0.5f + 0.5f) * float(m_camImpl->lastKnownWindowSize.x),
		(-originNDC.x * 0.5f + 0.5f) * float(m_camImpl->lastKnownWindowSize.y)
	);
	
	ImDrawList* dl = ImGui::GetBackgroundDrawList();
	
	dl->AddRectFilled(ImVec2(originWS.x - 8.0f, originWS.y - 8.0f), ImVec2(originWS.x + 8.0f, originWS.y + 8.0f), 0xFFFFFFFF, 0.0f);
}
