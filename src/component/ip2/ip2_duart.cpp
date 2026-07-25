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
        
        // bit 4 is used for channel selection on channel regs
        int32_t channel = (addr & 0x08); 
        uint8_t ret = 0;
        uint8_t mrPtr = 0;

        bool mustUpdateFrame = false;           // true if state of the DUART data framing must be updated
        bool mustUpdateInterrupts = false;      // true if state of the interrupts must be updated

        switch (addr)
        {
            case DUART_MODE_A:
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
            case DUART_READ_STATUS_B:

                ret = duart.channels[channel].status;
                break;
                // non standard bit rates
            case DUART_READ_BRG_TEST:
                ret = duart.brgTest;
                break;
            // FIFO read
            case DUART_READ_RX_HOLD_A:
            case DUART_READ_RX_HOLD_B:
                if (!duart.channels[channel].rxFifoFree)
                {
                    Logger::Log(DUART_LOG_PREFIX, std::format("DUART{} RX FIFO {} underflow...", duartId, channel).c_str());
                    mustUpdateInterrupts = true;
                    break; 
                }

                duart.channels[channel].rxFifoReadPtr++;

                if (duart.channels[channel].rxFifoReadPtr > DUART_FIFO_SIZE)
                    duart.channels[channel].rxFifoReadPtr = 0;

                // decrement free count
                duart.channels[channel].rxFifoFree--;
                ret = duart.channels[channel].rxFifoReadPtr;

                // Handle the fifo flags

                if (duart.channels[channel].rxFifoFree != DUART_FIFO_SIZE)
                    duart.channels[channel].status &= ~(DUART_STATUS_FIFO_FULL);

                // handle no error
                if (!(duart.channels[channel].mode1 & DUART_MODE_BLOCK_ERROR))
                    duart.channels[channel].status &= ~(DUART_STATUS_RECEIVED_BREAK | DUART_STATUS_FRAMING_ERROR | DUART_STATUS_PARITY_ERROR);

                // can't receive anything if we have not parsed anything
                if (!duart.channels[channel].rxFifoFree)
                    duart.channels[channel].status &= ~(DUART_STATUS_RECEIVER_READY);
                else 
                    duart.channels[channel].status |= ((duart.channels[channel].rxFifo[duart.channels[channel].rxFifoReadPtr]) >> 8); // throw in the latest fifo read 
                
                mustUpdateInterrupts = true; 

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

        DUART& duart = duarts[duartId];

        int32_t channel = (addr & 0x08); 
        uint8_t ret = 0;
        uint8_t mrPtr = 0;

        bool mustUpdateFrame = false;           // true if state of the DUART data framing must be updated
        bool mustUpdateInterrupts = false;      // true if state of the interrupts must be updated

        switch (addr)
        {
            // mode reg
            case DUART_MODE_A:
            case DUART_MODE_B:
                mrPtr = duart.channels[channel].modeRegCurrent;

                if (mrPtr == 0)
                {
                    duart.channels[channel].mode1 = value;
                    duart.channels[channel].modeRegCurrent++;
                }
                else
                    duart.channels[channel].mode2 = value;

                mustUpdateFrame = true;
                mustUpdateInterrupts = true; 
                break;
            case DUART_WRITE_CLOCKSEL_A:
            case DUART_WRITE_CLOCKSEL_B:
                duart.channels[channel].clocksel = value;
                SetBaudRate(duartId, channel, false, (value & 0x0F));
                SetBaudRate(duartId, channel, true, ((value >> 4) & 0x0F));


                break;

        }

        if (mustUpdateFrame)
            UpdateDataFrameState(duartId, channel);

        if (mustUpdateInterrupts)
            UpdateInterruptState(duartId, channel);

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

    void DUART68681::Tick()
    {
        // there's two clocks so we run our own clocks
    }

    void DUART68681::SetBaudRate(int32_t duart, int32_t channelId, bool isRx, uint8_t data)
    {
        UARTChannel& channel = duarts[duart].channels[channelId];

        int32_t baudRate = baudRateACR0[data & 0x0F];

        // 7th bit of acr switches baud rate
        if ((duarts[duart].auxControl) & 0x80) //bit0
            baudRate = baudRateACR1[data & 0x0F];

        // channel a
        if (!channelId)
        {
            // external from ip3, divide by 16
            if ((data & 0x0F) == 0x0E)
                baudRate = ip3clk >> 4;
            else if ((data & 0x0F) == 0x0F) // 0x0f
                baudRate = ip3clk;
        }
        else // channelb
        {
            // external from ip5, divide by 16
            if ((data & 0x0F) == 0x0E)
                baudRate = ip5clk >> 4;
            else if ((data & 0x0F) == 0x0F) // 0x0f
                baudRate = ip5clk;
        }
        
        if ((!baudRate) && ((data & 0xF) != 0xD))
            Logger::Log(DUART_LOG_PREFIX, std::format("Invalid DUART{} channel {} transmit clock configuration {}", duart, channelId, data).c_str(), LogChannels::Warning);
        
        if (!isRx)
        {
            Logger::Log(DUART_LOG_PREFIX, std::format("DUART{} channel {}: Transmit baud rate is now {}", duart, channelId, baudRate).c_str(), LogChannels::Warning);
            channel.baudRateTX = baudRate;
        }
        else
        {
            Logger::Log(DUART_LOG_PREFIX, std::format("DUART{} channel {}: Receive baud rate is now {}", duart, channelId, baudRate).c_str(), LogChannels::Warning);
            channel.baudRateRX = baudRate;           
        }

    }

    void DUART68681::UpdateDataFrameState(int32_t duart, int32_t channel)
    {
        
    }

    void DUART68681::UpdateInterruptState(int32_t duart, int32_t channel)
    {
        
    }
}