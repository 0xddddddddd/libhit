#ifndef BENCHMARK_STATIC_DEFINE
#   define BENCHMARK_STATIC_DEFINE
#endif

#include <benchmark/benchmark.h>

#include <algorithm>
#include <cstdint>
#include <random>
#include <string>
#include <vector>

#include <hex_match.hpp>

#ifdef _WIN32
#   pragma comment(lib, "shlwapi.lib")
#endif

namespace
{
    constexpr std::size_t kBufSize = 1024 * 1024 * 1024;   // 256 MiB

    // Fixed seed so every benchmark sees exactly the same data
    std::vector<std::uint8_t> make_buffer()
    {
        std::vector<std::uint8_t> buf(kBufSize);
        std::mt19937_64 rng(0xC0FFEE);
        for (auto& b : buf) b = static_cast<std::uint8_t>(rng());
        return buf;
    }

    // Bury the needle near the end so the scan has to walk the whole buffer (worst case)
    struct Fixture
    {
        std::vector<std::uint8_t> buf = make_buffer();
        std::size_t hit_pos;

        explicit Fixture(const std::vector<std::uint8_t>& needle)
        {
            hit_pos = kBufSize - needle.size() - 128;
            std::copy(needle.begin(), needle.end(), buf.begin() + hit_pos);
        }
    };

    const std::string kPatternStr = "?? 8B ?? ?? 89 5C 24 ?? 57 AA BB DD ?? FF FF ?? F1";
    const std::vector<std::uint8_t> kNeedle = {
        0x48, 0x8B, 0x11, 0x22, 0x89, 0x5C, 0x24, 0x33, 0x57, 0xAA, 0xBB, 0xDD, 0xAC, 0xFF, 0xFF, 0xAF, 0xF1
    };

    Fixture& fixture()
    {
        static Fixture f(kNeedle);
        return f;
    }
} // namespace

template <typename T>
static void BM_ours(benchmark::State& state)
{
    if (!details::cpu_ok<T>()) {
        state.SkipWithError("CPU does not support this instruction set");
        return;
    }

    auto& f = fixture();
    std::uint8_t* begin = f.buf.data();
    std::uint8_t* end = begin + f.buf.size();

    std::vector<std::uint16_t> pattern;
    if (hexdigit_to_xdigit(kPatternStr, pattern)) {
        state.SkipWithError("ours: pattern parse failed");
        return;
    }

    // Correctness check runs before the timed loop
    {
        void* addr = nullptr;
        bool failed = search_pattern_simd<T>(begin, end, pattern, 1, 0, 0,
                                             match_address, addr, details::filter_default{});
        if (failed || addr != begin + f.hit_pos) {
            state.SkipWithError("ours: wrong result");
            return;
        }
    }

    for (auto _ : state) {
        void* addr = nullptr;
        bool failed = search_pattern_simd<T>(begin, end, pattern, 1, 0, 0,
                                             match_address, addr, details::filter_default{});
        benchmark::DoNotOptimize(addr);
        benchmark::DoNotOptimize(failed);
    }
    state.SetBytesProcessed(static_cast<std::int64_t>(state.iterations()) * kBufSize);
}

BENCHMARK_TEMPLATE(BM_ours, details::scalar)->Name("ours/scalar")->Unit(benchmark::kMillisecond)->UseRealTime()->Threads(1)->Threads(4);
BENCHMARK_TEMPLATE(BM_ours, details::sse2)->Name("ours/sse2")->Unit(benchmark::kMillisecond)->UseRealTime()->Threads(1)->Threads(4);
BENCHMARK_TEMPLATE(BM_ours, details::sse41)->Name("ours/sse41")->Unit(benchmark::kMillisecond)->UseRealTime()->Threads(1)->Threads(4);
BENCHMARK_TEMPLATE(BM_ours, details::avx2)->Name("ours/avx2")->Unit(benchmark::kMillisecond)->UseRealTime()->Threads(1)->Threads(4);

BENCHMARK_MAIN();