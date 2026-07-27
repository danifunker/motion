/*
    m  o  t  i  o  n
    The SGI Emulator

    Copyright (c)2026 starfrost

    serial.hpp: Serial I/O

    This is a very basic connectionless implementation of a single serial line; it doe snot know anything about baud rate, data framing or any other 
    chip behaviour which would be specific to a UART. This just bitbangs the raw data to a queue which can be read by some otehr component.
*/

#pragma once
#include <component/component.hpp>

namespace Iris
{
    /// @brief The number of raw serial lines a ComponentSerial can expose. Sized for two DUART chips, two
    /// channels each.
    #define SERIAL_MAX_LINES        4

    #define SERIAL_LOG_PREFIX       "Emulation - Serial"

    // Cap on how much transmitted output a SerialLine remember,s for display purposes.
    #define SERIAL_TXLOG_MAX_SIZE   16384
    #define SERIAL_TXLOG_PURGE_SIZE 4096

    /// @brief A single byte oriented raw serial line.
    ///
    /// Every SerialLine also keeps a small text log of everything it has sent
    /// (see GetTxLog()), or  injected into its receive queue directly (see AddRxByte()/AddRxString()) .
    class SerialLine
    {
    public:
        /// @brief Send a single raw byte out of this line (e.g. from a UART transmitter) to whatever it's connected to.
        void SendByte(uint8_t data);

        /// @brief Try to receive a single raw byte from whatever this line is connected to (e.g. into a UART receiver).
        /// @param data the data written with the received byte if one was available.
        /// @return true if a byte was available and has been written to 'data'.
        bool TryReceiveByte(uint8_t& data);

        /// @brief Push a single byte into this line's receive queue.
        void AddRxByte(uint8_t data);

        /// @brief Convenience wrapper around AddRxByte() for a whole NULL termianted string.
        void AddRxString(const char* str);

        /// @brief Clear the transmit log
        void ClearTxLog();

        // Getters for private fields 

        /// @brief Get everything the current line has sent so far, for display purposes (e.g. the Coherent debug UI).
        std::string& GetTxLog() { return txLog; };


        // Setters for private fields

    private:

        // Everything sent out of this line ends up here too, capped to SERIAL_TXLOG_MAX_SIZE, purely so it can be
        // displayed somewhere (e.g. Coherent) even when there's no real console attached to stdio.

        // At this point i just gave up and used std::string
        std::string txLog;

        // Receive queue - fed directly via AddRxByte()/AddRxString().
        std::queue<uint8_t> rxQueue;
    };

    /// @brief Base class for components that expose one or more raw serial lines to the host (e.g. UART chips).
    /// Only handles getting bytes to/from the host chip
    class ComponentSerial : public Component
    {
    public:
        /// @brief 
        /// @return TRUE since this is a serial port (component determination truth value)
        bool IsSerialPort() override { return true; };

        /// @brief Get one of this component's raw serial lines.
        /// @param lineNum The line number to get (e.g. DUART chip/channel index).
        SerialLine& GetLine(int32_t lineNum) { return lines[lineNum % SERIAL_MAX_LINES]; };

    protected:
        SerialLine lines[SERIAL_MAX_LINES];
    };
}
