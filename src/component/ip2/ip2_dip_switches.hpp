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

        void AddUI() override;
    };

    class IP2Switches : public Component
    {
        friend class CoherentExtensionIP2Switches;

        #define SWITCH_ADDR                     0x31800000

        // DIp switch purposes
        // _END is used to make some things easier in the debug ui

        #define SWITCH_IS_SLAVE                 (1 << 15)
        #define SWITCH_PRIMARY_DISPLAY_TYPE     (1 << 14 | 1 << 13)         // primary display types
        #define SWITCH_PRIMARY_DISPLAY_TYPE_END 13                          // primary display types

        #define SWITCH_DISP_PROGRESSIVE         0x0                         // Progressive-scan 60hz monitor
        #define SWITCH_DISP_INTERLACED          0x1                         // Interlaced 30hz monitor
        #define SWITCH_DISP_TV_NTSC             0x2                         // NTSC Television
        #define SWITCH_DISP_TV_PAL              0x3                         // PAL Television ("bad" according to SGI)

        #define SWITCH_RS232_SPEED              (1 << 12 | 1 << 11)         // rs232 baud speed 
        #define SWITCH_RS232_SPEED_END          11         

        #define SWITCH_RS232_SPEED_9600         0x0                         // run port2 at 9600 baud
        #define SWITCH_RS232_SPEED_300          0x1                         // run port2 at 300 baud
        #define SWITCH_RS232_SPEED_1200         0x2                         // run port2 at 1200 baud
        #define SWITCH_RS232_SPEED_19200        0x3                         // run port2 at 19200 baud
        #define SWITCH_RS232_SPEED_600          0x4                         // you acnnot set this because it is only 2 bits lol

        #define SWITCH_USE_SECONDARY_DISP       (1 << 5)                    // use secondary display
        #define SWITCH_SHUTUP_PROM              (1 << 4)                    // shut up infinite prom loging
        #define SWITCH_AUTOBOOT                 (1 << 3)                    // autoboot or boot to prom
        #define SWITCH_BOOT_TYPE                (1 << 2 | 1 << 1 | 1 << 0)  // boot device
        #define SWITCH_BOOT_TYPE_END            0  

        #define SWITCH_BOOT_STORAGER_HDD        0x0                         // boot from storager ESDI hdd
        #define SWITCH_BOOT_STORAGER_TAPE       0x1                         // boot from storager ESDI tape
        #define SWITCH_BOOT_STORAGER_FLOPPY     0x2                         // boot from storager ESDI 5.25" floppy
        #define SWITCH_BOOT_STORAGER_XNS        0x3                         // Ethernet XNS Netboot
        #define SWITCH_BOOT_STORAGER_HDD_SMD    0x4                         // boot from storager ESDI hdd
        #define SWITCH_BOOT_PROM_MONITOR        0x5                         // boot to prom monitor

        // 0x0D ... 0x0F force DSD boot but idk why the user should cfg them

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

                // setup reasonable defaults

            };

            uint8_t OnRead8(size_t addr) override;
            uint16_t OnRead16(size_t addr) override;
            uint32_t OnRead32(size_t addr) override;
            void OnWrite8(size_t addr, uint8_t value) override;
            void OnWrite16(size_t addr, uint16_t value) override;
            void OnWrite32(size_t addr, uint32_t value) override; 

            const char* GetName() { return "IP2 Back Panel Switches"; };
        private: 
            uint16_t switchState; 
            AddrSpaceMapping mapping;
            CoherentExtensionIP2Switches* switchExtension;
    };
}
