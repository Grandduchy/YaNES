#ifndef PPU_HPP
#define PPU_HPP

#include "GamePak.h"
#include <array>
#include <memory>

class NES;

class Ppu {
    friend struct GamePak;
    friend struct Tests;
    friend class DebugView;
public:
    using PatternTableT = std::array<uint16_t, 8>; // A 8x8 tile, each pixel is 2 bits
    using PaletteT = std::tuple<uint8_t, uint8_t, uint8_t>; // A RGB representation of a palette colour
    using ColorSetT = std::tuple<uint8_t, uint8_t, uint8_t, uint8_t>; // A set of colors, each value is the NES's chrome color signal
    Ppu();
    Ppu(NES&);
    void setNESHandle(NES&) &;

    uint8_t readRegister(const uint16_t& adr);
    void writeRegister(const uint16_t& adr, const uint8_t& val);
    void vRamWrite(const uint16_t& adr, const uint8_t& val);
    uint8_t vRamRead(const uint16_t& adr) const;

    void clear();
    void runCycle();

    static uint16_t createLine(const uint8_t& left, const uint8_t& right);
    PatternTableT getPatternTile(const uint16_t& tileAddress) const;
    PatternTableT getPatternTile(const uint8_t& tileID, bool isLeft) const;
    void stdDrawPatternTile(const uint16_t& tileAddress) const;

    void setVBlank();
    void clearVBlank();

    // Converts a NES's chrome color to regular RGB values
    static PaletteT getRGBPalette(const uint8_t& paletteNum);
    // Get the Palette Selection (0,1,2,3) based on nametable address
    uint8_t getPaletteFromNameTable(const uint16_t& nameTableRelativeAdr, const uint16_t& atrTableStart) const;
    // Get A color set from the palette addresses (defined in wiki where)
    ColorSetT getColorSetFromAdr(const uint16_t& paletteAdr) const;
private:
    std::shared_ptr<NES> nes;
    static const std::array<const PaletteT, 0x40 > RGBPaletteTable;


    uint16_t scanline = 0;
    uint16_t cycle = 0;

    // Four Internal Registers
    uint16_t vAdr = 0; // VRAM address pointer
    uint16_t vTempAdr = 0; // Temporary vram addressing pointer, note that last bit (15th bit) is not used
    uint8_t fineXScroll = 0; // only three bits
    uint8_t writeToggle = 0; // 1 bit

    struct PPUCTRL {
        uint8_t nameTable : 4; // (NN) NameTable selector (0=0x2000, 1=0x2400, 2=0x2800, 3=0x2C00)
        uint8_t increment : 1; // (I) Vram increment per read/write of PPUData (0=1,1=32|going across vs down)
        uint8_t spriteTile : 1; // (S) Sprite Pattern Table Address for 8x8 sprites
        uint8_t bkgrdTile : 1; // (B) Background pattern table address (0=0x000, 1=0x1000)
        uint8_t spriteSz : 1; // (H) Sprite Size (0=8x8, 1=8x16)
        uint8_t masterSlave : 1; // (P) PPU master/slave select
        uint8_t NMI : 1; // (V) Generate NMi @ start of vblank (0=off,1=on)
        uint8_t asByte() const noexcept;
        void fromByte(const uint8_t&) noexcept;
        void clear() noexcept;
    };
    struct PPUMASK {
        uint8_t greyScale : 1; // (G) Produce a greyscale display (0=normal, 1=greyscale)
        uint8_t bkgrdLeftEnable : 1; // (m) show background in left 8 pixels of screen
        uint8_t spriteLeftEnable : 1; // (M) show sprites in left 8 pixels of screen
        uint8_t bkgrdEnable : 1; // (b) show background
        uint8_t spriteEnable : 1; // (s) show sprites
        // Emphasize which colour bits (RGB)
        uint8_t red : 1;
        uint8_t green : 1;
        uint8_t blue : 1;
        uint8_t asByte() const noexcept;
        void fromByte(const uint8_t&) noexcept;
        void clear() noexcept;
    };
    struct PPUSTATUS {
        uint8_t sOverflow : 1; // (O) Sprite Overflow
        uint8_t sprite0Hit : 1; // (S)
        uint8_t vblank : 1; // (V) Vertical Blank has Started
        uint8_t asByte() const noexcept;
        void fromByte(const uint8_t&) noexcept;
        void clear() noexcept;
    };

    // 8 Registers that are exposed to the cpu
    // Descriptions here : https://wiki.nesdev.com/w/index.php/PPU_registers
    PPUCTRL PpuCtrl;
    PPUMASK PpuMask;
    PPUSTATUS PpuStatus;
    uint8_t OamAddr = 0;
    // may or may not be needed, but added for now to easily know the scrolling that was set
    uint16_t scrollPos = 0; // (0-0xFF) -> x scroll, (0x100-0xFFFF) -> y scroll

    std::array<uint8_t, GamePak::KB16> memory{};
    // Oam is list of 64 sprites, each having info of 4 bytes
    // Description of each byte : https://wiki.nesdev.com/w/index.php/PPU_OAM
    std::array<uint8_t, 0xFF> OAM{};
    // Secondary OAM is the oam that is used during the next scanline, only 8 sprites are in this.
    // During rendering secondary oam is the sprites that are on the current scanline
    std::array<uint8_t, 0x20> secondOAM{};


    // Variables used for background ppu proccessing
    // These variables act as temporary variables
    uint8_t nameTableLatch;
    uint8_t attrTableLatch;
    uint16_t patternTableLowLatch;
    uint16_t patternTableHighLatch;
    // Shift registers
    uint8_t attrShiftLow = 0;
    uint8_t attrShiftHigh = 0;
    uint8_t bkShiftLow = 0;
    uint8_t bkShiftHigh = 0;

    void fetchNameTableByte();
    void fetchAttrTableByte();
    void fetchTableLowByte();
    void fetchTableHighByte();
    void lineStore();

    uint8_t bGPixel();

    // Helper functions for getPalette
    // Get the bit shift required from the byte of the attribute table for the nametable
    // for the correct palette to be selected
    uint8_t getShift(const uint16_t& nameTableRelativeAdr) const;
    // Get the Attribute tile address from the nametable address
    uint16_t getAtrAddress(const uint16_t& nameTableRelativeAdr, const uint16_t& atrTableStart) const;

    void renderPixel();
};
#endif // PPU_HPP