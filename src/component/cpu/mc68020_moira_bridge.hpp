#pragma once 

#include <Motion.hpp>
#include <coherent/coherent.hpp>
#include <component/component.hpp>
#include <component/cpu/cpu.hpp>
#include <component/cpu/moira/Moira.h>
#include <component/ip2/ip2_interrupt.hpp>
#include <base/emulation.hpp>

namespace Motion
{
    #define LOG_PREFIX_68020_BRIDGE         "68020 CPU"

    // The vectors that are just the machine working normally.
    #define MC68020_VECTOR_SYSCALL          32
    #define MC68020_VECTOR_INTERRUPT_FIRST  0x40
    #define MC68020_VECTOR_INTERRUPT_LAST   0x57

    class MC68020MoiraBridge : public Motion::Lisburn::Moira 
    {
        friend class MC68020;

        /*
            The MMU records a failed translation rather than raising it, because building a bus error
            stack frame needs the core. Moira lets us throw one straight out of the memory callbacks -
            it catches std::exception in its execute loop and runs it through processException - so this
            is where an MMU fault becomes a real exception vector 2.
        */
        void RaiseBusErrorIfFaulted(bool isWrite) const
        {
            size_t faultAddress = 0;
            bool faultWasWrite = false;

            if (!AddrSpace::TakeFault(&faultAddress, &faultWasWrite))
                return;

            Motion::Lisburn::StackFrame frame = {};

            frame.addr = (uint32_t)faultAddress;
            frame.pc = getPC();
            frame.sr = getSR();
            frame.ird = getIRD();
            frame.code = (uint16_t)((frame.ird & 0xFFE0) | (isWrite ? 0x00 : 0x10));

            throw Motion::Lisburn::BusError(frame);
        }

        uint8_t read8(uint32_t addr) const override
        {
            AddrSpace::ClearFault();
            uint8_t value = AddrSpace::ReadU8(addr);
            RaiseBusErrorIfFaulted(false);
            return value;
        };

        uint16_t read16(uint32_t addr) const override
        {
            AddrSpace::ClearFault();
            uint16_t value = AddrSpace::ReadU16(addr);
            RaiseBusErrorIfFaulted(false);
            return value;
        };

        void write8(uint32_t addr, uint8_t value) const override
        {
            AddrSpace::ClearFault();
            AddrSpace::WriteU8(addr, value);
            RaiseBusErrorIfFaulted(true);
        };

        void write16(uint32_t addr, uint16_t value) const override
        {
            AddrSpace::ClearFault();
            AddrSpace::WriteU16(addr, value);
            RaiseBusErrorIfFaulted(true);
        }; 

        void didExecuteException(Motion::Lisburn::M68kException exc, uint16_t vector) override
        {
            /*
                Interrupts and the syscall trap are the normal traffic and would drown everything
                else, so only report the exceptions that mean something went wrong. Gated behind
                logCpuTrace along with the rest of the bring-up instrumentation.
            */
            if (traceExceptions && vector != MC68020_VECTOR_SYSCALL
                && (vector < MC68020_VECTOR_INTERRUPT_FIRST || vector > MC68020_VECTOR_INTERRUPT_LAST))
            {
                Logger::Log(LOG_PREFIX_68020_BRIDGE, std::format("exception vector {} taken at pc 0x{:x}, sr 0x{:04x} ({})",
                    vector, getPC(), getSR(), (getSR() & 0x2000) ? "supervisor" : "user").c_str(), LogChannels::Warning);
            }

            Coherent::Exception(vector);
        };

        // Set from the logCpuTrace cvar by MC68020::Start.
        inline static bool traceExceptions = false;

    public:
        /*
            The IP2 does not autovector. An interrupt acknowledge cycle reads a vector number out of
            U118, a PROM addressed by the interrupt level and the state of the local interrupt lines,
            which is what puts the scheduler clock on vector 0x51 rather than autovector 30.
        */
        void UseVectoredInterrupts() { irqMode = Motion::Lisburn::IrqMode::USER; };

        /*
            Moira's default routes disassembly reads through read16, which for us is a real bus cycle
            that can raise a bus error. The debugger disassembles around wherever the PC happens to
            be, every frame, from outside the execute loop - so that throw had nothing to catch it and
            took the whole process down.
        */
        uint16_t read16Dasm(uint32_t addr) const override
        {
            AddrSpacePeek peek;
            return AddrSpace::ReadU16(addr);
        }

        uint16_t readIrqUserVector(uint8_t level) const override
        {
            if (!interrupts)
                interrupts = Emulation::GetMachine()->FindComponentByType<IP2Interrupt>();

            // With no interrupt logic to ask, fall back to what the CPU would do on its own.
            if (!interrupts)
                return (uint16_t)(24 + level);

            return interrupts->GetVector(level);
        }

    private:
        mutable IP2Interrupt* interrupts = nullptr;
    };
}