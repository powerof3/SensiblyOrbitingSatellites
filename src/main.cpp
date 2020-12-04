#include "version.h"

static float rX = 0.0;
static float rY = 0.0;
static float rZ = 0.0;

static std::vector<std::string> masserPhases;
static std::vector<std::string> secundaPhases;

static std::uint32_t curMasserPhase = 0;
static std::uint32_t curSecundaPhase = 0;

static bool dumpStats = false;


void GetMoonPhases(const CSimpleIniA& a_ini, std::vector<std::string>& a_vec, const char* a_type)
{
	CSimpleIniA::TNamesDepend values;
	a_ini.GetAllValues(a_type, "Path", values);
	values.sort(CSimpleIniA::Entry::LoadOrder());

	logger::info("{} found : {}", a_type, values.size());

	if (!values.empty()) {
		a_vec.reserve(values.size());
		for (auto& value : values) {
			logger::info("	{}", value.pItem);
			a_vec.emplace_back(value.pItem);
		}
	}
}


bool ReadINI()
{
	static std::string pluginPath;
	if (pluginPath.empty()) {
		pluginPath = SKSE::GetPluginConfigPath("po3_SensiblyOrbitingSatellites");
	}

	CSimpleIniA ini;
	ini.SetUnicode();
	ini.SetMultiKey();

	SI_Error rc = ini.LoadFile(pluginPath.c_str());
	if (rc < 0) {
		logger::error("Can't load 'po3_SensiblyOrbitingSatellites.ini'");
		return false;
	}

	rX = static_cast<float>(ini.GetDoubleValue("Rotation", "X", 0.0));
	rY = static_cast<float>(ini.GetDoubleValue("Rotation", "Y", 0.0));
	rZ = static_cast<float>(ini.GetDoubleValue("Rotation", "Z", 0.0));

	dumpStats = ini.GetBoolValue("Settings", "Debug Info", false);

	GetMoonPhases(ini, masserPhases, "Masser Phases");
	GetMoonPhases(ini, secundaPhases, "Secunda Phases");

	return true;
}


class MoonInit
{
public:
	static void Patch()
	{
		REL::Relocation<std::uintptr_t> moonInit{ REL::ID(25625) };

		auto& trampoline = SKSE::GetTrampoline();
		_AttachShape = trampoline.write_call<5>(moonInit.address() + 0xD4, AttachShape);
	}

private:
	static void AttachShape(RE::Moon* a_moon, RE::NiNode* a_root)
	{
		if (a_root) {
			auto& rot = a_root->local.rotate;
			rot.SetEulerAnglesXYZ(RE::degToRad(rX), RE::degToRad(rY), RE::degToRad(rZ));
			if (dumpStats) {
				logger::info("moon root rotation : {} | {} | {}", rX, rY, rZ);
			}
		}
		
		_AttachShape(a_moon, a_root);		
	}
	static inline REL::Relocation<decltype(AttachShape)> _AttachShape;
};


class SkyUpdate
{
public:
	static void Patch()
	{
		REL::Relocation<std::uintptr_t> skyUpdate{ REL::ID(25682) };

		auto& trampoline = SKSE::GetTrampoline();
		_GetMoonPhase = trampoline.write_call<5>(skyUpdate.address() + 0x395, GetMoonPhase);
	}

private:
	static bool GetMoonPhase(RE::Sky* a_sky)
	{
		using PhaseLength = RE::TESClimate::Timing::MoonPhaseLength;

		auto climate = a_sky->currentClimate;
		if (!climate) {
			return false;
		}

		auto phaseLength = to_underlying(*climate->timing.moonPhaseLength);
		if (!(phaseLength & 63)) {
			return false;
		}

		auto daysPassed = static_cast<std::uint32_t>(RE::Calendar::GetSingleton()->GetDaysPassed());
		std::uint32_t masserPhase = daysPassed % (masserPhases.size() * (phaseLength & 63)) / (phaseLength & 63);
		std::uint32_t secundaPhase = daysPassed % (secundaPhases.size() * (phaseLength & 63)) / (phaseLength & 63);

		if (masserPhase == curMasserPhase && secundaPhase == curSecundaPhase) {
			return false;
		}

		if (dumpStats) {
			logger::info("days passed {} | masser {} | secunda {}", daysPassed, masserPhase, secundaPhase);
		}

		curMasserPhase = masserPhase;
		curSecundaPhase = secundaPhase;

		return true;
	}
	static inline REL::Relocation<decltype(GetMoonPhase)> _GetMoonPhase;
};


class MoonUpdate
{
public:
	static void PatchLoadTexture()
	{
		REL::Relocation<std::uintptr_t> moonUpdate{ REL::ID(25626) };

		auto& trampoline = SKSE::GetTrampoline();
		_LoadTexture = trampoline.write_call<5>(moonUpdate.address() + 0x1FE, LoadTexture);
	}


	static void PatchUnkTextureFunc()
	{
		REL::Relocation<std::uintptr_t> moonUpdate{ REL::ID(25626) };

		auto& trampoline = SKSE::GetTrampoline();
		_UnkTextureFunc = trampoline.write_call<5>(moonUpdate.address() + 0x1D4, UnkTextureFunc);
	}


	static void Patch()
	{
		PatchLoadTexture();
		PatchUnkTextureFunc();
	}

private:
	static std::string GetMoonPhasePath(const char* a_path)
	{
		std::string path = a_path;
		if (path.find("Masser") != std::string::npos) {
			path = masserPhases[curMasserPhase];
		} else {
			path = secundaPhases[curSecundaPhase];
		}
		return path;
	}

	static void UnkTextureFunc(const char* a_path, RE::NiPointer<RE::NiSourceTexture>& a_texture, std::uint8_t a_unk3, bool a_unk4, std::int32_t a_unk5, bool a_unk6, bool a_unk7)
	{
		return _UnkTextureFunc(GetMoonPhasePath(a_path).c_str(), a_texture, a_unk3, a_unk4, a_unk5, a_unk6, a_unk7);
	}
	static inline REL::Relocation<decltype(UnkTextureFunc)> _UnkTextureFunc;


	static void LoadTexture(const char* a_path, std::uint8_t a_unk1, RE::NiPointer<RE::NiSourceTexture>& a_texture, bool a_unk2)
	{
		return _LoadTexture(GetMoonPhasePath(a_path).c_str(), a_unk1, a_texture, a_unk2);
	}
	static inline REL::Relocation<decltype(LoadTexture)> _LoadTexture;
};


void OnInit(SKSE::MessagingInterface::Message* a_msg)
{
	switch (a_msg->type) {
	case SKSE::MessagingInterface::kDataLoaded:
		{
			MoonInit::Patch();
			logger::info("patched moon root");

			if (!masserPhases.empty() && !secundaPhases.empty()) {
				SkyUpdate::Patch();
				MoonUpdate::Patch();
				logger::info("patched moon phases");
			}
		}
		break;
	}
}


extern "C" DLLEXPORT bool APIENTRY SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
	try {
		auto path = logger::log_directory().value() / "po3_SensiblyOrbitingSatellites.log";
		auto log = spdlog::basic_logger_mt("global log", path.string(), true);
		log->flush_on(spdlog::level::info);

#ifndef NDEBUG
		log->set_level(spdlog::level::debug);
		log->sinks().push_back(std::make_shared<spdlog::sinks::msvc_sink_mt>());
#else
		log->set_level(spdlog::level::info);

#endif
		spdlog::set_default_logger(log);
		spdlog::set_pattern("[%H:%M:%S] [%l] %v");

		logger::info("Sensibly Orbiting Satellites {}", SOS_VERSION_VERSTRING);

		a_info->infoVersion = SKSE::PluginInfo::kVersion;
		a_info->name = "Sensibly Orbiting Satellites";
		a_info->version = SOS_VERSION_MAJOR;

		if (a_skse->IsEditor()) {
			logger::critical("Loaded in editor, marking as incompatible");
			return false;
		}

		const auto ver = a_skse->RuntimeVersion();
		if (ver < SKSE::RUNTIME_1_5_39) {
			logger::critical("Unsupported runtime version {}", ver.string());
			return false;
		}
	} catch (const std::exception& e) {
		logger::critical(e.what());
		return false;
	} catch (...) {
		logger::critical("caught unknown exception");
		return false;
	}

	return true;
}


extern "C" DLLEXPORT bool APIENTRY SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	try {
		logger::info("Sensibly Orbiting Satellites loaded");

		SKSE::Init(a_skse);
		SKSE::AllocTrampoline(1 << 7);

		ReadINI();

		auto messaging = SKSE::GetMessagingInterface();
		if (!messaging->RegisterListener("SKSE", OnInit)) {
			return false;
		}
	} catch (const std::exception& e) {
		logger::critical(e.what());
		return false;
	} catch (...) {
		logger::critical("caught unknown exception");
		return false;
	}

	return true;
}
