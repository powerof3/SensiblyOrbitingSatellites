#include "Hooks.h"
#include "Settings.h"

namespace Hooks::Rotation
{
	struct Moon_AttachRoot
	{
		static void thunk(RE::Moon* a_moon, RE::NiNode* a_root)
		{
			const auto settings = Settings::GetSingleton();

			if (a_root) {
				auto rotation = settings->rotation;
				a_root->local.rotate.SetEulerAnglesXYZ(RE::deg_to_rad(rotation.x), RE::deg_to_rad(rotation.y), RE::deg_to_rad(rotation.z));

				if (settings->dumpStats) {
					logger::info("moon root rotation : {},{},{}", rotation.x, rotation.y, rotation.z);
				}
			}

			func(a_moon, a_root);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	void Install()
	{
		REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(25625, 26168), 0xD4 };  //Moon::RecreateGeometry
		stl::write_thunk_call<Moon_AttachRoot>(target.address());

		logger::info("installing rotation");
	}
}

namespace Hooks::Phases
{
	struct Moon_UpdatePhase
	{
		static bool func(RE::Sky* a_sky)
		{
			using PhaseLength = RE::TESClimate::Timing::MoonPhaseLength;

			const auto climate = a_sky->currentClimate;
			if (!climate) {
				return false;
			}

			auto phaseLength = stl::to_underlying(climate->timing.moonPhaseLength.get());
			if ((phaseLength & 0x3F) == 0) {
				return false;
			}

			const auto settings = Settings::GetSingleton();

			auto daysPassed = static_cast<std::uint32_t>(RE::Calendar::GetSingleton()->GetDaysPassed());
			std::uint32_t masserPhase = daysPassed % (settings->masserPhases.size() * (phaseLength & 0x3F)) / (phaseLength & 0x3F);
			std::uint32_t secundaPhase = daysPassed % (settings->secundaPhases.size() * (phaseLength & 0x3F)) / (phaseLength & 0x3F);

			if (masserPhase == settings->curMasserPhase && secundaPhase == settings->curSecundaPhase) {
				return false;
			}

			if (settings->dumpStats) {
				logger::info("days passed {} | masser {} | secunda {}", daysPassed, masserPhase, secundaPhase);
			}

			settings->curMasserPhase = masserPhase;
			settings->curSecundaPhase = secundaPhase;

			return true;
		}
#ifdef SKYRIM_AE
		static inline constexpr std::size_t size{ 0x95 };
#else
		static inline constexpr std::size_t size{ 0x5F };
#endif
	};

	void Install()
	{
		REL::Relocation<std::uintptr_t> func{ RELOCATION_ID(25620, 26158) };
		stl::asm_replace<Moon_UpdatePhase>(func.address());

		logger::info("installing update phase");
	}
}

namespace Hooks::Texture
{
	struct detail
	{
		static const std::string& get_moon_phase(const char* a_path)
		{
			const auto settings = Settings::GetSingleton();
			if (string::icontains(a_path, "Masser")) {
				return settings->masserPhases[settings->curMasserPhase];
			}
			return settings->secundaPhases[settings->curSecundaPhase];
		}
	};

	struct BSTextureDB_Demand
	{
		static void thunk(const char* a_path, RE::NiPointer<RE::NiSourceTexture>& a_texture, std::uint8_t a_arg3, bool a_arg4, std::int32_t a_arg5, bool a_arg6, bool a_arg7)
		{
			return func(detail::get_moon_phase(a_path).c_str(), a_texture, a_arg3, a_arg4, a_arg5, a_arg6, a_arg7);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct BSShaderManager_GetTexture
	{
		static void thunk(const char* a_path, bool a_arg1, RE::NiPointer<RE::NiSourceTexture>& a_texture, bool a_arg3)
		{
			return func(detail::get_moon_phase(a_path).c_str(), a_arg1, a_texture, a_arg3);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	void Install()
	{
		REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(25626, 26169) };  //Moon::Update

		stl::write_thunk_call<BSTextureDB_Demand>(target.address() + OFFSET(0x1D4, 0x2C5));
		stl::write_thunk_call<BSShaderManager_GetTexture>(target.address() + OFFSET(0x1FE, 0x2EF));

		logger::info("installing texture hook");
	}
}

namespace Hooks::Position
{
	float set_moon_angle(RE::Moon* a_moon)
	{
		float daySpeed = a_moon->speed * 4;

		const auto gameDaysPassed = RE::Calendar::GetSingleton()->GetDaysPassed();
		const auto angle = (gameDaysPassed * daySpeed - std::floorf(gameDaysPassed * daySpeed)) * 360.0f;

		a_moon->unkCC = angle;

		return angle;
	}

	void Install()
	{
		REL::Relocation<std::uintptr_t> target{ RELOCATION_ID(25626, 26169), OFFSET(0x121, 0x146) };  //Moon::Update

		struct Patch : Xbyak::CodeGenerator
		{
			Patch(std::uintptr_t a_func, std::uintptr_t a_target)
			{
				Xbyak::Label retnLabel;
				Xbyak::Label funcLabel;

				mov(rcx, rdi);  //move Moon ptr

				sub(rsp, 0x20);
				call(ptr[rip + funcLabel]);  // call our function
				add(rsp, 0x20);

#ifdef SKYRIM_AE
				movaps(xmm1, xmm0);  // move new angle into old
#endif

				// orig code
				mov(eax, dword[rsi + 0x1B0]);
				jmp(ptr[rip + retnLabel]);

				L(funcLabel);
				dq(a_func);

				L(retnLabel);
				dq(a_target + 0x6);
			}
		};

		Patch patch(reinterpret_cast<uintptr_t>(set_moon_angle), target.address());
		patch.ready();

		auto& trampoline = SKSE::GetTrampoline();
		SKSE::AllocTrampoline(56);

		trampoline.write_branch<6>(target.address(), trampoline.allocate(patch));

		logger::info("hooked position");
	}
}

namespace Hooks
{
	void Install()
	{
		const auto settings = Settings::GetSingleton();

		logger::info("installing hooks");

		Rotation::Install();

		if (!settings->masserPhases.empty() && !settings->secundaPhases.empty()) {
			Phases::Install();
			Texture::Install();
		}

		if (settings->hookPosition) {
			Position::Install();
		}
	}
}
