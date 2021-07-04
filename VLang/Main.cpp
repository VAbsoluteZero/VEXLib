#include <memory>

#include "Misc/RunSample.h"
#include <VCore/Containers/Dict.h>
#include <VCore/Utils/CoreTemplates.h>
#include "VEXBase.h"
#include "spdlog/sinks/msvc_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

using namespace vex;
using namespace vex;

void setup_loggers();

struct Class
{
	i32 Mem = 0;
};

struct ControlBlock
{
	int32_t Hash = -1;
	int32_t Next = -1;
};

struct SOA : public vex::SOAOneBuffer<int, ControlBlock, ControlBlock>
{
};
struct IntDict : public vex::Dict<i32, i32>
{
};

[[maybe_unused]] constexpr auto SzSoa = sizeof(SOA);
[[maybe_unused]] constexpr auto SzDict = sizeof(IntDict);
 
int main(int argc, char** argv)
{
	setup_loggers(); 

	int Testnt = 10;
	std::vector<int*> vec = {&Testnt, &Testnt};
	const auto& cvec = vec;
	using WTF = decltype(cvec[0]);
	auto cptr = cvec[0]; // decltype(cvec[0])
	*cptr = 12;
	spdlog::info(">vec[0] :: {} ...", *(cvec[0]));

	SampleRunner::Static().RunSample("test_defer");
	SampleRunner::Static().RunSample("ring_test_ctor");

	return 0;
}

void setup_loggers()
{
	auto vs_output = spdlog::create<spdlog::sinks::msvc_sink_st>("def");
	auto console = spdlog::stdout_color_st("win");


	spdlog::set_default_logger(vs_output);
	// spdlog::register_logger(console);
}
