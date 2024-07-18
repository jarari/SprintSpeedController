#include <detourxs/detourxs.h>

REL::Relocation<uintptr_t> ptr_SetMoveModeBits(REL::ID(1074248));
DetourXS setMoveModeHook;
uintptr_t SetMoveModeOrig;
RE::ActorValueInfo* speedMultAV;
RE::BSFixedString fLocomotionSprintPlaybackSpeed;

bool HookedSetMoveMode(RE::ActorState* as, uint16_t bits)
{
	if (bits & 0x100) {  //Sprint
		RE::Actor* a = reinterpret_cast<RE::Actor*>((uintptr_t)as - 0x128);
		float speedmult = a->GetActorValue(*speedMultAV);
		if (speedmult <= 0) {
			a->SetGraphVariableFloat(fLocomotionSprintPlaybackSpeed, 1.f);
		} else {
			a->SetGraphVariableFloat(fLocomotionSprintPlaybackSpeed, speedmult / 100.f);
		}
	}
	using func_t = decltype(&HookedSetMoveMode);
	return ((func_t)SetMoveModeOrig)(as, bits);
}

void InitializePlugin()
{
	speedMultAV = reinterpret_cast<RE::ActorValueInfo*>(RE::TESForm::GetFormByID(0x2DA));
	fLocomotionSprintPlaybackSpeed = RE::BSFixedString("fLocomotionSprintPlaybackSpeed");
	bool succ = setMoveModeHook.Create((LPVOID)(ptr_SetMoveModeBits.address()), HookedSetMoveMode);
	if (succ) {
		SetMoveModeOrig = (uintptr_t)setMoveModeHook.GetTrampoline();
	}
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Query(const F4SE::QueryInterface* a_f4se, F4SE::PluginInfo* a_info)
{
#ifndef NDEBUG
	auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#else
	auto path = logger::log_directory();
	if (!path) {
		return false;
	}

	*path /= fmt::format(FMT_STRING("{}.log"), Version::PROJECT);
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
#endif

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

#ifndef NDEBUG
	log->set_level(spdlog::level::trace);
#else
	log->set_level(spdlog::level::info);
	log->flush_on(spdlog::level::warn);
#endif

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("%g(%#): [%^%l%$] %v"s);

	logger::info(FMT_STRING("{} v{}"), Version::PROJECT, Version::NAME);

	a_info->infoVersion = F4SE::PluginInfo::kVersion;
	a_info->name = Version::PROJECT.data();
	a_info->version = Version::MAJOR;

	if (a_f4se->IsEditor()) {
		logger::critical("loaded in editor"sv);
		return false;
	}

	const auto ver = a_f4se->RuntimeVersion();
	if (ver < F4SE::RUNTIME_1_10_162) {
		logger::critical(FMT_STRING("unsupported runtime v{}"), ver.string());
		return false;
	}

	return true;
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Load(const F4SE::LoadInterface* a_f4se)
{
	F4SE::Init(a_f4se);

	const F4SE::MessagingInterface* message = F4SE::GetMessagingInterface();
	message->RegisterListener([](F4SE::MessagingInterface::Message* msg) -> void {
		if (msg->type == F4SE::MessagingInterface::kGameDataReady) {
			InitializePlugin();
		}
	});

	return true;
}
