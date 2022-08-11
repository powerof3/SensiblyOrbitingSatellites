#pragma once

class Settings
{
public:
	[[nodiscard]] static Settings* GetSingleton()
	{
		static Settings singleton;
		return std::addressof(singleton);
	}

	RE::NiPoint3 rotation{};

	std::vector<std::string> masserPhases{};
	std::vector<std::string> secundaPhases{};

	std::uint32_t curMasserPhase{ 0 };
	std::uint32_t curSecundaPhase{ 0 };

	bool dumpStats{ false };

private:
	struct detail
	{
		template <class T>
		static void get_value(CSimpleIniA& a_ini, T& a_value, const char* a_section, const char* a_key, const char* a_comment)
		{
			if constexpr (std::is_same_v<T, bool>) {
				a_value = a_ini.GetBoolValue(a_section, a_key, a_value);
				a_ini.SetBoolValue(a_section, a_key, a_value, a_comment);
			} else if constexpr (std::is_floating_point_v<T>) {
				a_value = static_cast<float>(a_ini.GetDoubleValue(a_section, a_key, a_value));
				a_ini.SetDoubleValue(a_section, a_key, a_value, a_comment);
			} else {
				a_value = a_ini.GetValue(a_section, a_key, a_value.c_str());
				a_ini.SetValue(a_section, a_key, a_value.c_str(), a_comment);
			}
		}

		static void get_moon_phases(const CSimpleIniA& a_ini, std::vector<std::string>& a_vec, const char* a_type)
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
	};

	Settings();
	Settings(const Settings&) = delete;
	Settings(Settings&&) = delete;

	~Settings() = default;

	Settings& operator=(const Settings&) = delete;
	Settings& operator=(Settings&&) = delete;
};
