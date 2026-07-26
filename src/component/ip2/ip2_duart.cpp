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
        int32_t channelId = 0;

        if (addr & 0x08)
            channelId = 1;

        UARTChannel& channel = duarts[duartId].channels[channelId];

        uint8_t ret = 0;
        uint8_t mrPtr = 0;

        bool mustUpdateFrame = false;           // true if state of the DUART data framing must be updated
        bool mustUpdateInterrupts = false;      // true if state of the interrupts must be updated

        switch (addr)
        {
            case DUART_MODE_A:
            case DUART_MODE_B:
                mrPtr = channel.modeRegCurrent;

                if (mrPtr == 0)
                {
                    ret = channel.mode1;
                    channel.modeRegCurrent++;
                }
                else
                    ret = channel.mode2;

                mustUpdateFrame = true;
                mustUpdateInterrupts = true; 
                break;
            case DUART_READ_STATUS_A:
            case DUART_READ_STATUS_B:
                ret = channel.status;
                break;
                // non standard bit rates
            case DUART_READ_BRG_TEST:
                ret = duart.brgTest;
                break;
            // FIFO read
            case DUART_READ_RX_HOLD_A:
            case DUART_READ_RX_HOLD_B:
                if (!channel.rxFifoFree)
                {
                    Logger::Log(DUART_LOG_PREFIX, std::format("DUART{} RX FIFO {} underflow...", duartId, channel).c_str());
                    mustUpdateInterrupts = true;
                    break; 
                }

                channel.rxFifoReadPtr++;

                if (channel.rxFifoReadPtr > DUART_FIFO_SIZE)
                    channel.rxFifoReadPtr = 0;

                // decrement free count
                channel.rxFifoFree--;
                ret = channel.rxFifoReadPtr;

                // Handle the fifo flags

                if (channel.rxFifoFree != DUART_FIFO_SIZE)
                    channel.status &= ~(DUART_STATUS_FIFO_FULL);

                // handle no error
                if (!(channel.mode1 & DUART_MODE_BLOCK_ERROR))
                    channel.status &= ~(DUART_STATUS_RECEIVED_BREAK | DUART_STATUS_FRAMING_ERROR | DUART_STATUS_PARITY_ERROR);

                // can't receive anything if we have not parsed anything
                if (!channel.rxFifoFree)
                    channel.status &= ~(DUART_STATUS_RECEIVER_READY);
                else 
                    channel.status |= ((channel.rxFifo[channel.rxFifoReadPtr]) >> 8); // throw in the latest fifo read 
                
                mustUpdateInterrupts = true; 

                break; 
        }

        if (mustUpdateFrame)
            UpdateDataFrameState(duartId, channelId);

        if (mustUpdateInterrupts)
            UpdateInterruptState(duartId, channelId);

        Logger::Log(DUART_LOG_PREFIX, std::format("DUART{} read8 0x{:x} from addr 0x{:x}", duartId, ret, addr).c_str(), LogChannels::Debug);

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

        // bit 4 is used for channel selection on channel regs
        int32_t channelId = 0;

        if (addr & 0x08)
            channelId = 1;

        UARTChannel& channel = duarts[duartId].channels[channelId];
 
        uint8_t ret = 0;
        uint8_t mrPtr = 0;

        bool mustUpdateFrame = false;           // true if state of the DUART data framing must be updated
        bool mustUpdateInterrupts = false;      // true if state of the interrupts must be updated

        switch (addr)
        {
            // mode reg
            case DUART_MODE_A:
            case DUART_MODE_B:
                mrPtr = channel.modeRegCurrent;

                if (mrPtr == 0)
                {
                    channel.mode1 = value;
                    channel.modeRegCurrent++;
                }
                else
                    channel.mode2 = value;

                mustUpdateFrame = true;
                mustUpdateInterrupts = true; 
                break;
            case DUART_WRITE_CLOCKSEL_A:
            case DUART_WRITE_CLOCKSEL_B:
                channel.clocksel = value;
                SetBaudRate(duartId, channelId, false, (value & 0x0F));
                SetBaudRate(duartId, channelId, true, ((value >> 4) & 0x0F));

                SetRxClock(channel.baudRateRX);
                SetTxClock(channel.baudRateTX);

                break;
            case DUART_WRITE_COMMAND_A:
            case DUART_WRITE_COMMAND_B:
                // Command register
                switch ((value >> 4) & 0x0F)
                {
                    case DUART_COMMAND_NOP:
                        break;
                    case DUART_COMMAND_RESET_MR_PTR:
                        channel.modeRegCurrent = 0;
                        break;
                    case DUART_COMMAND_RESET_CHAN_RECEIVER:
                        channel.rxEnabled = false;
                        channel.status &= ~(DUART_STATUS_RECEIVER_READY | DUART_STATUS_RECEIVED_BREAK | DUART_STATUS_FRAMING_ERROR | DUART_STATUS_PARITY_ERROR);
                        // MAME guy was not sure of this and i ran out of screen space anyway
                        channel.status &= ~(DUART_STATUS_OVERRUN_ERROR);

                        channel.rxFifoReadPtr = channel.rxFifoWritePtr = channel.rxFifoFree = 0;
                        break;


                }
                break;
        }

        if (mustUpdateFrame)
            UpdateDataFrameState(duartId, channelId);

        if (mustUpdateInterrupts)
            UpdateInterruptState(duartId, channelId);

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
            Logger::Log(DUART_LOG_PREFIX, std::format("DUART{} channel {}: Transmit baud rate is now {}", duart, channelId, baudRate).c_str(), LogChannels::Debug);
            channel.baudRateTX = baudRate;
        }
        else
        {
            Logger::Log(DUART_LOG_PREFIX, std::format("DUART{} channel {}: Receive baud rate is now {}", duart, channelId, baudRate).c_str(), LogChannels::Debug);
            channel.baudRateRX = baudRate;           
        }

    }

    void DUART68681::UpdateDataFrameState(int32_t duart, int32_t channel)
    {
        
    }

    void DUART68681::UpdateInterruptState(int32_t duart, int32_t channel)
    {
        
    }

    //
    // TICK method + CLOCK
    //

    void DUART68681::Tick()
    {
        // there's two clocks so we run our own clocks

        bool runTx = false, runRx = false;

        if (!lastRxClkNs)
            runRx = true;
        
        if (!lastTxClkNs)
            runTx = true;

        auto ns = Chrono_GetTicksNS(Chrono_GetTime());

        if ((ns - lastRxClkNs) > rxClkNs)
            runRx = true;

        if ((ns - lastTxClkNs) > txClkNs)
            runTx = true;

        if (runRx)
        {
            lastRxClkNs = ns;
            OnRxClock();
        }

        if (runTx)
        {
            lastTxClkNs = ns;
            OnTxClock();
        }
    }

    void DUART68681::OnRxClock()
    {

    }

    void DUART68681::OnTxClock()
    {

    }

    void DUART68681::SetRxClock(uint32_t hz)
    {
        double intermediate = 1.0 / hz * 1000000000;
        rxClkNs = (uint64_t)intermediate;
    }

    void DUART68681::SetTxClock(uint32_t hz)
    {
        double intermediate = 1.0 / hz * 1000000000;
        txClkNs = (uint64_t)intermediate;
    }

}