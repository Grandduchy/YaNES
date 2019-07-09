#ifndef DISASSEMBLER6502_HPP
#define DISASSEMBLER6502_HPP

#include "State6502.hpp"
#include <utility>
#include <array>

class Disassembler6502 {
public:
    // First is the particular instruction
    // Second is the addressing mode w/ the instruction
    using addressingPtr = uint16_t (Disassembler6502::*)(State6502&);
    using instrPtr = void (Disassembler6502::*)(State6502&, addressingPtr&);
    using Instr = std::pair<instrPtr, addressingPtr>;

    Disassembler6502();
    
    void runCycle(State6502&);
private:
    std::array<Instr, 0xFF> opcodeTable{};

    void OP_AND(State6502&, addressingPtr&);


    uint16_t ADR_IMPLICIT(State6502&);
    uint16_t ADR_ACCUM(State6502&);
    uint16_t ADR_IMMEDIATE(State6502&);
    uint16_t ADR_ZEROPAGE(State6502&);
    uint16_t ADR_ZEROPAGEX(State6502&);
    uint16_t ADR_ZEROPAGEY(State6502&);
    uint16_t ADR_RELATIVE(State6502&);
    uint16_t ADR_ABS(State6502&);
    uint16_t ADR_ABSX(State6502&);
    uint16_t ADR_ABSY(State6502&);
    uint16_t ADR_INDIRECT(State6502&);
    uint16_t ADR_INDEXINDIRECT(State6502&);
    uint16_t ADR_INDRECTINDEX(State6502&);

};

#endif // DISASSEMBLER6502_HPP