#pragma once
#include <Iris.hpp>

namespace Iris
{
    class Component
    {

    public: 

        /// @brief determines when this component was last ticked.
        uint64_t lastTickNs;

        /// @brief Allows an artificial delay to be implemented for less precisely timed components.
        uint64_t delayNs;

        /// @brief Returns the clock speed of the component in hertz
        /// @return Returns the clock speed of the component in hertz
        virtual uint32_t GetClockSpeed() { return 0; }; 

        virtual void Start() { };
        virtual void Tick() { };        // ticks at clock speed
        virtual void Shutdown() { };
        
        virtual uint8_t OnRead8(size_t addr) { return 0; };
        virtual uint16_t OnRead16(size_t addr) { return 0; };
        virtual uint32_t OnRead32(size_t addr) { return 0; };
        virtual void OnWrite8(size_t addr, uint8_t value) { };
        virtual void OnWrite16(size_t addr, uint16_t value) { };
        virtual void OnWrite32(size_t addr, uint32_t value) { };

        // Component type determination truth values

        /// @brief True if this component is a CPU.
        /// @return A boolean indicating if this is a CPU. Note: Type equality checks could work for this but I think this is faster than RTTI, since it runs every 
        /// tick
        virtual bool IsCPU() { return false; };

        /// @brief True if this component is a serial port.
        /// @return A boolean indicating if this is a Serial Port.
        virtual bool IsSerialPort() { return false; };

        /// @brief get the name of this component. immutable const char*.
        virtual const char* GetName() { return "Name this component by overriding Component::GetName!"; };
    };
}