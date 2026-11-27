# libhit

**libhit** 是一个用于在进程内存或其他字节缓冲区中搜索十六进制字节模式的 C++20 头文件库，支持整字节和半字节通配符，并提供标量、SSE2、SSE4.1、AVX2 与 AVX-512 搜索实现。

> 搜索函数接收调用者提供的有效内存区间；本库不负责读取其他进程的内存，也不负责分配或管理搜索缓冲区。

## 功能

- 模式支持完整字节（`57`）、单字节通配符(`?`)、整字节通配符（`??`）和半字节通配符（`5?`、`?9`）。
- 支持查找第 N 次匹配，以及通过回调检查、收集或提前终止匹配。
- 支持返回匹配地址、读取匹配位置偏移处的值，或解析相对立即数目标地址。
- 提供标量、SSE2、SSE4.1、AVX2 与 AVX-512 实现；具体实现是否安全可用取决于 CPU 支持情况。调用显式 SIMD 版本前，应确保目标 CPU 支持相应指令集。
- Header-only：核心实现位于 `include/hex_match.hpp`，无需单独编译库。

## 模式格式

每个模式字节由两个十六进制字符组成，字节之间可使用空格：

| 模式 | 掩码 | 含义 | 示例匹配 |
| --- | --- | --- | --- |
| `57` | `0xFF` | 精确匹配完整字节 | `0x57` |
| `??` | `0x00` | 高、低半字节均为通配符 | `0x00`–`0xFF` |
| `5?` | `0xF0` | 高半字节为 `5`，低半字节任意 | `0x50`–`0x5F` |
| `?9` | `0x0F` | 低半字节为 `9`，高半字节任意 | `0x09`、`0x19`、…、`0xF9` |

当前实现要求模式至少包含 **2 个有效字节**作为搜索锚点（即至少两个完整的十六进制字节，如 `48 8B`）。仅有一个完整字节、空模式、格式错误的模式，或不含两个完整字节锚点的模式不会匹配。

## 快速开始

```cpp
#include <hex_match.hpp>
#include <cstdint>
#include <iostream>

int main()
{
    std::uint8_t bytes[] = {0x48, 0x8B, 0x05, 0x12, 0x34, 0x56, 0x78};
    void* result = nullptr;

    // 返回 false 表示找到，true 表示未找到或参数/模式无效。
    const bool failed = search_pattern(
        bytes,
        bytes + sizeof(bytes), // 搜索区间为 [start, end)
        "48 8B 05",
        1,                      // 第一次匹配（index 从 1 开始）
        0,                      // 相对匹配起点的偏移
        0,                      // match_address 时不使用
        match_address,
        result);

    if (!failed) {
        std::cout << "Found at: " << result << '\n';
    }
}
```

匹配通配符时，只需更改模式，例如 `"48 8B ?? 5? ?9"`。结束地址不包含在搜索范围内，且 `start` 与 `end` 必须描述有效、可读的同一缓冲区区间。

## API 概览

头文件提供通用 `search_pattern(...)` 接口和显式 SIMD 入口，如 `search_pattern_sse2(...)`、`search_pattern_sse41(...)`、`search_pattern_avx2(...)`；AVX-512 接口是否可用取决于编译器及构建配置。也可使用 `search_pattern_simd<Implementation>(...)` 选择实现。

| 参数 | 说明 |
| --- | --- |
| `start` | 搜索区间起始地址，包含在搜索范围内 |
| `end` | 搜索区间结束地址，不包含在搜索范围内；范围为 `[start, end)` |
| `keyword` | 十六进制字节模式字符串，或 API 支持的预解析模式 |
| `index` | 目标匹配序号，从 `1` 开始；回调可检查匹配并在返回 `true` 时停止搜索 |
| `offset` | 相对于匹配起始位置的字节偏移 |
| `size` | `match_constant` 读取的字节数；仅使用该模式时有意义 |
| `type` | 结果处理方式：`match_constant` 读取值、`match_address` 返回匹配地址、`match_imm` 计算相对立即数目标 |
| `address` | 接收结果的输出引用，类型需与所选结果模式兼容 |
| `filter` | 可选回调，参数为匹配地址和匹配序号；返回 `true` 停止继续搜索 |

| 返回值 | 含义 |
| --- | --- |
| `false` | 找到并处理了目标匹配 |
| `true` | 未找到匹配，或模式/参数无效 |

偏移、读取宽度和地址计算必须处于调用者可安全访问的范围内。请以 `include/hex_match.hpp` 中的函数签名与实现为准；该头文件是当前 API 的权威来源。

## 构建项目与示例

要求 CMake 3.20 或更高版本及支持 C++20 的编译器。仓库根 CMake 项目默认在 Windows Visual Studio 生成器中选择 Win32（x86）；如需其他架构，请在首次配置构建目录时显式指定，例如 `-A x64`。

```sh
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

仓库包含三个示例子项目：

- `src/small`：使用内置字节数组进行搜索。
- `src/small_file`：从 `test_data.bin` 读取数据并搜索。
- `src/small_filter`：演示使用回调收集多个匹配结果。

文件示例所需数据来自 `vsprojects/small_file/test_data.bin` 和 `vsprojects/small_filter/test_data.bin`，CMake 会在配置时复制到对应的构建目录。

## Benchmark

`test/benchmark.cc` 使用 Google Benchmark 测量不同搜索实现的性能。依赖通过 Git 子模块固定为 Google Benchmark `v1.10.0`（见 `.gitmodules`）。首次克隆仓库时初始化子模块：

```sh
git submodule update --init --recursive
```

如果本地 `third-party/benchmark` 缺失，CMake 配置也会尝试获取 v1.10.0；需要离线构建时，可预先初始化子模块，或在 CMake 中关闭 `LIBHIT_FETCH_BENCHMARK` 并提供有效的 `LIBHIT_BENCHMARK_SOURCE_DIR`。

```sh
cmake --build build --config Release --target libhit_benchmark
```

Benchmark 当前会创建 **32 MiB** 测试缓冲区，搜索命中位置靠近缓冲区末尾；首次运行会初始化该数据。下方性能数据为已完成的实测结果，来自 README 记录的测试环境；不同 CPU、编译器、构建配置及系统负载可能导致结果变化。

## 性能测试

### 测试环境

| 项目 | 配置 |
| --- | --- |
| 操作系统 | Windows 11 26H2 |
| 开发工具 | Visual Studio LTSC 18.11 |
| 处理器 | Intel Core i9-13950HX |
| 内存 | 96 GB DDR5 5200 MT/s |
| 基准框架 | Google Benchmark |
| 线程数 | 1、4 |
| 指标 | Time、CPU、Iterations、bytes_per_second |

### 64 MB 数据集

| 实现 | 线程数 | Time | CPU | Iterations | 吞吐量 |
| --- | ---: | ---: | ---: | ---: | ---: |
| Scalar | 1 | 12.1 ms | 11.3 ms | 58 | 5.16 GiB/s |
| Scalar | 4 | 12.1 ms | 12.3 ms | 56 | 20.69 GiB/s |
| SSE2 | 1 | 3.30 ms | 3.33 ms | 216 | 18.92 GiB/s |
| SSE2 | 4 | 3.79 ms | 3.74 ms | 192 | 66.05 GiB/s |
| SSE4.1 | 1 | 3.00 ms | 2.87 ms | 218 | 20.86 GiB/s |
| SSE4.1 | 4 | 3.84 ms | 3.75 ms | 200 | 65.12 GiB/s |
| AVX2 | 1 | 2.73 ms | 2.61 ms | 257 | 22.89 GiB/s |
| AVX2 | 4 | 3.59 ms | 3.59 ms | 196 | 69.73 GiB/s |

### 128 MB 数据集

| 实现 | 线程数 | Time | CPU | Iterations | 吞吐量 |
| --- | ---: | ---: | ---: | ---: | ---: |
| Scalar | 1 | 24.0 ms | 23.7 ms | 29 | 5.21 GiB/s |
| Scalar | 4 | 24.1 ms | 24.0 ms | 28 | 20.74 GiB/s |
| SSE2 | 1 | 6.78 ms | 6.77 ms | 97 | 18.43 GiB/s |
| SSE2 | 4 | 7.91 ms | 7.98 ms | 92 | 63.20 GiB/s |
| SSE4.1 | 1 | 6.61 ms | 6.28 ms | 107 | 18.92 GiB/s |
| SSE4.1 | 4 | 8.37 ms | 8.52 ms | 88 | 59.72 GiB/s |
| AVX2 | 1 | 6.02 ms | 5.57 ms | 115 | 20.75 GiB/s |
| AVX2 | 4 | 8.04 ms | 8.00 ms | 84 | 62.19 GiB/s |

### 1024 MB 数据集

| 实现 | 线程数 | Time | CPU | Iterations | 吞吐量 |
| --- | ---: | ---: | ---: | ---: | ---: |
| Scalar | 1 | 201 ms | 203 ms | 3 | 4.98 GiB/s |
| Scalar | 4 | 204 ms | 207 ms | 4 | 19.64 GiB/s |
| SSE2 | 1 | 55.7 ms | 52.9 ms | 13 | 17.96 GiB/s |
| SSE2 | 4 | 68.2 ms | 71.6 ms | 12 | 58.61 GiB/s |
| SSE4.1 | 1 | 56.3 ms | 55.3 ms | 13 | 17.78 GiB/s |
| SSE4.1 | 4 | 65.4 ms | 65.1 ms | 12 | 61.19 GiB/s |
| AVX2 | 1 | 51.8 ms | 51.3 ms | 14 | 19.31 GiB/s |
| AVX2 | 4 | 65.4 ms | 62.5 ms | 12 | 61.18 GiB/s |

> 以上为已测量数据，不是跨平台性能保证。测试代码中的缓冲区大小应与对应数据集一致；运行不同数据集时请记录所用代码版本和配置。

## 目录结构

```text
include/hex_match.hpp       核心 header-only 实现
src/small/                  内置数据示例
src/small_file/             文件搜索示例
src/small_filter/           回调过滤示例
test/benchmark.cc           Google Benchmark 基准程序
test/random/random.cc       安全单元测试
third-party/benchmark/      Google Benchmark 子模块
```

## 注意事项

- 只搜索调用者有权访问且已映射为可读的内存区间；不要将无效指针或不连续内存区间传入搜索接口。
- 对 `search_pattern_sse41`、`search_pattern_avx2`、`search_pattern_avx512` 等显式 SIMD 入口，调用方应先确认目标处理器支持相应指令集；不支持时可能触发非法指令异常。通用接口的具体选择行为请参照头文件实现。
- `match_constant` 和 `match_imm` 会根据偏移读取或计算地址，调用方必须确保相关字节有效且在允许访问的范围内。
- Benchmark 是性能测试程序，不是单元测试；运行前请考虑其初始化和计算时间。
