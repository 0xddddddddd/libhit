#include <iostream>
#include <hex_match.hpp>

int main()
{
    std::uint8_t buf[256];

    std::memset(buf, 0xCC, sizeof(buf));
    /*
        mov   eax, 0x0F          ; B8 0F 00 00 00
        nop   dword ptr [rax]    ; 0F 1F 00
        syscall                  ; 0F 05
        ret                      ; C3
    */
    static const std::uint8_t code[] = {
        0xB8, 0x0F, 0x00, 0x00, 0x00,
        0x0F, 0x1F, 0x00,
        0x0F, 0x05,
        0xC3
    };

    std::memcpy(buf + 100, code, sizeof(code));

    void* addr = nullptr;

    bool fail = search_pattern(buf, buf + sizeof(buf),
                               "B8 ? 0 0 0 F 1F 0 F 5 C3",
                               1,
                               0,
                               0,
                               match_address,
                               addr);

    std::printf("scalar : fail=%d, offset=%td\n", fail,
                static_cast<std::uint8_t*>(addr) - buf);
    assert(addr != nullptr);

    printf("addr: %p\n", addr);

    return 0;
}