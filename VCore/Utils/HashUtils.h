#pragma once
/*
 * MIT LICENSE
 * Copyright (c) 2019 Vladyslav Joss
 */
#include <VCore/Utils/CoreTemplates.h>

#include <random>

#if INTPTR_MAX == INT64_MAX
#define VEXCORE_x64
#elif INTPTR_MAX == INT32_MAX
#define VEXCORE_x32
#else
#error Unknown ptr size, abort
#endif

#pragma warning(push)
#pragma warning(disable : 26495)
#pragma warning(disable : 26451)
#pragma warning(disable : 28812)
#include <VCore/Dependencies/MurmurHash3.h>
#pragma warning(pop)

namespace vex
{
    namespace util
    {
        static constexpr i32 gPrimeNumbers[] = {3, 7, 11, 17, 23, 29, 37, 47, 59, 71, 89, 107,
            131, 163, 197, 239, 293, 353, 431, 521, 631, 761, 919, 1103, 1327, 1597, 1931, 2333,
            2801, 3371, 4049, 4861, 5839, 7013, 8419, 10103, 12143, 14591, 17321, 21269, 25253,
            31393, 39769, 49157, 62851, 90523, 108631, 130363, 156437, 187751, 225307, 270371,
            324449, 389357, 467237, 560689, 672827, 807403, 946037, 1395263, 1572869, 2009191,
            2411033, 2893249, 3471899, 4166287, 4999559, 5999471, 7199369};

        static constexpr i32 gPrimeSize = sizeof(gPrimeNumbers) / sizeof(i32);

        inline constexpr i32 findUpperBound(const i32* a, i32 n, i32 x)
        {
            if (x > a[n - 1])
                return a[n - 1];

            i32 iter = 0;
            i32 count = n;
            while (iter < count)
            {
                i32 mid = (iter + count) / 2;
                if (x > a[mid])
                {
                    iter = mid + 1;
                }
                else
                {
                    count = mid;
                }
            }

            return a[iter];
        }

        inline constexpr i32 closestPrime(i32 value)
        {
            for (i32 i = 0; i < gPrimeSize; i++)
            {
                i32 prime = gPrimeNumbers[i];
                if (prime >= value)
                    return prime;
            }

            return gPrimeNumbers[gPrimeSize - 1];
        }

        inline constexpr bool isPrime(i32 val)
        {
            // works for 1 too.
            if ((val & 1) != 0)
            {
                i32 stop = (i32)std::sqrt(val);
                for (i32 div = 3; div <= stop; div += 2)
                {
                    if ((val % div) == 0)
                        return false;
                }
                return true;
            }
            return (val == 2);
        }

        inline constexpr i32 closestPrimeSearch(i32 value)
        {
            [[unlikely]] if (value > gPrimeNumbers[gPrimeSize - 1])
            {
                for (i32 i = (value | 1); i < INT32_MAX; i += 2)
                {
                    if (isPrime(value))
                        return i;
                }
            }

            return findUpperBound(gPrimeNumbers, gPrimeSize, value);
        }

        inline i32 randomRange(i32 fromInc, i32 toExc)
        {
            static std::random_device rd;
            static std::mt19937 mt(rd());
            std::uniform_int_distribution<int> dist(fromInc,
                toExc - 1); // exclusive, as it is more common use case
            return dist(mt);
        }

        static constexpr u64 k_fnv_prime = 16777619u;
        static constexpr u64 k_fnv_offset_basis = 2166136261u;
        static constexpr u64 k_fnv_prime64 = 1099511628211ull;
        static constexpr u64 k_fnv_offset_basis64 = 14695981039346656037ull;

        template <bool b64>
        const u64 fnv_prime = b64 ? k_fnv_prime64 : k_fnv_prime;
        template <bool b64>
        const u64 fnv_offset_basis = b64 ? k_fnv_offset_basis64 : k_fnv_offset_basis;

        template <bool b64 = false>
        static inline constexpr auto fnv1a(const char* text)
        {
            u64 hash = fnv_offset_basis<b64>;
            for (auto it = text; *it != '\0'; ++it)
            {
                hash ^= *it;
                hash *= fnv_prime<b64>;
            }
            using value_type = std::conditional_t<b64, i32, i64>;
            return (value_type)hash;
        }
        template <bool b64 = false>
        static inline constexpr auto fnv1a(u8* bytes, u32 len)
        {
            u64 hash = fnv_offset_basis<b64>;
            for (auto i = 0; i < len; ++i)
            {
                hash ^= bytes[i];
                hash *= fnv_prime<b64>;
            }

            using value_type = std::conditional_t<b64, i32, i64>;
            return (value_type)hash;
        }
        template <typename T, bool b64 = false>
        static inline constexpr auto fnv1a_obj(const T& obj)
        {
            return fnv1a<b64>((u8*)&obj, sizeof(T));
        }

        static inline i32 hash(char* c, i32 sz)
        {
            return (i32)murmur::MurmurHash3_x86_32(c, sz);
        }

        struct SHash
        {
            static inline i32 hash(const std::string& str)
            {
#ifdef ECSCORE_x64
                return (i32)murmur::MurmurHash3_x86_32(str.data(), (i32)str.size());
#else
                return fnv1a((u8*)str.data(), str.length());
#endif
            }
        };
        struct SHash_STD
        {
            static inline i32 hash(const std::string& str)
            {
                return (i32)std::hash<std::string>{}(str);
            }
        };
        struct SHash_FNV1a
        {
            static inline i32 hash(const std::string& str)
            {
                return fnv1a((u8*)str.data(), str.length());
            }
        };
        struct SHash_MURMUR
        {
            static inline i32 hash(const std::string& str)
            {
                return (i32)murmur::MurmurHash3_x86_32(str.data(), (i32)str.size());
            }
        };
    } // namespace util

    constexpr auto operator"" _fnv1a64(const char* cstr, u64 len) 
    {
        return util::fnv1a<true>((u8*)cstr, len);
    }
    constexpr auto operator"" _fnv1a(const char* cstr, u64 len)
    {
        return util::fnv1a((u8*)cstr, len);
    }
} // namespace vex
