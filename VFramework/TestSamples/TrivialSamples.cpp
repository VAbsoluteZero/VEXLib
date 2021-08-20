#include "VCore/Containers/Ring.h"
#include "VFramework/Misc/RunSample.h"
#include "VFramework/VEXBase.h"

VEX_TESTSAMPLE_INLINE{"ring_test_ctor", []
    {
        static bool gCalledCtor = false;
        static i32 gCalledDtor = 0;
        struct CtorTst
        {
            i32 Int = 0;
            CtorTst(const CtorTst&) = default;
            CtorTst(CtorTst&&) = default;
            CtorTst() { gCalledCtor = true; }
            CtorTst(i32 i) : Int(i) {}

            CtorTst& operator=(const CtorTst&) = default;

            ~CtorTst()
            { //
                gCalledDtor++;
            }

            operator int() { return Int; }
        };
        CtorTst a;
        CtorTst b;
        CtorTst c;
        a = std::move(b);
        a = std::move(c);
        // std::aligned_storage_t<CtorTst> Store;
        {
            vex::StaticRing<CtorTst, 5> ring;

            auto toStr = [&]() -> std::string
            { //
                return fmt::format("  {} {} {} {} {}  ", ring.AtAbsIndexUnchecked(0).Int,
                    ring.AtAbsIndexUnchecked(1).Int, ring.AtAbsIndexUnchecked(2).Int, ring.AtAbsIndexUnchecked(3).Int,
                    ring.AtAbsIndexUnchecked(4).Int);
            };
            const auto& cring = ring;

            for (i32 i : 5_times)
            {
                ring.Put(i);
                spdlog::info(
                    "+size:{} full:{}  last_i:{} arr:{}", ring.Count(), ring.Full(), ring.TailIndex(), toStr());
            }
            for (auto& val : ring)
            {
                spdlog::info("yy+e:{} ", val.Int);
            }

            spdlog::info("---------------------------");
            for (i32 i : 2_times)
            {
                ring.PopDiscard();
                spdlog::info("-size:{} full:{} last_i:{} arr:{}", ring.Count(), ring.Full(), ring.TailIndex(), toStr());
            }

            for (auto& val : ring)
            {
                spdlog::info("+e:{} ", val.Int);
            }

            for (const auto& cval : cring)
            {
                spdlog::info("+ec:{} ", cval.Int);
            }
            auto r = ring.rbegin();
            for (; r != ring.rend(); ++r)
            {
                spdlog::info("rv ec:{} ", (*r).Int);
            }

            spdlog::info("---------------------------");
            for (i32 i : 10_times)
            {
                ring.Put(i);
                spdlog::info(
                    "+size:{} full:{}  last_i:{} arr:{}", ring.Count(), ring.Full(), ring.TailIndex(), toStr());
            }
            spdlog::info("---------------------------");
        }
        return !gCalledCtor && gCalledDtor;
    }};
