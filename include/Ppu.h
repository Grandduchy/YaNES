#ifndef PPU_HPP
#define PPU_HPP

#include <array>
#include <memory>

#include "functions.hpp"
#include "GamePak.h"

class NES;

// Inner status registers used by the ppu
namespace Inner {
    // Control Status register
    struct PPUCTRL {
        uint8_t nameTable : 4; // (NN) NameTable selector (0=0x2000, 1=0x2400, 2=0x2800, 3=0x2C00)
        uint8_t increment : 1; // (I) Vram increment per read/write of PPUData (0=1,1=32|going across vs down)
        uint8_t spriteTile : 1; // (S) Sprite Pattern Table Address for 8x8 sprites
        uint8_t bkgrdTile : 1; // (B) Background pattern table address (0=0x000, 1=0x1000)
        uint8_t spriteSz : 1; // (H) Sprite Size (0=8x8, 1=8x16)
        uint8_t masterSlave : 1; // (P) PPU master/slave select
        uint8_t NMI : 1; // (V) Generate NMi @ start of vblank (0=off,1=on)
        operator uint8_t() const noexcept;
        void fromByte(const uint8_t&) noexcept;
        void clear() noexcept;
    };
    // Mask status register
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
        operator uint8_t() const noexcept;
        void fromByte(const uint8_t&) noexcept;
        void clear() noexcept;
    };
    // Status (of PPU) register
    struct PPUSTATUS {
        uint8_t sOverflow : 1; // (O) Sprite Overflow
        uint8_t sprite0Hit : 1; // (S)
        uint8_t vblank : 1; // (V) Vertical Blank has Started
        operator uint8_t() const noexcept;
        void fromByte(const uint8_t&) noexcept;
        void clear() noexcept;
    };
}

class Ppu {
    friend struct GamePak;
    friend struct Tests;
    friend class DebugView;
public:
    // Typedefs
    // A 8x8 tile, each pixel is 2 bits
    using PatternTableT = std::array<uint16_t, 8>;
    // A RGB representation of a palette colour
    using PaletteT = std::tuple<uint8_t, uint8_t, uint8_t>;
    // A set of colors, each value is the NES's chrome color signal, essentially the entire palette of colours
    using ColorSetT = std::tuple<uint8_t, uint8_t, uint8_t, uint8_t>;

    Ppu();
    Ppu(std::shared_ptr<NES>);
    void setNESHandle(std::shared_ptr<NES>) &;

    // Runs a cycle of the ppu
    void runCycle();

    // Read Write Register Functions
    // Read Write onto the NES ram bus
    uint8_t readRegister(const uint16_t& adr);
    void writeRegister(const uint16_t& adr, const uint8_t& val);
    // Read write onto the ppu's own ram bus
    void vRamWrite(const uint16_t& adr, const uint8_t& val);
    uint8_t vRamRead(const uint16_t& adr) const;

    //////  ------------- Tester/Viewer Functions --------------
    // This functions are mainly used by the viewer classes to see inside the contents of the ppu

    // Functions dealing with tiles from pattern tables

    // Combines two bit planes to create a single line that represents a 2 bit binary image
    static uint16_t createLine(const uint8_t& left, const uint8_t& right);
    // Function returns a single pattern tile in the form of an array
    // Each element is the bit pixel of the tile
    PatternTableT getPatternTile(const uint16_t& tileAddress) const;
    // Indexing but using a tileId in range (0-0xFF)
    // isLeft determines if its in the left or right pattern table in the vRam
    PatternTableT getPatternTile(const uint8_t& tileID, bool isLeft) const;
    // Prints a tile address to stdout
    void stdDrawPatternTile(const uint16_t& tileAddress) const;

    // These functions are functions used to get the colour from attribute tables
    // Generally anything to do with colours

    // Converts a NES's chroma color to regular RGB values
    static PaletteT getRGBPalette(const uint8_t& paletteNum);
    // Get the Palette Selection (0,1,2,3) based on nametable address
    uint8_t getPaletteFromNameTable(const uint16_t& nameTableRelativeAdr, const uint16_t& atrTableStart) const;
    // Get A color set from the palette addresses (defined in wiki where)
    ColorSetT getColorSetFromAdr(const uint16_t& paletteAdr) const;
    // Gets a chroma colour from the id of palette and bit of pixel
    uint8_t getChromaFromPaletteRam(const uint8_t& paletteID, const uint8_t& pixel) const;
private:
    // Helper functions for getPalette
    // Get the bit shift required from the byte of the attribute table for the nametable
    // for the correct palette to be selected
    uint8_t getShift(const uint16_t& nameTableRelativeAdr) const;
    // Get the Attribute tile address from the nametable address
    uint16_t getAtrAddress(const uint16_t& nameTableRelativeAdr, const uint16_t& atrTableStart) const;

public:
    // Indicator variable for when an entire frame of the ppu has completed
    // at this point its best to draw
    bool completeFrame = false;
    // Completely clears all variables
    void clear();
private:
    std::shared_ptr<NES> nes;
    static const std::array<const PaletteT, 0x40 > RGBPaletteTable;


    int32_t scanline = 0;
    uint16_t cycle = 0;

    // Four Internal Registers
    // VRAM address pointer
    // vAdr contains coarse X (scroll) and coarse Y (scroll) in its lower bits that determines which tile to select
    uint16_t vAdr = 0; // see https://wiki.nesdev.com/w/index.php/PPU_scrolling on how it's deconstructed
    uint16_t vTempAdr = 0; // Temporary vram addressing pointer, note that last bit (15th bit) is not used
    inline uint8_t getFineY() const noexcept; // function gets FineY from vAdr

    // fine X and fine Y select which specific pixel/bit it needs from a particular tile
    // The tile is selected by coarse X and coarse Y. both is contained in vAdr
    // it is 3 bits because each tile is a 8x8 grid, the max range of 3 bits is 0-7 to select which bit it needs
    // fine Y is contained in vAdr.
    uint8_t fineXScroll = 0; // only three bits
    // toggler for reads and writes on the cpu bus
    uint8_t writeToggle = 0; // 1 bit



    // 8 Registers that are exposed to the cpu
    // Descriptions here : https://wiki.nesdev.com/w/index.php/PPU_registers
    Inner::PPUCTRL PpuCtrl;
    Inner::PPUMASK PpuMask;
    Inner::PPUSTATUS PpuStatus;
    uint8_t OamAddr = 0;

    // Ppu has its own RAM on its own bus separate from the CPU
    // See memory map https://wiki.nesdev.com/w/index.php/PPU_memory_map for it's details
    std::array<uint8_t, memsize::KB16> memory{};
    // Oam is list of 64 sprites, each having info of 4 bytes
    // Description of each byte : https://wiki.nesdev.com/w/index.php/PPU_OAM
    std::array<uint8_t, 0xFF> OAM{};
    // Secondary OAM is the oam that is used during the next scanline, only 8 sprites are in this.
    // During rendering secondary oam is the sprites that are on the current scanline
    std::array<uint8_t, 0x20> secondOAM{};


    // ----------- Variables used for background ppu proccessing -----------
    // Renders a pixel by using all the variables and functions below
    void renderPixel();
    // These variables act as temporary variables used for each 8 cycles to put into the shift registers
    // once the 8 cycles are done (then the shift registers has the next 8 pixels)
    uint8_t nameTableLatch = 0;
    uint8_t attrTableLatch = 0;
    uint8_t patternTableLowLatch = 0;
    uint8_t patternTableHighLatch = 0;
    // Functions used to fetch each latch
    void fetchNameTableByte();
    void fetchAttrTableByte();
    void fetchPatternLowByte();
    void fetchPatternHighByte();
    // Background Shift registers
    uint16_t attrShiftLow = 0;
    uint16_t attrShiftHigh = 0;
    uint16_t bkShiftLow = 0;
    uint16_t bkShiftHigh = 0;
    void shiftRegisters() noexcept;
    void updateShifters() noexcept;
    // Four operations are done throughout the proccess of cycling
    // Increment the coarse X and Y variables to select a new tile
    void coraseXIncr(); // done every 8 cycles(needs the next tile)
    void coraseYIncr(); // done every scanline (needs the next line of each tile)
    // Transfer parts of the temp X or Y into the vAdr
    void transferX();
    void transferY();
    // Setting and clearing vertical blanks
    void setVBlank();
    void clearVBlank();


};
#endif // PPU_HPP
