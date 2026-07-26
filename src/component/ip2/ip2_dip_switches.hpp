/* 
    m  o  t  i  o  n
    The SGI Emulator

    Copyright (c)2026 starfrost

    ip2_dip_switches.hpp: Implements the system configuration DIP switches at 31800000 on the IP2 and a Coherent extension for them
*/

#pragma once
#include <coherent/coherent.hpp>

namespace Iris
{
    class CoherentExtensionIP2Switches : public CoherentExtension
    {
    public:
        CoherentExtensionIP2Switches(Component* owner) : CoherentExtension(owner) {}

        void AddMenu() override;
    };

    class IP2Switches : public Component
    {
        friend class CoherentExtensionIP2Switches;

        #define SWITCH_ADDR             31800000

        public:
            void Start() override
            {
                AddrSpaceMapping mapping = AddrSpaceMapping();

                mapping.component = this; 
                mapping.startAddr = SWITCH_ADDR;
                mapping.endAddr = SWITCH_ADDR + 1; 

                AddrSpace::AddMapping(mapping);

                switchExtension = new CoherentExtensionIP2Switches(this);
                switchExtension->SetExtensionType(CoherentExtensionType::CustomMenu);
                switchExtension->SetMenuName("System Configuration");
                Coherent::RegisterExtension(switchExtension);
            };

            uint8_t OnRead8(size_t addr) override;
            uint16_t OnRead16(size_t addr) override;
            uint32_t OnRead32(size_t addr) override;
            void OnWrite8(size_t addr, uint8_t value) override;
            void OnWrite16(size_t addr, uint16_t value) override;
            void OnWrite32(size_t addr, uint32_t value) override; 

            const char* GetName() { return "IP2 Back Panel Switches"; };
        private: 
            uint16_t status; 
            AddrSpaceMapping mapping;
            CoherentExtensionIP2Switches* switchExtension;
    };
}
