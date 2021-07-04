#include "RunSample.h"

#include "VLang/VEXBase.h"


vex::SampleRunner& vex::SampleRunner::Static()
{
	static SampleRunner Self;
	return Self;
}

void vex::SampleRunner::Register(const char* key, std::function<bool()>&& func)
{
	if (_count >= _maxSamples)
		return;
	_entries[_count++] = SampleEntry{key, func};
}

void vex::SampleRunner::RunSample(const char* key)
{
	for (int i = 0; i < _count; ++i)
	{
		if ((nullptr != _entries[i].Run) && !::strcmp(key, _entries[i].Key))
		{
			spdlog::info(" ==> SampleRunner :: running {}   ", key);

			bool bResult = _entries[i].Run();

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
