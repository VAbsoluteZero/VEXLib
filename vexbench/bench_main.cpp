#define ANKERL_NANOBENCH_IMPLEMENT
#include <nanobench/nanobench.h>
#include <vexcore/utils/HashUtils.h>
#include <vexcore/utils/VUtilsBase.h>

#include <atomic>

#include "bench_config.h"

BENCH("Measure rng", "[rng]") {
using namespace vex::rng;
    size_t run_cnt = 10'000'000;

    gBench.run("rng float gen: const", [&] {
        Rand rng1;
        useVar(rng1); 
        for (int i = 0; i < run_cnt; i++) {
            float acc = rng1.rand01();
            useVar(acc);
        }
        useVar(rng1);
    });
    gBench.run("rng gen: u32", [&] { 
        Rand rng1;
        useVar(rng1);
        for (int i = 0; i < run_cnt; i++) {
            auto acc = rng1.rand();
            useVar(acc); 
        }
        useVar(rng1);
    });
    gBench.run("rng gen: stateless", [&] {
        auto r = rand();
        useVar(r);
        for (int i = 0; i < run_cnt; i++) {
            auto acc =  Splitmix64::stateless(12345, i);
            useVar(acc);
        } 
    });
    gBench.run("rng gen: marsene", [&] {
        std::mt19937 mt(static_cast<uint32_t>(std::rand()));
        useVar(mt);
        for (int i = 0; i < run_cnt; i++) {
            auto acc = mt();
            useVar(acc);
        }
        useVar(mt);
    });
}

int main(int argc, char** argv) {
    std::optional<snitch::cli::input> args = snitch::cli::parse_arguments(argc, argv);
    if (!args) {
        return 1;
    }

    args->arguments.push_back({"--verbosity", "quiet", "quiet"});
    args->arguments.push_back({{}, {"test regex"}, "[rng]"});
    // args->arguments.push_back({ {}, {"test regex"}, "[wgpu]" });
    snitch::tests.configure(*args);

    bool r = snitch::tests.run_tests(*args) ? 0 : 1;
    return 0;
}