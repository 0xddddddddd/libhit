#ifndef __HEX_MATCH_H__
#define __HEX_MATCH_H__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#	pragma once
#endif

/*
* Pattern word layout (std::uint16_t), one word per pattern byte:
*
*      15        8 7         0
*     +-----------+-----------+
*     |   mask    |  pattern  |
*     +-----------+-----------+
*       HIBYTE(w)   LOBYTE(w)
*
* A memory byte matches when:  (mem & mask) == pattern
*
* mask semantics (per nibble):
*      0xF0  high nibble compared, low nibble wildcard   ("5?")
*      0x0F  low nibble compared, high nibble wildcard   ("?9")
*      0xFF  whole byte compared                         ("57")
*      0x00  whole byte wildcard                         ("??")
*
* Examples:
*      "57" -> 0xFF57
*      "?9" -> 0x0F09
*      "5?" -> 0xF050
*      "??" -> 0x0000
*
* Only words with mask == 0xFF can be used as a memchr anchor.
*/

// Author: 0xddddddddd
// GitHub: https://github.com/0xddddddddd/libhit

#include <iostream>
#include <string>
#include <array>
#include <vector>
#include <cstring>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <bit>
#include <functional>
#include <optional>
#include <span>
#include <memory>
#include <assert.h>
#include <immintrin.h>

#ifdef _WIN32
#   define WIN32_LEAN_AND_MEAN
#   define NOMINMAX
#   include <windows.h>
#   ifndef PF_SSE4_1_INSTRUCTIONS_AVAILABLE
#       define PF_SSE4_1_INSTRUCTIONS_AVAILABLE 37
#   endif
#   ifndef PF_AVX2_INSTRUCTIONS_AVAILABLE
#       define PF_AVX2_INSTRUCTIONS_AVAILABLE 40
#   endif
#   ifndef PF_AVX512F_INSTRUCTIONS_AVAILABLE
#       define PF_AVX512F_INSTRUCTIONS_AVAILABLE 41
#   endif
#endif

#if defined(_MSC_VER) || (defined(__AVX512F__) && defined(__AVX512BW__))
#   define HEXMATCH_HAS_AVX512 1
#else
#   define HEXMATCH_HAS_AVX512 0
#endif

#ifndef CastSet
#   define CastSet(t, x) *std::bit_cast<t*>(x)
#endif

#ifndef CastGet
#   define CastGet(t, x) *std::bit_cast<t*>(x)
#endif

#ifndef CastBit
#   define CastBit(t, x) std::bit_cast<t>(x)
#endif

#ifndef CastRef
#   define CastRef(t, x) (*std::bit_cast<t*>(std::bit_cast<std::uintptr_t>(x)))
#endif

#ifndef MAKEWORD
#   define MAKEWORD(a, b)   ((std::uint16_t)(((std::uint8_t)(((std::uintptr_t)(a)) & 0xff)) | \
                            ((std::uint16_t)((std::uint8_t)(((std::uintptr_t)(b)) & 0xff))) << 8))
#endif

#ifndef MAKELONG
#   define MAKELONG(a, b)   ((std::int32_t)(((std::uint16_t)(((std::uintptr_t)(a)) & 0xffff)) | \
                            ((std::uint32_t)((std::uint16_t)(((std::uintptr_t)(b)) & 0xffff))) << 16))
#endif

#ifndef MAKEBYTE
#   define MAKEBYTE(a, b)   ((std::uint8_t)((((std::uintptr_t)(a)) & 0x0f) << 4 | \
                            (((std::uintptr_t)(b)) & 0x0f)))
#endif

#ifndef LOWORD
#   define LOWORD(l)        ((std::uint16_t)(((std::uintptr_t)(l)) & 0xffff))
#endif

#ifndef HIWORD
#   define HIWORD(l)        ((std::uint16_t)((((std::uintptr_t)(l)) >> 16) & 0xffff))
#endif

#ifndef LOBYTE
#   define LOBYTE(w)        ((std::uint8_t)(((std::uintptr_t)(w)) & 0xff))
#endif

#ifndef HIBYTE
#   define HIBYTE(w)        ((std::uint8_t)((((std::uintptr_t)(w)) >> 8) & 0xff))
#endif

#ifndef HINIBBLE
#   define HINIBBLE(b)      ((std::uint8_t)((((std::uintptr_t)(b)) >> 4) & 0x0f))
#endif

#ifndef LONIBBLE
#   define LONIBBLE(b)      ((std::uint8_t)(((std::uintptr_t)(b)) & 0x0f))
#endif

#if defined(DEBUG) || defined(_DEBUG)
#   pragma optimize("gt", on)
#endif

enum match_type
{
    match_constant,
    match_imm,
    match_address
};

enum class match_mode
{
    scalar,
    sse2,
    sse41,
    avx2,
    avx512,
};

namespace
{
    static const char hex_asc[] = "0123456789abcdef";
    static const char hex_asc_upper[] = "0123456789ABCDEF";
}

/**
 * hex_to_bin - convert a hex digit to its real value
 * @ch: ascii character represents hex digit
 *
 * hex_to_bin() converts one hex digit to its actual value or -1 in case of bad
 * input.
 *
 * This function is used to load cryptographic keys, so it is coded in such a
 * way that there are no conditions or memory accesses that depend on data.
 *
 * Explanation of the logic:
 * (ch - '9' - 1) is negative if ch <= '9'
 * ('0' - 1 - ch) is negative if ch >= '0'
 * we "and" these two values, so the result is negative if ch is in the range
 *	'0' ... '9'
 * we are only interested in the sign, so we do a shift ">> 8"; note that right
 *	shift of a negative value is implementation-defined, so we cast the
 *	value to (unsigned) before the shift --- we have 0xffffff if ch is in
 *	the range '0' ... '9', 0 otherwise
 * we "and" this value with (ch - '0' + 1) --- we have a value 1 ... 10 if ch is
 *	in the range '0' ... '9', 0 otherwise
 * we add this value to -1 --- we have a value 0 ... 9 if ch is in the range '0'
 *	... '9', -1 otherwise
 * the next line is similar to the previous one, but we need to decode both
 *	uppercase and lowercase letters, so we use (ch & 0xdf), which converts
 *	lowercase to uppercase
 */
static constexpr int hexdigit_to_bin(unsigned char ch) noexcept
{
    unsigned int c = ch;
    unsigned int cu = c & 0xdf;
    return static_cast<int>(-1 +
                            ((c - '0' + 1) & (((c - '9' - 1) & ('0' - 1 - c)) >> 8)) +
                            ((cu - 'A' + 11) & (((cu - 'F' - 1) & ('A' - 1 - cu)) >> 8)));
}

// Parse one nibble character.
// Returns (mask << 8 | value), where mask is the *unshifted* 0xF (compare)
// or 0 (wildcard '?'); the position shift is applied later by
// hexdigit_nibble_make().
// Returns -1 on an invalid character.
static constexpr std::int32_t hexdigit_nibble_parse(char c) noexcept
{
    if (c == '?') {
        return 0;
    }

    std::int32_t v = hexdigit_to_bin(c);
    if (v < 0) {
        return -1;
    }

    return static_cast<std::int32_t>(MAKEWORD(static_cast<std::uint8_t>(v), 0xF));
}

// Combine two nibbles into one pattern word.
//   high nibble mask: 0xF << 4     = 0xF0
//   low  nibble mask: 0xF          = 0x0F
//   full byte mask  : 0xF0 | 0x0F  = 0xFF
// Returns -1 on an invalid character, otherwise a value in [0, 0xFFFF]
// (0xFFFF is a valid result for "FF", so -1 is used as the error marker).
static constexpr std::int32_t hexdigit_nibble_make(char hi_ch,
                                                   char lo_ch) noexcept
{
    std::int32_t hi = hexdigit_nibble_parse(hi_ch);
    std::int32_t lo = hexdigit_nibble_parse(lo_ch);
    if (hi < 0 || lo < 0)
        return -1;

    return static_cast<std::int32_t>(MAKEWORD(MAKEBYTE(LOBYTE(hi), LOBYTE(lo)),
                                              MAKEBYTE(HIBYTE(hi), HIBYTE(lo))));
}

//static constexpr bool search_is_pattern(const std::string& data)
//{
//    if (data.empty()) {
//        return true;
//    }
//
//    if (data.size() & 1) {
//        throw std::runtime_error("pattern length must be even, got " +
//                                 std::to_string(data.size()));
//    }
//
//    for (std::size_t i = 0, n = data.size(); i < n; i += 2) {
//        auto hi = hexdigit_nibble_parse(data[i]);
//        auto lo = hexdigit_nibble_parse(data[i + 1]);
//        if (!hi)
//            throw std::runtime_error("invalid char '" + std::string(1, data[i]) +
//                                     "' at position " + std::to_string(i + 1));
//
//        if (!lo)
//            throw std::runtime_error("invalid char '" + std::string(1, data[i + 1]) +
//                                     "' at position " + std::to_string(i + 2));
//    }
//
//    return false;
//}

static bool string_replace(std::string& source,
                           const std::string& delimiters,
                           const std::string& dispose = "",
                           const std::size_t offset = 1)
{
    if (source.empty()) {
        return true;
    }

    if (delimiters.empty()) {
        return true;
    }

    for (std::string::size_type pos = source.find(delimiters); pos != std::string::npos; pos = source.find(delimiters)) {
        if (source.replace(pos, offset, dispose).size()) {
            continue;
        }
        return true;
    }

    return false;
}

static bool hexdigit_to_xdigit(const std::string_view& data,
                               std::vector<std::uint16_t>& buffer)
{
    if (data.empty()) {
        return true;
    }

    std::string masks(data);
    {
        // 去掉 空格
        if (string_replace(masks, " ")) {
            return true;
        }

        if (masks.size() & 1) {
            return true;
        }

        for (std::size_t i = 0, n = masks.size(); i < n; i += 2) {
            std::int32_t mask = hexdigit_nibble_make(masks[i], masks[i + 1]);
            if (mask < 0)
                return true;

            buffer.push_back(static_cast<std::uint16_t>(mask));
        }
    }

    return false;
}

namespace details
{
    template <typename R = void*>
    struct context
    {
        std::uint8_t* start;                      // Scan range start
        std::uint8_t* end;                        // Scan range end (exclusive)
        const std::string& keyword;               // Hexadecimal pattern
        std::size_t index;                        // One-based match index
        std::size_t offset;                       // Offset from the match
        std::size_t size;                         // Value width in bytes
        match_type type;                          // Address calculation mode
        match_mode mode;
        R& address;
    };

    // ---------------------------------------------------------------------
    // CPU feature detection: skip unsupported instruction sets instead of
    // crashing with an illegal instruction
    // ---------------------------------------------------------------------
    static inline bool cpu_sse41()
    {
#ifdef _WIN32
        return IsProcessorFeaturePresent(PF_SSE4_1_INSTRUCTIONS_AVAILABLE);
#else
        return __builtin_cpu_supports("sse4.1");
#endif
    }

    static inline bool cpu_avx2()
    {
#ifdef _WIN32
        return IsProcessorFeaturePresent(PF_AVX2_INSTRUCTIONS_AVAILABLE);
#else
        return __builtin_cpu_supports("avx2");
#endif
    }

    // Our avx512 path needs AVX512BW (_mm512_cmpeq_epi8_mask), so checking F alone is not enough
    static inline bool cpu_avx512()
    {
#ifdef _WIN32
        // Windows has no PF_ constant for BW; query cpuid leaf 7, EBX bit 30 (AVX512BW)
        int r[4];
#ifdef _MSC_VER
        __cpuidex(r, 7, 0);
#else
        __cpuid_count(7, 0, r[0], r[1], r[2], r[3]);
#endif
        return IsProcessorFeaturePresent(PF_AVX512F_INSTRUCTIONS_AVAILABLE) && ((r[1] >> 30) & 1);
#else
        return __builtin_cpu_supports("avx512f") && __builtin_cpu_supports("avx512bw");
#endif
    }

    struct scalar {};
    struct sse2 {};
    struct sse41 {};
    struct avx2 {};
    struct avx512 {};

    template <typename T> inline bool cpu_ok();
    template <> inline bool cpu_ok<scalar>() { return true; }
    template <> inline bool cpu_ok<sse2>() { return true; }
    template <> inline bool cpu_ok<sse41>() { return cpu_sse41(); }
    template <> inline bool cpu_ok<avx2>() { return cpu_avx2(); }
    template <> inline bool cpu_ok<avx512>() { return cpu_avx512(); }

    struct filter_default
    {
        constexpr bool operator()(const std::uint8_t*, std::size_t) const noexcept { return false; }
    };

    using search_filter = std::function<bool(const std::uint8_t*, std::size_t)>;

    template <typename F>
    inline constexpr bool is_no_filter_v = std::is_same_v<std::remove_cvref_t<F>, filter_default> || std::is_same_v<std::remove_cvref_t<F>, std::nullptr_t>;

    template <typename F>
    static constexpr bool notify(F&& filter, const std::uint8_t* pos, std::size_t nth) noexcept
    {
        static_assert(std::is_invocable_v<F&, const std::uint8_t*, std::size_t>,
                      "filter must be callable as bool(const std::uint8_t*, std::size_t)");
        static_assert(std::is_same_v<std::invoke_result_t<F&, const std::uint8_t*, std::size_t>, bool>,
                      "filter must return bool (true = stop searching)");

        if constexpr (is_no_filter_v<F>) {
            return false;
        } else {
            return static_cast<bool>(filter(pos, nth));
        }
    }

    template <typename T>
    static T address_cast(std::uintptr_t value) noexcept
    {
        static_assert(std::is_pointer_v<T> || std::is_integral_v<T>,
                      "address type must be a pointer or an integer");

        if constexpr (std::is_pointer_v<T>) {
            return reinterpret_cast<T>(value);
        } else {
            return static_cast<T>(value);
        }
    }

    template <typename T>
    inline constexpr bool is_addr_v = std::is_pointer_v<T> || std::is_integral_v<T>;

    template <typename T>
    static std::uint8_t* to_byte_ptr(T v) noexcept
    {
        static_assert(is_addr_v<T>, "address must be a pointer or an integer");

        if constexpr (std::is_pointer_v<T>) {
            return reinterpret_cast<std::uint8_t*>(reinterpret_cast<std::uintptr_t>(v));
        } else {
            return reinterpret_cast<std::uint8_t*>(static_cast<std::uintptr_t>(v));
        }
    }

    template <typename T>
    constexpr inline T block_length(T x) noexcept { return (x + (sizeof(T) - 1)) & ~(sizeof(T) - 1); }
    constexpr inline std::uint16_t is_anchor(std::uint16_t w) noexcept { return HIBYTE(w) == 0xFF; }

    template <typename T> struct mode_traits;
    template <> struct mode_traits<scalar> { using type = scalar; };
    template <> struct mode_traits<sse2> { using type = sse2; };
    template <> struct mode_traits<sse41> { using type = sse41; };
    template <> struct mode_traits<avx2> { using type = avx2; };
    template <> struct mode_traits<avx512> { using type = avx512; };

    template <typename T>
    inline constexpr bool is_text_v = std::is_convertible_v<const T&, std::string_view>;

    template <typename T> struct simd_traits;

    template <> struct simd_traits<scalar>
    {
        using type = std::uint8_t*;
        static constexpr std::size_t block = 0;
    };

    template <> struct simd_traits<sse2>
    {
        using type = __m128i;
        using mask_type = std::uint16_t;

        static constexpr std::size_t block = sizeof(type);

        static inline type set1(std::uint8_t v) noexcept { return _mm_set1_epi8(static_cast<char>(v)); }
        static inline type load(const void* p) noexcept { return _mm_loadu_si128(static_cast<const type*>(p)); }
        static inline type load_aligned(const void* p) noexcept { return _mm_load_si128(static_cast<const type*>(p)); }
        static inline mask_type cmpeq_mask(type a, type b) noexcept
        {
            return static_cast<mask_type>(_mm_movemask_epi8(_mm_cmpeq_epi8(a, b)));  // 16 位
        }
        static inline mask_type cmpeq_mask(type a, type b, type c, type d) noexcept
        {
            return static_cast<mask_type>(_mm_movemask_epi8(_mm_and_si128(_mm_cmpeq_epi8(a, b),
                                                                          _mm_cmpeq_epi8(c, d))));
        }
        static inline bool equal(type a, type b) noexcept
        {
            return cmpeq_mask(a, b) == 0xFFFF;
        }
        static inline bool match(type mem, type pat, type mask) noexcept
        {
            return _mm_movemask_epi8(_mm_cmpeq_epi8(_mm_and_si128(_mm_xor_si128(mem, pat), mask), _mm_setzero_si128())) == 0xFFFF;
        }
    };

    template <> struct simd_traits<sse41> : simd_traits<sse2>
    {
        static inline bool match(type mem, type pat, type mask) noexcept
        {
            return _mm_testz_si128(_mm_xor_si128(mem, pat), mask) != 0;
        }
    };

    template <> struct simd_traits<avx2>
    {
        using type = __m256i;
        using mask_type = std::uint32_t;

        static constexpr std::size_t block = sizeof(type);

        static inline type set1(std::uint8_t v) noexcept { return _mm256_set1_epi8(static_cast<char>(v)); }
        static inline type load(const void* p) noexcept { return _mm256_loadu_si256(static_cast<const type*>(p)); }
        static inline type load_aligned(const void* p) noexcept { return _mm256_load_si256(static_cast<const type*>(p)); }
        static inline type band(type a, type b) noexcept { return _mm256_and_si256(a, b); }
        static inline mask_type cmpeq_mask(type a, type b) noexcept
        {
            return static_cast<mask_type>(_mm256_movemask_epi8(_mm256_cmpeq_epi8(a, b)));  // 32 位
        }
        static inline mask_type cmpeq_mask(type a, type b, type c, type d) noexcept
        {
            return static_cast<mask_type>(_mm256_movemask_epi8(_mm256_and_si256(_mm256_cmpeq_epi8(a, b),
                                                                                _mm256_cmpeq_epi8(c, d))));
        }
        static inline bool equal(type a, type b) noexcept
        {
            return cmpeq_mask(a, b) == 0xFFFFFFFFu;
        }
        static inline bool match(type mem, type pat, type mask) noexcept
        {
            return _mm256_testz_si256(_mm256_xor_si256(mem, pat), mask) != 0;
        }
    };

#if HEXMATCH_HAS_AVX512
    template <> struct simd_traits<avx512>
    {
        using type = __m512i;
        using mask_type = std::uint64_t;
        static constexpr std::size_t block = sizeof(type);
        static inline type set1(std::uint8_t v) noexcept { return _mm512_set1_epi8(static_cast<char>(v)); }
        static inline type load(const void* p) noexcept { return _mm512_loadu_si512(p); }
        static inline type load_aligned(const void* p) noexcept { return _mm512_load_si512(static_cast<const type*>(p)); }
        static inline type band(type a, type b) noexcept { return _mm512_and_si512(a, b); }
        static inline mask_type cmpeq_mask(type a, type b) noexcept
        {
            return static_cast<mask_type>(_mm512_cmpeq_epi8_mask(a, b));
        }
        static inline mask_type cmpeq_mask(type a, type b, type c, type d) noexcept
        {
            return static_cast<mask_type>(_mm512_mask_cmpeq_epi8_mask(_mm512_cmpeq_epi8_mask(a, b), c, d));
        }
        static inline bool equal(type a, type b) noexcept
        {
            return cmpeq_mask(a, b) == ~static_cast<mask_type>(0);
        }
        static inline bool match(type mem, type pat, type mask) noexcept
        {
            return _mm512_test_epi8_mask(_mm512_xor_si512(mem, pat), mask) == 0;
        }
    };
#endif

    template <typename T>
    struct matcher
    {
        using traits = simd_traits<T>;

        static constexpr std::size_t block = traits::block;

        struct context
        {
            std::vector<std::array<std::uint8_t, block>> patterns;
            std::vector<std::array<std::uint8_t, block>> masks;
            // GCC -Wignored-attributes
            std::vector<typename traits::type> patterns_v;
            std::vector<typename traits::type> masks_v;
            std::size_t size;
            std::size_t blocks;                                 // 完整块数
            std::size_t tail;                                   // 尾部字节数
            std::size_t tail_pos;
            std::size_t offset;
        };

        static context prepare(const std::vector<std::uint16_t>& keyword)
        {
            const std::size_t count = (keyword.size() + block - 1) / block;

            context ctx{ std::vector<std::array<std::uint8_t, block>>(count),
                         std::vector<std::array<std::uint8_t, block>>(count),
                         std::vector<typename traits::type>(0),
                         std::vector<typename traits::type>(0),
                         keyword.size(),
                         keyword.size() / block,
                         keyword.size() % block,
                         keyword.size() >= block ? keyword.size() - block : 0,
                         keyword.size() / block * block
            };

            for (std::size_t i = 0; i < count; ++i) {
                const std::size_t block_offset = i * block;
                const std::size_t block_size = std::min<std::size_t>(block, keyword.size() - block_offset);
                for (std::size_t j = 0; j < block_size; ++j) {
                    const std::uint16_t word = keyword[block_offset + j];
                    ctx.patterns[i][j] = LOBYTE(word);      // value
                    ctx.masks[i][j] = HIBYTE(word);         // mask, see "Pattern word layout"
                }
                ctx.patterns_v.push_back(traits::load(ctx.patterns[i].data()));
                ctx.masks_v.push_back(traits::load(ctx.masks[i].data()));
            }

            return ctx;
        }

        static inline bool match(const std::uint8_t* pos,   // Candidate match start
                                 std::size_t cursor,        // Bytes remaining from `pos` to the end of the scan range
                                 const context& ctx         // Pre-split pattern/mask blocks (see prepare())
        ) noexcept
        {
            // Compare every complete block of the pattern with one vector operation.
            // These loads stay inside [pos, pos + pattern_size), so they are in range
            // as long as `pos` is a valid match start.
            for (std::size_t i = 0; i < ctx.blocks; ++i) {
                const typename traits::type mem = traits::load(pos + i * block);
                if (!traits::match(mem,
                                   ctx.patterns_v[i],
                                   ctx.masks_v[i])) {
                    return false;
                }
            }

            // Tail: the last `ctx.tail` pattern bytes (pattern_size % block), if any.
            // Fast path: load a whole block at the tail and compare it with a single
            // vector operation. The pattern/mask of the tail block are zero-padded,
            // so the bytes beyond the pattern are wildcards and never affect the result.
            // This reads up to (block - tail) bytes past the end of the pattern, so it is
            // only allowed when enough bytes remain after `pos + ctx.offset`.
            if (cursor <= block && ctx.tail != 0) {
                assert(ctx.blocks == ctx.patterns_v.size() - 1);
                return traits::match(traits::load(pos + ctx.offset),
                                     ctx.patterns_v[ctx.blocks],
                                     ctx.masks_v[ctx.blocks]);
            }

            // Slow path: compare the tail byte by byte, never touching memory beyond
            // the pattern. Used when a full-block load is not safe.
            for (std::size_t i = 0; i < ctx.tail; ++i) {
                if (static_cast<std::uint8_t>(pos[ctx.offset + i] & ctx.masks[ctx.blocks][i]) != ctx.patterns[ctx.blocks][i]) {
                    return false;
                }
            }

            return true;
        }
    };

    template <>
    struct matcher<scalar>
    {
        struct context { const std::vector<std::uint16_t>& keyword; };

        static context prepare(const std::vector<std::uint16_t>& keyword) { return { keyword }; }

        static bool match(const std::uint8_t* pos, const context& ctx) noexcept
        {
            for (std::size_t i = 0, n = ctx.keyword.size(); i < n; ++i) {
                if (static_cast<std::uint8_t>(pos[i] & HIBYTE(ctx.keyword[i])) != LOBYTE(ctx.keyword[i])) {
                    return false;
                }
            }

            return true;
        }
    };

    template <typename T>
    struct anchor
    {
        using traits = simd_traits<T>;
        using mask_t = typename traits::mask_type;
        using type_t = typename traits::type;
        using matcher_t = matcher<T>;

        template <typename U>
        static inline auto cmpeq_mask(std::uint8_t* prev_pos, std::uint8_t* next_pos, U prev, U next)
        {
            return static_cast<mask_t>(traits::cmpeq_mask(traits::load(prev_pos), prev,
                                                          traits::load(next_pos), next));
        }

        template <typename U>
        static inline std::uint8_t* match(std::uint8_t* pos,     // Start address of the current block
                                          std::uint8_t* end,     // Last valid match start (inclusive), i.e. range_end - pattern_size
                                          std::size_t remain,    // Bytes from `pos` to the end of the scan range
                                          U m,                   // Candidate bitmap, bit k corresponds to pos + k
                                          const typename matcher_t::context& ctx
        )
        {
            // Walk the bitmap from the lowest set bit to the highest.
            // Candidate addresses are therefore strictly increasing.
            for (; m; m = static_cast<mask_t>(m & (m - 1)))
            {
                std::uint8_t* cursor = pos + static_cast<std::size_t>(std::countr_zero(m));
                // The bitmap covers a whole block, but near the end of the scan range
                // the high bits map to starts whose pattern would extend past the range.
                // Candidates are increasing, so once one is out of range, all the
                // remaining ones are too: stop here.
                if (cursor > end) {
                    break;
                }

                if (matcher_t::match(cursor, remain, ctx)) {
                    return cursor;
                }
            }

            return nullptr;
        }
    };

    template <typename T>
    static bool search_result(std::uint8_t* pos,  // Match start address returned by the scanner
                              std::size_t offset, // Byte offset from the match start; base = pos + offset
                              T& address,         // Output: the resulting value or address (pointer or integer type)
                              std::size_t size,   // match_constant only: bytes to read at base, one of 1/2/4/8
                              match_type type     // How to interpret base: constant value / rel32 call-jmp target / raw address
    )
    {
        std::uintptr_t base = reinterpret_cast<std::uintptr_t>(pos) + offset;

        switch (type) {
            case match_constant:
                switch (size) {
                    case 1: address = details::address_cast<T>(static_cast<std::uintptr_t>(CastGet(std::uint8_t, base)));  return false;
                    case 2: address = details::address_cast<T>(static_cast<std::uintptr_t>(CastGet(std::uint16_t, base))); return false;
                    case 4: address = details::address_cast<T>(static_cast<std::uintptr_t>(CastGet(std::uint32_t, base))); return false;
                    case 8: address = details::address_cast<T>(static_cast<std::uintptr_t>(CastGet(std::uint64_t, base))); return false;
                    default: break;
                }
                return true;
            case match_imm:
                address = details::address_cast<T>(base + 5 + static_cast<std::uintptr_t>(static_cast<std::intptr_t>(CastGet(std::int32_t, base + 1))));
                return false;
            case match_address:
                address = details::address_cast<T>(base);
                return false;
            default: break;
        }

        return true;
    }

    template <typename T>
    struct scanner
    {
        template <typename R, typename F>
        static bool search_pattern(std::uint8_t* start,                       // Start address of the scan range
                                   std::uint8_t* end,                         // End address of the scan range (exclusive)
                                   const std::vector<std::uint16_t>& keyword, // Pre-parsed pattern words, each is (mask << 8 | pattern)
                                   std::size_t index,                         // One-based index: which match to use when the pattern occurs multiple times
                                   std::size_t offset,                        // Byte offset from the match start; result is computed at (match + offset)
                                   std::size_t size,                          // match_constant only: bytes to read at (match + offset), one of 1/2/4/8
                                   match_type type,                           // How to interpret the match: constant value / rel32 call-jmp target / raw address
                                   R& address,                                // Output: the resulting value or address (pointer or integer type)
                                   F&& filter                                 // Per-match callback bool(const uint8_t* pos, size_t nth); return true to stop searchin
        )
        {
            if (start >= end || keyword.empty() ||
                keyword.size() > static_cast<std::size_t>(end - start)) {
                return true;
            }

            const auto keyword_prev_it = std::ranges::find_if(keyword, details::is_anchor);
            if (keyword_prev_it == keyword.end())
                return true;

            const std::size_t anchor_prev_off = static_cast<std::size_t>(keyword_prev_it - keyword.begin());
            const std::size_t anchor_next_off = keyword.size() - 1 - static_cast<std::size_t>(
                std::ranges::find_if(keyword.rbegin(), keyword.rend(), details::is_anchor) - keyword.rbegin());

            const auto matcher_context = details::matcher<T>::prepare(keyword);

            const auto anchor_v_prev = details::simd_traits<T>::set1(LOBYTE(keyword[anchor_prev_off]));
            const auto anchor_v_next = details::simd_traits<T>::set1(LOBYTE(keyword[anchor_next_off]));

            std::uint8_t* pos = nullptr;
            std::uint8_t* cursor = start;

            for (std::size_t matches = 0, i = 0; cursor + keyword.size() <= end;
                 cursor += details::simd_traits<T>::block, i += details::simd_traits<T>::block)
            {
                if ((i & (64 - 1)) == 0)
                    _mm_prefetch(reinterpret_cast<const char*>(start + i + 4096), _MM_HINT_T0);

                typename details::simd_traits<T>::mask_type hit =
                    details::anchor<T>::cmpeq_mask(cursor + anchor_prev_off,
                                                   cursor + anchor_next_off,
                                                   anchor_v_prev,
                                                   anchor_v_next);

                if (hit == 0) {
                    continue;
                }

                std::size_t remain = static_cast<std::size_t>(end - cursor);
                pos = details::anchor<T>::match(cursor, end, remain,
                                                hit,
                                                matcher_context);
                if (pos) {
                    if (details::notify(filter, pos, ++matches) || matches == index)
                        break;
                }
            }

            if (pos == nullptr) {
                return true;
            }

            return search_result(pos, offset, address, size, type);
        }
    };

    template <>
    struct scanner<details::scalar>
    {
        template <typename R, typename F>
        static bool search_pattern(std::uint8_t* start,
                                   std::uint8_t* end,
                                   const std::vector<std::uint16_t>& keyword,
                                   std::size_t index,
                                   std::size_t offset,
                                   std::size_t size,
                                   match_type type,
                                   R& address,
                                   F&& filter)
        {
            if (start >= end || keyword.empty() ||
                keyword.size() > static_cast<std::size_t>(end - start)) {
                return true;
            }

            const auto it = std::ranges::find_if(keyword, details::is_anchor);
            if (it == keyword.end()) {
                return true;    // 全通配/半字节模式不支持
            }

            const std::size_t anchor_offset = static_cast<std::size_t>(it - keyword.begin());
            const std::uint8_t anchor = LOBYTE(*it);
            const auto ctx = details::matcher<details::scalar>::prepare(keyword);

            std::uint8_t* pos = nullptr;
            std::uint8_t* cursor = start + anchor_offset;
            const std::size_t remaining = static_cast<std::size_t>((end - keyword.size()) - start) + 1;

            for (std::size_t matches = 0, consumed = 0; consumed < remaining; ) {
                std::uint8_t* hit = static_cast<std::uint8_t*>(std::memchr(cursor + consumed, anchor, remaining - consumed));
                if (!hit) {
                    break;
                }

                pos = static_cast<std::uint8_t*>(hit - anchor_offset);
                if (details::matcher<details::scalar>::match(pos, ctx)) {
                    if (details::notify(filter, pos, ++matches) || matches == index)
                        break;
                }

                consumed = static_cast<std::size_t>(hit - cursor) + 1;
            }

            if (pos == nullptr) {
                return true;
            }

            return search_result(pos, offset, address, size, type);
        }
    };
}

template <typename T, typename K, typename R, typename F>
static bool search_pattern_simd(std::uint8_t* start, // Start address of the scan range
                                std::uint8_t* end,   // End address of the scan range (exclusive)
                                const K& keyword,    // Pattern: hex string (e.g. "48 8B ?? 5?") or pre-parsed std::vector<std::uint16_t>
                                std::size_t index,   // One-based index: which match to use when the pattern occurs multiple times
                                std::size_t offset,  // Byte offset from the match start; result is computed at (match + offset)
                                std::size_t size,    // match_constant only: bytes to read at (match + offset), one of 1/2/4/8
                                match_type type,     // How to interpret the match: constant value / rel32 call-jmp target / raw address
                                R& address,          // Output: the resulting value or address (pointer or integer type)
                                F&& filter           // Per-match callback bool(const uint8_t* pos, size_t nth); return true to stop searching
)
{
    if (keyword.empty()) {
        return true;
    }

    if constexpr (details::is_text_v<K>)
    {
        std::vector<std::uint16_t> buffer;
        {
            if (hexdigit_to_xdigit(keyword, buffer)) {
                return true;
            }
            return details::scanner<T>::search_pattern(start, end, buffer, index, offset, size, type, address, std::forward<F>(filter));
        }
    } else {
        return details::scanner<T>::search_pattern(start, end, keyword, index, offset, size, type, address, std::forward<F>(filter));
    }

    return true;
}

template <typename K, typename R, typename F>
static bool search_pattern_simd(std::uint8_t* start, // Start address of the scan range
                                std::uint8_t* end,   // End address of the scan range (exclusive)
                                const K& keyword,    // Pattern: hex string (e.g. "48 8B ?? 5?") or pre-parsed std::vector<std::uint16_t>
                                std::size_t index,   // One-based index: which match to use when the pattern occurs multiple times
                                std::size_t offset,  // Byte offset from the match start; result is computed at (match + offset)
                                std::size_t size,    // match_constant only: bytes to read at (match + offset), one of 1/2/4/8
                                match_type type,     // How to interpret the match: constant value / rel32 call-jmp target / raw address
                                match_mode mode,     // Instruction set used for scanning: scalar / sse2 / sse41 / avx2
                                R& address,          // Output: the resulting value or address (pointer or integer type)
                                F&& filter           // Per-match callback bool(const uint8_t* pos, size_t nth); return true to stop searching
)
{
    switch (mode)
    {
        case match_mode::scalar:
            return search_pattern_simd<details::scalar>(start, end, keyword, index, offset, size, type, address, std::forward<F>(filter));
        case match_mode::sse2:
            return search_pattern_simd<details::sse2>(start, end, keyword, index, offset, size, type, address, std::forward<F>(filter));
        case match_mode::sse41:
            return search_pattern_simd<details::sse41>(start, end, keyword, index, offset, size, type, address, std::forward<F>(filter));
        case match_mode::avx2:
            return search_pattern_simd<details::avx2>(start, end, keyword, index, offset, size, type, address, std::forward<F>(filter));
#if HEXMATCH_HAS_AVX512
        case match_mode::avx512:
            return search_pattern_simd<details::avx512>(start, end, keyword, index, offset, size, type, address, std::forward<F>(filter));
#endif
        default: break;
    }

    return true;
}

template <typename R = void*, typename F = details::filter_default>
static bool search_pattern(std::uint8_t* start,                   // Start address of the scan range
                           std::uint8_t* end,                     // End address of the scan range (exclusive)
                           const std::string_view& keyword,       // Hex pattern string, e.g. "48 8B ?? 5?"
                           std::size_t index,                     // One-based index: which match to use when the pattern occurs multiple times
                           std::size_t offset,                    // Byte offset from the match start; result is computed at (match + offset)
                           std::size_t size,                      // match_constant only: bytes to read at (match + offset), one of 1/2/4/8
                           match_type type,                       // How to interpret the match: constant value / rel32 call-jmp target / raw address
                           R& address,                            // Output address
                           F&& filter = details::filter_default{} // Per-match callback, return true to stop
)
{
    return search_pattern_simd(start, end, keyword, index, offset, size, type,
                               match_mode::scalar, address, std::forward<F>(filter));
}

template <typename R = void*, typename F = details::filter_default>
static bool search_pattern(std::uintptr_t start,                  // Start address of the scan range
                           std::uintptr_t end,                    // End address of the scan range (exclusive)
                           const std::string_view& keyword,       // Hex pattern string, e.g. "48 8B ?? 5?"
                           std::size_t index,                     // One-based index: which match to use when the pattern occurs multiple times
                           std::size_t offset,                    // Byte offset from the match start; result is computed at (match + offset)
                           std::size_t size,                      // match_constant only: bytes to read at (match + offset), one of 1/2/4/8
                           match_type type,                       // How to interpret the match: constant value / rel32 call-jmp target / raw address
                           R& address,                            // Output address
                           F&& filter = details::filter_default{} // Per-match callback, return true to stop
)
{
    return search_pattern(reinterpret_cast<std::uint8_t*>(start),
                          reinterpret_cast<std::uint8_t*>(end),
                          keyword, index, offset, size, type, address,
                          std::forward<F>(filter));
}

template <typename T, typename R = void*, typename F = details::filter_default>
static bool search_pattern_sse2(T start,                               // Start address of the scan range
                                T end,                                 // End address of the scan range (exclusive)
                                const std::string_view& keyword,       // Hex pattern string, e.g. "48 8B ?? 5?"
                                std::size_t index,                     // One-based index: which match to use when the pattern occurs multiple times
                                std::size_t offset,                    // Byte offset from the match start; result is computed at (match + offset)
                                std::size_t size,                      // match_constant only: bytes to read at (match + offset), one of 1/2/4/8
                                match_type type,                       // How to interpret the match: constant value / rel32 call-jmp target / raw address
                                R& address,                            // Output address
                                F&& filter = details::filter_default{} // Per-match callback, return true to stop
)
{
    return search_pattern_simd(details::to_byte_ptr(start), details::to_byte_ptr(end), keyword, index, offset, size, type,
                               match_mode::sse2, address, std::forward<F>(filter));
}

template <typename T, typename R = void*, typename F = details::filter_default>
static bool search_pattern_sse41(T start,                               // Start address of the scan range
                                 T end,                                 // End address of the scan range (exclusive)
                                 const std::string_view& keyword,       // Hex pattern string, e.g. "48 8B ?? 5?"
                                 std::size_t index,                     // One-based index: which match to use when the pattern occurs multiple times
                                 std::size_t offset,                    // Byte offset from the match start; result is computed at (match + offset)
                                 std::size_t size,                      // match_constant only: bytes to read at (match + offset), one of 1/2/4/8
                                 match_type type,                       // How to interpret the match: constant value / rel32 call-jmp target / raw address
                                 R& address,                            // Output address
                                 F&& filter = details::filter_default{} // Per-match callback, return true to stop
)
{
    return search_pattern_simd(details::to_byte_ptr(start), details::to_byte_ptr(end), keyword, index, offset, size, type,
                               match_mode::sse41, address, std::forward<F>(filter));
}

template <typename T, typename R = void*, typename F = details::filter_default>
static bool search_pattern_avx2(T start,                               // Start address of the scan range
                                T end,                                 // End address of the scan range (exclusive)
                                const std::string_view& keyword,       // Hex pattern string, e.g. "48 8B ?? 5?"
                                std::size_t index,                     // One-based index: which match to use when the pattern occurs multiple times
                                std::size_t offset,                    // Byte offset from the match start; result is computed at (match + offset)
                                std::size_t size,                      // match_constant only: bytes to read at (match + offset), one of 1/2/4/8
                                match_type type,                       // How to interpret the match: constant value / rel32 call-jmp target / raw address
                                R& address,                            // Output address
                                F&& filter = details::filter_default{} // Per-match callback, return true to stop
)
{
    return search_pattern_simd(details::to_byte_ptr(start), details::to_byte_ptr(end), keyword, index, offset, size, type,
                               match_mode::avx2, address, std::forward<F>(filter));
}

#if HEXMATCH_HAS_AVX512
template <typename T, typename R = void*, typename F = details::filter_default>
static bool search_pattern_avx512(T start,                               // Start address of the scan range
                                  T end,                                 // End address of the scan range (exclusive)
                                  const std::string_view& keyword,       // Hex pattern string, e.g. "48 8B ?? 5?"
                                  std::size_t index,                     // One-based index: which match to use when the pattern occurs multiple times
                                  std::size_t offset,                    // Byte offset from the match start; result is computed at (match + offset)
                                  std::size_t size,                      // match_constant only: bytes to read at (match + offset), one of 1/2/4/8
                                  match_type type,                       // How to interpret the match: constant value / rel32 call-jmp target / raw address
                                  R& address,                            // Output address
                                  F&& filter = details::filter_default{} // Per-match callback, return true to stop
)
{
    return search_pattern_simd(details::to_byte_ptr(start), details::to_byte_ptr(end), keyword, index, offset, size, type,
                               match_mode::avx512, address, std::forward<F>(filter));
}
#endif

#if defined(DEBUG) || defined(_DEBUG)
#   pragma optimize("gt", off)
#endif

#endif // __HEX_MATCH_H__