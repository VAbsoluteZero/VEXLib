#include "RunSample.h"

#include "VFramework/VEXBase.h"


vex::SampleRunner& vex::SampleRunner::global()
{
	static SampleRunner Self;
	return Self;
}

void vex::SampleRunner::registerSample(const char* key, std::function<bool()>&& func)
{
	if (count >= max_samples)
		return;
	entries[count++] = SampleEntry{key, func};
}

void vex::SampleRunner::runSamples(const char* key)
{
	for (int i = 0; i < count; ++i)
	{
		if ((nullptr != entries[i].runnable) && !::strcmp(key, entries[i].key))
		{
			spdlog::info(" ==> SampleRunner :: running {}   ", key);

			bool bResult = entries[i].runnable();

			if (!bResult)
			{
				spdlog::info(" ==> SampleRunner :: done {}, FAILURE !    ", key);
			}
			else
			{
				spdlog::info(" ==> SampleRunner :: done {}, success   ", key);
			}

			return;
		}
	}
}
