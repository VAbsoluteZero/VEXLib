#include <nanobench/nanobench.h>

#include <snitch/snitch.hpp>

namespace bench = ankerl::nanobench;

#define BENCH(NAME, ...) TEST_CASE(NAME, __VA_ARGS__)

#define useVar(var) bench::doNotOptimizeAway(var)

static inline ankerl::nanobench::Bench gBench{}; 

#define BENCH_RUN_G(NAME, LAMBDA) gBench.run(NAME, LAMBDA)
#define BENCH_RUN(NAME, LAMBDA) bench::Bench().run(NAME, LAMBDA)