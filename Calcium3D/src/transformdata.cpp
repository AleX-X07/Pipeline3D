#include <c3d/transformdata.h>

#include <c3d/gameobject.h>

#include <imgui/imgui.h>

using namespace c3d;

struct TransformData::Internal
{
	GameObject* object;

	bool        localMatrixDirty;
	glm::mat4   localMatrix;

	bool        globalMatrixDirty;
	glm::mat4   globalMatrix;
};

/**
 * Transform class somewhat inspired by https://github.com/ddiakopoulos/tinygizmo
 */

// Utility functions for dealing with quaternions
constexpr glm::vec3 qxdir(const glm::quat& q)                    { return { q.w * q.w + q.x * q.x - q.y * q.y - q.z * q.z, (q.x * q.y + q.z * q.w) * 2.0f, (q.z * q.x - q.y * q.w) * 2.0f }; }
constexpr glm::vec3 qydir(const glm::quat& q)                    { return { (q.x * q.y - q.z * q.w) * 2.0f, q.w * q.w - q.x * q.x + q.y * q.y - q.z * q.z, (q.y * q.z + q.x * q.w) * 2.0f }; }
constexpr glm::vec3 qzdir(const glm::quat& q)                    { return { (q.z * q.x + q.y * q.w) * 2.0f, (q.y * q.z - q.x * q.w) * 2.0f, q.w * q.w - q.x * q.x - q.y * q.y + q.z * q.z }; }
          glm::vec3 qrot(const glm::quat& q, const glm::vec3& v) { return qxdir(q) * v.x + qydir(q) * v.y + qzdir(q) * v.z; }
constexpr glm::quat qconj(const glm::quat&q)                     { return { -q.x, -q.y, -q.z, q.w}; }
          glm::quat qinv(const glm::quat& q)                     { return qconj(q) / glm::dot(q, q); }
constexpr glm::quat qmul(const glm::quat& a, const glm::quat& b) { return { a.x * b.w + a.w * b.x + a.y * b.z - a.z * b.y, a.y * b.w + a.w * b.y + a.z * b.x - a.x * b.z, a.z * b.w + a.w * b.z + a.x * b.y - a.y * b.x, a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z }; }


TransformData::TransformData()
	: m_pImpl(new Internal)
{
	m_pImpl->object = nullptr;

	Position = glm::vec3(0.0f);
	Position.OnChange([this]() { m_pImpl->localMatrixDirty = true; });

	Orientation = glm::quat(glm::vec3(0.0f));
	Orientation.OnChange([this]() { m_pImpl->localMatrixDirty = true; });

	Scale = glm::vec3(1.0f);
	Scale.OnChange([this]() { m_pImpl->localMatrixDirty = true; });

	Parent = nullptr;
	Parent.OnChange([this]() {
		m_pImpl->globalMatrixDirty = true;
	});
}

TransformData::TransformData(const ga::Badge<GameObject>&, GameObject* object)
	: TransformData()
{
	m_pImpl->object = object;
}

TransformData::TransformData(ga::core::CttiDeserializer& deserializer)
	: TransformData()
{
	Deserialize(deserializer);
}

TransformData::TransformData(const TransformData& other)
	: TransformData()
{
	operator=(other);
}

TransformData::~TransformData() noexcept
{
}

TransformData& TransformData::operator=(const TransformData& other)
{
	m_pImpl->object = other.m_pImpl->object;

	Position = other.Position.ConstRef();
	Orientation = other.Orientation.ConstRef();
	Scale = other.Scale.ConstRef();

	return *this;
}

bool TransformData::IsScaleUniform() const
{
	return Scale.ConstRef().x == Scale.ConstRef().y && Scale.ConstRef().x == Scale.ConstRef().z;
}

GameObject* TransformData::GetGameObject()
{
	return m_pImpl->object;
}

const bool TransformData::IsDirty() const
{
	return m_pImpl->localMatrixDirty || m_pImpl->globalMatrixDirty;
}

void TransformData::UpdateMatrices()
{
	if (m_pImpl->localMatrixDirty)
	{
		m_pImpl->localMatrix = glm::mat4(
			glm::vec4(qxdir(Orientation) * Scale->x, 0.0f),
			glm::vec4(qydir(Orientation) * Scale->y, 0.0f),
			glm::vec4(qzdir(Orientation) * Scale->z, 0.0f),
			glm::vec4(Position(), 1.0f)
		);

		m_pImpl->localMatrixDirty  = false;
		m_pImpl->globalMatrixDirty = true;
	}

	if (m_pImpl->globalMatrixDirty)
	{
		glm::mat4 upperMatrix(1.0f);
		if (Parent() != nullptr)
			upperMatrix = Parent.ConstRef()->GlobalMatrix();

		m_pImpl->globalMatrix      = m_pImpl->localMatrix * upperMatrix;
		m_pImpl->globalMatrixDirty = false;
	}
}

const glm::mat4& TransformData::LocalMatrix()
{
	UpdateMatrices();
	return m_pImpl->localMatrix;
}

const glm::mat4& TransformData::GlobalMatrix()
{
	UpdateMatrices();
	return m_pImpl->globalMatrix;
}

void TransformData::Rotate(const glm::quat& rotation)
{
	Orientation = qmul(rotation, Orientation);
}

void TransformData::Rotate(const glm::vec3& axis, float angle)
{
	Orientation = qmul(glm::angleAxis(angle, axis), Orientation);
}

void TransformData::LookAt(const TransformData& target, const glm::vec3& up)
{
	LookAt(target.Position.ConstRef(), up);
}

void TransformData::LookAt(const glm::vec3& target, const glm::vec3& up)
{
	glm::vec3 forward = glm::normalize(target - Position.ConstRef());
	Orientation = glm::quatLookAt(forward, up);
	m_pImpl->localMatrixDirty = true;
}

glm::vec3 TransformData::TransformPoint(const glm::vec3& p) const
{
	return Position + TransformVector(p);
}

glm::vec3 TransformData::TransformVector(const glm::vec3& v) const
{
	return qrot(Orientation(), v * Scale());
}

glm::vec3 TransformData::DetransformPoint(const glm::vec3& p) const
{
	return DetransformVector(p - Position());
}

glm::vec3 TransformData::DetransformVector(const glm::vec3& v) const
{
	return qrot(qinv(Orientation.ConstRef()), v) / Scale.ConstRef();
}

void TransformData::Serialize(ga::core::CttiSerializer& serializer) const
{
	serializer << m_pImpl->object;
	serializer << Position;
	serializer << Orientation;
	serializer << Scale;
}

void TransformData::Deserialize(ga::core::CttiDeserializer& deserializer)
{
	deserializer >> m_pImpl->object;
	deserializer >> Position;
	deserializer >> Orientation;
	deserializer >> Scale;
	m_pImpl->localMatrixDirty = true;
}

void TransformData::ResolvePointers(ga::core::CttiDeserializer& deserializer)
{
	m_pImpl->object = deserializer.ResolvePointer(m_pImpl->object);
}

void TransformData::RenderUi()
{
	if (ImGui::DragFloat3("Position", (float*)Position.operator->(), 1.0f, -100000.0f, 100000.0f, "%.2f", 1.0f))
		Position.TriggerChanged();
	
	glm::vec3 eulerAngles = glm::eulerAngles(Orientation.ConstRef());
	if (ImGui::DragFloat3("Orientation", (float*)(&eulerAngles), 1.0f, -100000.0f, 100000.0f, "%.2f", 1.0f))
		Orientation = glm::quat(eulerAngles);
	
	if (ImGui::DragFloat3("Scale", (float*)Scale.operator->(), 1.0f, -100000.0f, 100000.0f, "%.2f", 1.0f))
		Scale.TriggerChanged();
}
