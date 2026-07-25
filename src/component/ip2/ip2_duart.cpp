/* 
    m  o  t  i  o  n
    The SGI Emulator

    Copyright (c)2026 starfrost

    ip2_duart.cpp: Emulates two SCN68681 UARTs mapped at 32000000 and 32800000 respectively

    Largely adapted from the MAME emulation:
    https://github.com/mamedev/mame/blob/master/src/devices/machine/mc68681.h
    https://github.com/mamedev/mame/blob/master/src/devices/machine/mc68681.cpp
*/

#include <component/ip2/ip2_duart.hpp>

namespace Iris
{
    uint8_t DUART68681::OnRead8(size_t addr)
    {
        addr = addr & (DUART_NUM_REGS - 1);
        auto duartId = GetDuartIONum(addr);
        
        DUART duart = duarts[duartId];

        int32_t channel = 1;
        uint8_t ret = 0;
        uint8_t mrPtr = 0;

        bool mustUpdateFrame = false;           // true if state of the DUART data framing must be updated
        bool mustUpdateInterrupts = false;      // true if state of the interrupts must be updated

        switch (addr)
        {
            case DUART_MODE_A:
                channel = 0;
                fallthrough;
            case DUART_MODE_B:
                mrPtr = duart.channels[channel].modeRegCurrent;

                if (mrPtr == 0)
                {
                    ret = duart.channels[channel].mode1;
                    duart.channels[channel].modeRegCurrent++;
                }
                else
                    ret = duart.channels[channel].mode2;

                mustUpdateFrame = true;
                mustUpdateInterrupts = true; 
                break;
            case DUART_READ_STATUS_A:
                channel = 0;
                fallthrough;
            case DUART_READ_STATUS_B:
                ret = duart.channels[channel].status;
                break;
                // non standard bit rates
            case DUART_READ_BRG_TEST:
                ret = duart.brgTest;
                break;
            case DUART_READ_RX_HOLD_A:
                channel = 0;
                fallthrough;
            case DUART_READ_RX_HOLD_B:
            
                duart.channels[channel].rxFifoReadPtr++;

                if (duart.channels[channel].rxFifoReadPtr > DUART_FIFO_SIZE)
                    duart.channels[channel].rxFifoReadPtr = 0;
                
                ret = duart.channels[channel].rxFifoReadPtr;

                duart.channels[channel].rxFifoWritePtr--; 

                mustUpdateInterrupts = true; 
                break; 
                
            break;
        }

        if (mustUpdateFrame)
            UpdateDataFrameState(duartId, channel);

        if (mustUpdateInterrupts)
            UpdateInterruptState(duartId, channel);

        Logger::Log(DUART_LOG_PREFIX, std::format("DUART{} read8 from addr 0x{:x}", duartId, addr).c_str(), LogChannels::Debug);

        return ret; 
    }

    uint16_t DUART68681::OnRead16(size_t addr)
    {
        // big endian
        return (OnRead8(addr) << 8) + (OnRead8(addr + 1));
    }

    uint32_t DUART68681::OnRead32(size_t addr)
    {
        // big endian
        return (OnRead8(addr) << 24) + (OnRead8(addr + 1) << 16) + OnRead8(addr + 2) << 8 + OnRead8(addr + 3);
    }

    void DUART68681::OnWrite8(size_t addr, uint8_t value)
    {
        addr = addr & (DUART_NUM_REGS - 1);
        auto duartId = GetDuartIONum(addr);
        Logger::Log(DUART_LOG_PREFIX, std::format("DUART{} write8 0x{:x} to addr 0x{:x}", duartId, value, addr).c_str(), LogChannels::Debug);
    }

    void DUART68681::OnWrite16(size_t addr, uint16_t value)
    {
        OnWrite8(addr, (value) & 0xFF00);
        OnWrite8(addr, (value + 1) & 0x00FF);
    }

    void DUART68681::OnWrite32(size_t addr, uint32_t value)
    {
        // big endian
        OnWrite8(addr, (value) & 0xFF000000);
        OnWrite8(addr, (value + 1) & 0x00FF0000);
        OnWrite8(addr, (value + 2) & 0x0000FF00);
        OnWrite8(addr, (value + 3) & 0x000000FF);
    }

    void DUART68681::UpdateDataFrameState(int32_t duart, int32_t channel)
    {
        
    }

    void DUART68681::UpdateInterruptState(int32_t duart, int32_t channel)
    {
        
    }
}