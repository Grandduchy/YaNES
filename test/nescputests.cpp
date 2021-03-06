#include "GamePak.h"
#include "tests.hpp"
#include "Cpu6502.h"
#include "NES.h"
#include "Memory.h"


#include <boost/test/results_collector.hpp>
#include <memory>
#include <fstream>
#include <numeric>
#include <iostream>
#include <sstream>
#include <tuple>
// 0->PC, 1->A, 2->X, 3->Y, 4->P, 5->SP, 6->Instruction Description
using TupleState = std::tuple<uint16_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, std::string>;

TupleState getTestState(const std::string& line) {
    std::istringstream strStream(line);
    std::string word;
    TupleState tState;
    strStream >> word; // first is always the pc
    std::get<0>(tState) = static_cast<uint16_t>(std::stoul(word.c_str(), nullptr, 16));
    word.clear();

    std::string instrDesc;
    // Instruction Description varies each line
    // Keep adding to a str until a register appears
    // Note that the instruction returned in the error is the NEXT instruction that would be preformed
    // Meaning the real error is the one before it.
    while (strStream >> word) {
        if (std::find(word.cbegin(), word.cend(), ':') != word.cend()) {// now a register
            word.erase(0,2);
            std::get<1>(tState) = static_cast<uint8_t>(std::stoul(word.c_str(), nullptr, 16));
            break;
        }
        else {
            instrDesc += " " + word;
        }
    }

    // For now do not use a loop
    // Later use varadic templates w/Integer Sequence to do this in a loop
    strStream >> word;
    word.erase(0, 2);
    std::get<2>(tState) = static_cast<uint8_t>(std::stoul(word.c_str(), nullptr, 16)); // x
    strStream >> word;
    word.erase(0, 2);
    std::get<3>(tState) = static_cast<uint8_t>(std::stoul(word.c_str(), nullptr, 16)); // y
    strStream >> word;
    word.erase(0, 2);
    std::get<4>(tState) = static_cast<uint8_t>(std::stoul(word.c_str(), nullptr, 16)); // p
    strStream >> word;
    word.erase(0, 3);
    std::get<5>(tState) = static_cast<uint8_t>(std::stoul(word.c_str(), nullptr, 16)); // sp
    std::get<6>(tState) = instrDesc;

    return tState;
}

inline bool currentTestsPass() {
    using namespace boost::unit_test;
    test_case::id_t id = framework::current_test_case().p_id;
    test_results res = results_collector.results(id);
    return res.passed();
}

void Tests::nesCpuTest() {

    std::cout << "\n--- Running CPU Diagnostics, Nestest ---\n";

    std::shared_ptr<NES> nes = std::make_shared<NES>();
    nes->init();

    Cpu6502& cpu = nes->cpu;
    Memory& memory = nes->cpu.memory;
    GamePak::load(memory, "../rsc/tests/nestest.nes");

    std::ifstream ifsLog("../rsc/tests/nestest.log", std::ios_base::in);
    ckPassFail(ifsLog.good(), "Could not open log file to compare testsing");

    cpu.cpuAllowDec = false;
    cpu.memory = memory;
    cpu.pc = 0xC000;
    cpu.sp = 0xFD;
    cpu.a = cpu.x = cpu.y = 0;
    cpu.status.reset();

    std::string cycleResults;
    for (int i = 1; std::getline(ifsLog, cycleResults); ++i) {
        std::string instrDesc;
        uint8_t a, x, y, p, sp;
        uint16_t pc;
        uint8_t Statep = cpu.status;
        std::tie(pc, a, x, y, p, sp, instrDesc) = getTestState(cycleResults);
        ckPassErr(cpu.a == a, "(" + std::to_string(i) + ") Accumulator Register failure detected at " + instrDesc);
        ckPassErr(cpu.x == x && cpu.y == y, "(" + std::to_string(i) + ") X,Y Register failure detected at " + instrDesc);
        ckPassErr(Statep == p, "(" + std::to_string(i) + ") Status failure detected at " + instrDesc);
        ckPassErr(cpu.sp == sp, "(" + std::to_string(i) +  ") Stack pointer failure detected at " + instrDesc);
        ckPassErr(cpu.pc == pc, "(" + std::to_string(i) + ") Program Counter failure detected at " + instrDesc);
        ckPassErr(cpu.memory.read(0x02) == 0 && cpu.memory.read(0x03) == 0, " CPU NesTest has triggered an error at " + instrDesc);

        if (!currentTestsPass()) {
            BOOST_FAIL(std::string("Cannot continue tests err msg: ") + std::to_string(static_cast<int>(cpu.memory.read(0x2))));
        }
        cpu.runCycle();

        if (i == 5000) { // at around 5000, the tests reaches illegal opcodes which this project will not implement
            std::cout << "NesTest passed with no errors + code(" << static_cast<int>(cpu.memory.read(0x2)) << ')';
            break;
        }
    }

}
