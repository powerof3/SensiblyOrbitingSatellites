#include "version.h"

static float x = 0.0;
static float y = 0.0;
static float z = 0.0;


bool ReadINI()
{
	static std::string pluginPath;
	if (pluginPath.empty()) {
		pluginPath = SKSE::GetPluginConfigPath("po3_SensiblyOrbitingSatellites");
	}

	CSimpleIniA ini;
	SI_Error rc = ini.LoadFile(pluginPath.c_str());

	if (rc < 0) {
		logger::error("Can't load 'po3_SensiblyOrbitingSatellites.ini'");
		return false;
	}

	ini.SetUnicode();

	x = static_cast<float>(ini.GetDoubleValue("Settings", "X", 0.0));
	y = static_cast<float>(ini.GetDoubleValue("Settings", "Y", 0.0));
	z = static_cast<float>(ini.GetDoubleValue("Settings", "Z", 0.0));

	return true;
}


class Moon
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
			rot.SetEulerAnglesXYZ(RE::degToRad(x), RE::degToRad(y), RE::degToRad(z));
			logger::info("moon root rotation : {} | {} | {}", x, y, z);
		}
		return _AttachShape(a_moon, a_root);
	}
	static inline REL::Relocation<decltype(AttachShape)> _AttachShape;
};


void OnInit(SKSE::MessagingInterface::Message* a_msg)
{
	switch (a_msg->type) {
	case SKSE::MessagingInterface::kDataLoaded:
		{
			Moon::Patch();
			logger::info("patched moon root");
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
		SKSE::AllocTrampoline(1 << 4);

		ReadINI();

		auto messaging = SKSE::GetMessagingInterface();
		if (!messaging->RegisterListener("SKSE", OnInit)) {
			logger::critical("Failed to register messaging listener!");
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
