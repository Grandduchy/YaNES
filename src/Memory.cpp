#include "Memory.hpp"
#include "NES.hpp"

#include <fstream>
#include <iostream>
#include <memory>

constexpr inline bool inRange(const uint16_t& min, const uint16_t& max, const uint16_t& val) {
    return val <= max && val >= min;
}


Memory::Memory() {
    static constexpr bool warn = false;
    if (warn)
        std::cerr << "Warning, Memory class does not have a NES handle\n";
}

Memory::Memory(NES& nes) {
    setNESHandle(nes);
}

void Memory::setNESHandle(NES& nes) {
    this->nes = std::make_shared<NES>(nes);
}

uint8_t Memory::read(const uint16_t& adr) const {
    if (inRange(0x0800, 0x0FFF, adr)) // ram mirror, repeats every 0x0800
        return memory[adr % 0x0800];
    else if (inRange(0x2000, 0x3FFF, adr)) // nes ppu register mirrors, repeats every 0x8
        return nes->ppu.readRegister(0x2000 + adr % 8);
    else if (adr == 0x4014)
        return nes->ppu.readRegister(adr);
    else
        return memory[adr];
}

void Memory::write(const uint16_t& adr, const uint8_t& val) {
    if (inRange(0x0800, 0x0FFF, adr)) // ram mirror, repeats every 0x0800
        memory[adr % 0x0800] = val;
    else if (inRange(0x2000, 0x3FFF, adr)) // nes ppu register mirrors, repeats every 0x8
        nes->ppu.writeRegister(0x2000 + adr % 8, val);
    else if (adr == 0x4014)
        return nes->ppu.writeRegister(adr, val);
    else
        memory[adr] = val;
}

void Memory::clear() {
    memory.fill(0);
}


uint8_t& Memory::operator[](const size_t& index) {
    return memory[index];
}

const uint8_t& Memory::operator[](const size_t& index) const {
    return memory[index];
}
