/*
    m  o  t  i  o  n
    The SGI Emulator

    Copyright (c)2026 starfrost

    ip2_clock.hpp: Shims the ip2 clock regs
    For now, simple enough to be a header only situation
*/

#include <Iris.hpp>
#include <component/component.hpp>
#include <component/addrspace.hpp>

namespace Iris
{
    #define IP2_CLOCK_START                     0x34000000
    #define IP2_CLOCK_END                       0x35000000

    #define IP2_CLOCK_CTRL                      0x34000000          // clock control register
    #define IP2_CLOCK_DATA                      0x35000000          // clock data register
    
    class IP2Clock : public Component
    {
    public:
        void Start() override
        {
            AddrSpaceMapping mapping = AddrSpaceMapping();

            mapping.component = this; 
            mapping.startAddr = IP2_CLOCK_START;
            mapping.endAddr = IP2_CLOCK_END;

            AddrSpace::AddMapping(mapping);

            // setup reasonable defaults

        }

        uint8_t OnRead8(size_t addr) 
        { 
            uint8_t ret = 0xFF;

            switch (addr)
            {
                case IP2_CLOCK_CTRL:
                    ret = clockCtrl;
                    break; 
                case IP2_CLOCK_DATA:
                    ret = clockData; 
                    break;
            }

            //Logger::Log(std::format("IP2 RTC Read {:x} from 0x{:x}", ret, addr).c_str());
            return ret;
        }

        void OnWrite8(size_t addr, uint8_t value)
        {

            switch (addr)
            {
                case IP2_CLOCK_CTRL:
                    clockCtrl = value;
                    break; 
                case IP2_CLOCK_DATA:
                    clockData = value; 
                    break;
            }

            //Logger::Log(std::format("IP2 RTC write {:x} to 0x{:x}", value, addr).c_str());
        }

        const char* GetName() { return "IP2 RTC"; };
    private: 
        uint16_t clockCtrl;
        uint16_t clockData;
    };

}