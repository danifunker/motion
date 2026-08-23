#pragma once 

#include <Motion.hpp>
#include <coherent/coherent.hpp>
#include <component/component.hpp>
#include <component/cpu/cpu.hpp>
#include <component/cpu/moira/Moira.h>
#include <base/emulation.hpp>

namespace Motion
{
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

        void didExecuteException(Motion::Lisburn::M68kException exc, uint16_t vector) override { Coherent::Exception(vector); } ;
    };
}