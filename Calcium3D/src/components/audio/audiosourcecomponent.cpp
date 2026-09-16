#include <c3d/components/audio/audiosourcecomponent.h>

#include <gallium/audio/audiosystem.h>
#include <gallium/audio/audiosource.h>

#include <c3d/game.h>
#include <c3d/gameobject.h>
#include <c3d/transformdata.h>

#include <imgui/imgui.h>

using namespace c3d::components::audio;

AudioSource::AudioSource(c3d::GameObject* owner)
	: Component(owner)
	, Sound()
	, Volume(1.0f)
	, IsLooping(false)
	, IsSpatialized(false)
{
	m_lastKnownPosition = owner->Transform->Position;
}

AudioSource::AudioSource(ga::core::CttiDeserializer& deserializer)
	: Component(deserializer)
{
	Deserialize(deserializer);
}

AudioSource::~AudioSource() noexcept
{
}

void AudioSource::Update(float t, float dt, c3d::GameSubsystems& subsystems)
{
	if (Sound->IsValid())
	{
		bool newSource = false;

		if (!m_source.IsValid())
		{
			m_source = subsystems.audio.CreateSource({
				.sound = Sound,
				.bus   = subsystems.audio.GetMasterBus()
			});

			newSource = true;
		}

		if (!m_source.IsValid())
			return;

		if (newSource)
			subsystems.audio.Play(m_source);

		auto& source = subsystems.audio.GetSource(m_source);

		if (IsLooping.HasChanged())
			source.SetLoop(IsLooping);

		if (IsLooping && source.GetState() != ga::audio::ESourceState::Playing)
		{
			subsystems.audio.Seek(m_source, 0.0);
			subsystems.audio.Play(m_source);
		}

		if (IsSpatialized.HasChanged())
			if (!IsSpatialized)
				source.DisableSpatial();
		
		if (IsSpatialized)
		{
			source.SetPosition(GetGameObject()->Transform->Position);

			auto velocity = (GetGameObject()->Transform->Position - m_lastKnownPosition) * dt;
			source.SetVelocity(velocity);
			m_lastKnownPosition = GetGameObject()->Transform->Position;
		}

		if (Volume.HasChanged())
			source.SetVolume(Volume);
	}
}

void AudioSource::Serialize(ga::core::CttiSerializer& serializer) const
{
	Component::Serialize(serializer);
	serializer << Sound;
	serializer << Volume;
	serializer << IsSpatialized;
	serializer << IsLooping;
}

void AudioSource::Deserialize(ga::core::CttiDeserializer& deserializer)
{
	Component::Deserialize(deserializer);
	deserializer >> Sound;
	deserializer >> Volume;
	deserializer >> IsSpatialized;
	deserializer >> IsLooping;
}

void AudioSource::ResolvePointers(ga::core::CttiDeserializer& deserializer)
{
	Component::ResolvePointers(deserializer);
}

void AudioSource::RenderUi()
{
	// FIXME: Needs proper access to asset cache
	
	//if (ImGui::Button("X"))
	//	Source = nullptr;
	//ImGui::SameLine();
	//
	//ImGui::PushItemWidth(-97.0f);
	//
	//std::string targetName;
	//if (Source.ConstRef())
	//	targetName = Source.ConstRef()->Name.ConstRef();
	//ImGui::InputText("Target", &targetName, ImGuiInputTextFlags_ReadOnly);
	//
	//if (ImGui::BeginDragDropTarget())
	//{
	//	const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ga__AudioSource");
	//
	//	if (payload)
	//		Source = *reinterpret_cast<ga::audio::AudioSource**>(payload->Data);
	//
	//	ImGui::EndDragDropTarget();
	//}

	if (ImGui::DragFloat("Volume", &Volume.NonConstRef_Unguarded(), 0.1f, 0.0f, 1.0f))
		Volume.TriggerChanged();

	if (ImGui::Checkbox("Looping", &IsLooping.NonConstRef_Unguarded()))
		IsLooping.TriggerChanged();

	if (ImGui::Checkbox("Spatialized", &IsSpatialized.NonConstRef_Unguarded()))
		IsSpatialized.TriggerChanged();
}
