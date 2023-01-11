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
        const char* key = nullptr;
        std::function<bool()> runnable;
    };

    // to easily run specific test sample from main/console if needed.
    struct SampleRunner
    {
    public:
        static SampleRunner& global();

        void registerSample(const char* key, std::function<bool()>&& func);

        void runSamples(const char* key);

    private:
        static const int max_samples = 64;
        SampleEntry entries[max_samples];
        int count = 0;
    };
} // namespace vex

namespace vex::internals
{
    struct RunCmd_Anon
    {
        template <typename Callable>
        RunCmd_Anon(const char* sampleName, Callable&& callable)
        {
            vex::SampleRunner::global().registerSample(sampleName, callable);
        }
    };
}; // namespace vex::internals

#define VexPrivate_DECLARE_TESTSAMPLE(cmdKey, code)                     \
    namespace vex::internals                                            \
    {                                                                   \
        struct RunCmd_##cmdKey                                          \
        {                                                               \
            RunCmd_##cmdKey()                                           \
            {                                                           \
                vex::SampleRunner::global().registerSample(#cmdKey,     \
                    []() -> bool                                        \
                    {                                                   \
                        (code);                                         \
                    });                                                 \
            }                                                           \
        };                                                              \
        static inline auto RegisterRunCmd_##cmdKey = RunCmd_##cmdKey{}; \
    };

#define VEX_TESTSAMPLE(cmdKey, code) VexPrivate_DECLARE_TESTSAMPLE(cmdKey, code);
#define VEX_TESTSAMPLE_INLINE \
    static inline vex::internals::RunCmd_Anon vpint_COMBINE(sample_anon_, __LINE__) =
