#include "Settings.h"

Settings::Settings()
{
	const auto path = fmt::format("Data/SKSE/Plugins/{}.ini", Version::PROJECT);

	CSimpleIniA ini;
	ini.SetUnicode();
	ini.SetMultiKey();

	ini.LoadFile(path.c_str());

	detail::get_value(ini, dumpStats, "Settings", "Print Debug Info");

	detail::get_value(ini, hookPosition, "Settings", "Update Angle Per Frame");

	detail::get_value(ini, rotation.x, "Rotation", "X");
	detail::get_value(ini, rotation.y, "Rotation", "Y");
	detail::get_value(ini, rotation.z, "Rotation", "Z");

	detail::get_moon_phases(ini, masserPhases, "Masser Phases");
	detail::get_moon_phases(ini, secundaPhases, "Secunda Phases");
	
	detail::get_value(ini, masserPhaseLength, "Phase Length in Days", "Masser");
	detail::get_value(ini, secundaPhaseLength, "Phase Length in Days", "Secunda");

	(void)ini.SaveFile(path.c_str());
}
