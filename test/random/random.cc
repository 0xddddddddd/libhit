#include <iostream>
#include <algorithm>
#include <cstdint>
#include <random>
#include <format>
#include <ranges>
#include <chrono>

#include <hex_match.hpp>

namespace
{
    static void print_time(const char* name, std::uint64_t ns)
    {
        printf("%-12s= %llu ns | %.3f us | %.3f ms | %.6f s\n",
               name,
               static_cast<unsigned long long>(ns),
               ns / 1e3,
               ns / 1e6,
               ns / 1e9);
    }

    static std::mt19937& rng()
    {
        thread_local std::mt19937 gen(std::random_device{}());
        return gen;
    }

    static std::size_t random_size(std::size_t min, std::size_t max)
    {
        std::uniform_int_distribution<std::size_t> dist(min, max);
        return dist(rng());
    }

    std::vector<std::uint8_t> make_buffer(std::size_t min = 4 * 1024,
                                          std::size_t max = 1 * 1024 * 1024)
    {
        std::vector<std::uint8_t> buf(random_size(min, max));
        std::mt19937_64 rng(0xC0FFEE);
        for (auto& b : buf) b = static_cast<std::uint8_t>(rng());
        return buf;
    }

    struct hit_data
    {
        std::vector<std::uint8_t> buf;      // 随机数据
        std::vector<std::uint8_t> needle;   // 相对 buf 的特征码
        std::string pattern;                // 相对 buf 的特征码字符串
        std::size_t offset;                 // 相对 buf 的偏移点
    };

    static void to_pattern(const std::vector<std::uint8_t>& bytes,
                           std::string& pattern,
                           std::size_t wildcard_percent = 25)
    {
        static constexpr std::string_view hex_asc_upper = "0123456789ABCDEF";

        std::uniform_int_distribution<std::size_t> percent(0, 99);
        std::uniform_int_distribution<std::size_t> kind(0, 2);

        for (const auto& v : bytes) {
            char hi = hex_asc_upper[v >> 0x4];
            char lo = hex_asc_upper[v & 0x0F];

            if (percent(rng()) >= wildcard_percent) {
                pattern += hi;
                pattern += lo;
                pattern += ' ';
                continue;
            }

            switch (kind(rng())) {
                case 0:  pattern += '?'; pattern += lo;  break;
                case 1:  pattern += hi;  pattern += '?'; break;
                default: pattern += '?'; pattern += '?'; break;
            }
            pattern += ' ';
        }
    }

    bool make_hit(struct hit_data& hit,
                  std::vector<std::uint8_t>&& buffer)
    {
        if (buffer.empty())
            return false;

        const std::size_t len = random_size(std::min<std::size_t>(10, buffer.size()),
                                            std::min<std::size_t>(100, buffer.size()));

        const std::size_t offset = random_size(0, buffer.size() - len);

        hit.buf = std::move(buffer);
        hit.offset = offset;
        hit.needle.assign(hit.buf.begin() + offset,
                          hit.buf.begin() + offset + len);

        to_pattern(hit.needle, hit.pattern);

        return true;
    }
}

int main()
{
    std::vector<struct hit_data> search_data;

    for (std::size_t i = 0; i < 1000; ++i)
    {
        struct hit_data hit;
        make_hit(hit, make_buffer());
        search_data.push_back(std::move(hit));
    }

    std::size_t ok = 0, fail_search = 0, fail_offset = 0;

    std::uint64_t time_total = 0;
    std::uint64_t time_min = UINT64_MAX;
    std::uint64_t time_max = 0;
    std::uint64_t bytes_total = 0;

    for (std::size_t i = 0; i < search_data.size(); ++i)
    {
        const auto& v = search_data[i];
        void* result = nullptr;

        const auto t0 = std::chrono::steady_clock::now();

        const bool failed = search_pattern(v.buf.data(),
                                                v.buf.data() + v.buf.size(),
                                                v.pattern,
                                                1, 0, 0, match_address, result);

        const auto t1 = std::chrono::steady_clock::now();

        const std::uint64_t elapsed = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());

        time_total += elapsed;
        time_min = std::min(time_min, elapsed);
        time_max = std::max(time_max, elapsed);
        bytes_total += v.buf.size();

        if (failed) {
            printf("[%4zu] FAIL(search) : offset=%-6zu len=%-3zu pattern=%s\n",
                   i, v.offset, v.needle.size(), v.pattern.c_str());
            ++fail_search;
            continue;
        }

        const std::size_t got = static_cast<std::size_t>(static_cast<const std::uint8_t*>(result) - v.buf.data());

        if (got != v.offset) {
            printf("[%4zu] FAIL(offset) : got=%-6zu expect=%-6zu len=%-3zu pattern=%s\n",
                   i, got, v.offset, v.needle.size(), v.pattern.c_str());
            ++fail_offset;
            continue;
        }

        ++ok;
    }

    const std::size_t total = search_data.size();
    const std::size_t fail = fail_search + fail_offset;
    const double ok_rate = total ? ok * 100.0 / total : 0.0;
    const double fail_rate = total ? fail * 100.0 / total : 0.0;

    printf("\n");
    printf("total       = %zu\n", total);
    printf("success     = %zu (%.2f%%)\n", ok, ok_rate);
    printf("fail        = %zu (%.2f%%)\n", fail, fail_rate);
    printf("  search    = %zu\n", fail_search);
    printf("  offset    = %zu\n", fail_offset);

    printf("\n");
    if (total) {
        print_time("time total", time_total);
        print_time("time avg", time_total / total);
        print_time("time min", time_min);
        print_time("time max", time_max);

        const double mb = bytes_total / (1024.0 * 1024.0);
        const double sec = time_total / 1e9;
        printf("%-12s= %.2f MB in %.3f s (%.2f MB/s)\n",
               "throughput", mb, sec, sec > 0 ? mb / sec : 0.0);
    }

    return 0;
}