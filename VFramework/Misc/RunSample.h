#pragma once

#include <string.h> 
#include <functional>

/*
 * Primitive setup to be able to run arbitrary test code associated with string key 
 */

namespace vex
{
	struct SampleEntry
	{
		const char* Key = nullptr;
		std::function<bool()> Run;
	};

	// to easily run specific test sample from main/console if needed.
	struct SampleRunner
	{
	public:
		static SampleRunner& Global();

		void Register(const char* key, std::function<bool()>&& func);

		void RunSample(const char* key);

	private:
		static const int _maxSamples = 64;
		SampleEntry _entries[_maxSamples];
		int _count = 0;
	};
} // namespace vp

namespace Vex_Private
{
	struct RunCmd_Anon
	{
		template <typename Callable>
		RunCmd_Anon(const char* sampleName, Callable&& callable)
		{
			vex::SampleRunner::Global().Register(sampleName, callable);
		}
	};
}; // namespace Vex_Private

#define VexPrivate_DECLARE_TESTSAMPLE(cmdKey, code)                                     \
	namespace Vex_Private                                                               \
	{                                                                                   \
		struct RunCmd_##cmdKey                                                          \
		{                                                                               \
			RunCmd_##cmdKey()                                                           \
			{                                                                           \
				vex::SampleRunner::Global().Register(#cmdKey, []() -> bool { (code); }); \
			}                                                                           \
		};                                                                              \
		static inline auto RegisterRunCmd_##cmdKey = RunCmd_##cmdKey{};                 \
	};

#define VEX_TESTSAMPLE(cmdKey, code) VexPrivate_DECLARE_TESTSAMPLE(cmdKey, code);
#define VEX_TESTSAMPLE_INLINE static inline Vex_Private::RunCmd_Anon vpint_COMBINE(sample_anon_, __LINE__) =
