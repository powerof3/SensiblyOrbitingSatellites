#pragma once

class Settings
{
public:
	struct Moon
	{
		void LoadData(std::string a_type, const CSimpleIniA& a_ini);

		const std::string& GetPhase();
		double GetSpeed() const;
		double GetOffset() const;
		bool HasPhases() const;

		bool UpdatePhase(std::uint32_t a_daysPassed);

		std::string type;

	private:
		double speed{ 0.0 };
		double offset{ 0.0 };

		std::vector<std::string> phases{};
		std::uint32_t phaseLength{ 3 };
		std::uint32_t currentPhase{ 0 };
	};

	[[nodiscard]] static Settings* GetSingleton()
	{
		static Settings singleton;
		return std::addressof(singleton);
	}

	Moon* GetMoon(RE::Moon* a_moon);
	Moon* GetMoon(const char* a_path);

	Moon masser;
	Moon secunda;

private:
	struct detail
	{
		template <class T>
		static void get_value(const CSimpleIniA& a_ini, T& a_value, const char* a_section, const char* a_key)
		{
			if constexpr (std::is_same_v<T, bool>) {
				a_value = a_ini.GetBoolValue(a_section, a_key, a_value);
			} else if constexpr (std::is_integral_v<T>) {
				a_value = a_ini.GetLongValue(a_section, a_key, a_value);
			} else if constexpr (std::is_same_v<T, double>) {
				a_value = a_ini.GetDoubleValue(a_section, a_key, a_value);
			} else if constexpr (std::is_floating_point_v<T>) {
				a_value = static_cast<float>(a_ini.GetDoubleValue(a_section, a_key, a_value));
			} else {
				a_value = a_ini.GetValue(a_section, a_key, a_value.c_str());
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
