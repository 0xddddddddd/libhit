#include <fstream>
#include <iostream>
#include <string>
#include <hex_match.hpp>

int main()
{
    std::ifstream file("./test_data.bin", std::ios::binary);
    if (!file)
    {
        std::cerr << "Failed to open ./test_data.bin\n";
        return 1;
    }

    file.seekg(0, std::ios::end);
    const std::streampos file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<std::uint8_t> contents(static_cast<size_t>(file_size));
    file.read(reinterpret_cast<char*>(contents.data()), file_size);

    std::vector<const void*> prt_pool;
    void* ret = nullptr;
    search_pattern_sse2(contents.data(),
                        contents.data() + contents.size(),
                        "?? 7F CC ?1 A?",
                        0,
                        0,
                        0,
                        match_address,
                        ret,
                        [&] (const std::uint8_t* pos, std::size_t nth)
    {
        prt_pool.push_back(reinterpret_cast<const void*>(pos));
        return false;
    }
    );

    assert(!prt_pool.empty());

    for (const auto& v : prt_pool) {
        printf("ret: %p\n", v);
    }

    return 0;
}