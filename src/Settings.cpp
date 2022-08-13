#include "Settings.h"

void Settings::Moon::LoadData(std::string a_type, const CSimpleIniA& a_ini)
{
	type = std::move(a_type);

    detail::get_value(a_ini, speed, type.c_str(), "Speed");
	detail::get_value(a_ini, phaseLength, type.c_str(), "Phase Length in Days");

	CSimpleIniA::TNamesDepend values;
	a_ini.GetAllValues(type.c_str(), "Phase Path", values);
	values.sort(CSimpleIniA::Entry::LoadOrder());

	logger::info("{} found : {}", type, values.size());

	if (!values.empty()) {
		phases.reserve(values.size());
		for (auto& value : values) {
			logger::info("	{}", value.pItem);
			phases.emplace_back(value.pItem);
		}
	}
}

const std::string& Settings::Moon::GetPhase()
{
	return phases[currentPhase];
}

double Settings::Moon::GetSpeed() const
{
	return speed;
}

bool Settings::Moon::HasPhases() const
{
	return !phases.empty();
}

bool Settings::Moon::UpdatePhase(std::uint32_t a_daysPassed)
{
	std::uint32_t phase = a_daysPassed % (phases.size() * phaseLength) / phaseLength;

	if (phase == currentPhase) {
		return false;
	}

	currentPhase = phase;

	return true;
}

Settings::Moon* Settings::GetMoon(RE::Moon* a_moon)
{
	if (string::icontains(a_moon->stateTextures[RE::Moon::Phase::kFull], "Masser")) {
		return &masser;
	} else {
		return &secunda;
	}
}

Settings::Moon* Settings::GetMoon(const char* a_path)
{
	if (string::icontains(a_path, "Masser")) {
		return &masser;
	} else {
		return &secunda;
	}
}

Settings::Settings()
{
	const auto path = fmt::format("Data/SKSE/Plugins/{}.ini", Version::PROJECT);

	CSimpleIniA ini;
	ini.SetUnicode();
	ini.SetMultiKey();

	ini.LoadFile(path.c_str());

	masser.LoadData("Masser", ini);
	secunda.LoadData("Secunda", ini);

	(void)ini.SaveFile(path.c_str());
}
