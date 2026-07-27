/*
    m  o  t  i  o  n
    The SGI Emulator

    Copyright (c)2026 starfrost

    serial.cpp: Serial I/O (see serial.hpp)
*/

#include <component/serial/serial.hpp>

namespace Iris
{
    void SerialLine::SendByte(uint8_t data)
    {
        // send something to use the tx log

        txLog += (char)data;

        if (txLog.size() > SERIAL_TXLOG_MAX_SIZE)
            txLog.erase(0, SERIAL_TXLOG_PURGE_SIZE);
    }

    void SerialLine::ClearTxLog()
    {
        txLog.clear();
    }

    bool SerialLine::TryReceiveByte(uint8_t& data)
    {
        if (rxQueue.empty())
            return false;

        data = rxQueue.front();
        rxQueue.pop();
        return true;
    }

    void SerialLine::AddRxByte(uint8_t data)
    {
        rxQueue.push(data);
    }

    void SerialLine::AddRxString(const char* str)
    {
        if (!str)
            return;

        for (const char* c = str; *c; c++)
            rxQueue.push((uint8_t)*c);
    }
}
