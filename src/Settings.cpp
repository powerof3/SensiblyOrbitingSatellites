#include "Settings.h"

Settings::Settings()
{
	const auto path = fmt::format("Data/SKSE/Plugins/{}.ini", Version::PROJECT);

	CSimpleIniA ini;
	ini.SetUnicode();

	ini.LoadFile(path.c_str());

	detail::get_value(ini, rotation.x, "Rotation", "X", ";Moon parent node rotation. Default rotation is from north to south (0,0,0)");
	detail::get_value(ini, rotation.y, "Rotation", "Y", nullptr);
	detail::get_value(ini, rotation.z, "Rotation", "Z", nullptr);

	detail::get_value(ini, dumpStats, "Settings", "Print Debug Info", nullptr);

	detail::get_moon_phases(ini, masserPhases, "Masser Phases");
	detail::get_moon_phases(ini, secundaPhases, "Secunda Phases");

	(void)ini.SaveFile(path.c_str());
}
