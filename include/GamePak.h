#ifndef GAMEPAK_HPP
#define GAMEPAK_HPP

#include <cstdint>
#include <string>
#include <memory>

class Memory;
class Ppu;
class NES;

class GamePak {
    friend struct Tests;
public:
    // Type of mirroring of flag 6
    enum MIRRORT {
        HORIZONTAL = 0,
        VERTICAL = 1
    };
    GamePak() = default;
    GamePak(std::shared_ptr<NES>);

    void setNESHandle(std::shared_ptr<NES>) &;

    void load(const std::string& fname);

    uint8_t PRG_ROM_sz = 0; // Program Read only memory in 16kb size
    uint8_t CHR_ROM_sz = 0; // Character Read only memory in 8kb size
    uint8_t mapper = 0; // Mapper number
    MIRRORT mirror = MIRRORT::HORIZONTAL;
    uint8_t flags7 = 0, flags8 = 0, flags9 = 0, flags10 = 0; // flags used in header, currently unused in this project

private:
    std::shared_ptr<NES> nes;
    static GamePak cpuLoad(Memory& memory, std::ifstream& ifs);
    // Old ways of loading memory, used by load however.
    // These are used by Tests
    static GamePak load(Memory& memory, Ppu& ppu, const std::string& fname);
    static GamePak load(Memory& memory, const std::string& fname);
};

#endif // GAMEPAK_HPP
