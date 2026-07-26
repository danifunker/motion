#include <component/ip2/ip2_dip_switches.hpp>   

namespace Iris
{
    //
    // STATUS REG 
    //

    uint8_t IP2Switches::OnRead8(size_t addr)
    { 
        // only two addresses, lazy mode
        // BIG ENDIAN !!!! 
        if (addr & 0x01)
            return (switchState >> 8) & 0xFF;
        else
            return (switchState & 0xFF00);
    };

    uint16_t IP2Switches::OnRead16(size_t addr) { return switchState; };

    // not sure how this behaves on real h/w, if its open bus, 0 or sign extended etc
    uint32_t IP2Switches::OnRead32(size_t addr) { return switchState; };
    
    void IP2Switches::OnWrite8(size_t addr, uint8_t value)
    {
        // BIG ENDIAN !!!! 
        if (addr & 0x01)
            switchState = (value & 0xFF00) | value;
        else
            switchState = (value << 8) | (value & 0xFF);
    };

    void IP2Switches::OnWrite16(size_t addr, uint16_t value) { switchState = value; };

    // not sure how this behaves on real h/w, if its open bus, 0 or sign extended etc
    void IP2Switches::OnWrite32(size_t addr, uint32_t value) { switchState = value; }; 

    //
    // COHERENT debugger extension
    //

    void CoherentExtensionIP2Switches::AddMenu()
    {
        // cond for determing checkbox state of menu
        bool cond = false;
        IP2Switches* switches = (IP2Switches*)component;

        // bits 15:14
        cond = (switches->switchState & SWITCH_IS_SLAVE);

        if (ImGui::MenuItem("This IRIS is Slave (switch multibus area)", nullptr, &cond))
            FLIP_MASK_STATE(switches->switchState, SWITCH_IS_SLAVE);

        if (ImGui::BeginMenu("Display Type"))
        {
            cond = (switches->switchState & SWITCH_PRIMARY_DISPLAY_TYPE) == (SWITCH_DISP_PROGRESSIVE << SWITCH_PRIMARY_DISPLAY_TYPE_END);

            if (ImGui::MenuItem("Non-interlaced 60Hz Monitor", nullptr, &cond))
            {
                switches->switchState &= (~SWITCH_PRIMARY_DISPLAY_TYPE);
                FLIP_MASK_STATE(switches->switchState, (SWITCH_DISP_PROGRESSIVE << SWITCH_PRIMARY_DISPLAY_TYPE_END));
            }

            cond = (switches->switchState & SWITCH_PRIMARY_DISPLAY_TYPE) == (SWITCH_DISP_INTERLACED << SWITCH_PRIMARY_DISPLAY_TYPE_END);

            if (ImGui::MenuItem("Interlaced 30Hz monitor", nullptr, &cond))
            {
                switches->switchState &= (~SWITCH_PRIMARY_DISPLAY_TYPE);
                FLIP_MASK_STATE(switches->switchState, (SWITCH_DISP_INTERLACED << SWITCH_PRIMARY_DISPLAY_TYPE_END));

            }

            cond = (switches->switchState & SWITCH_PRIMARY_DISPLAY_TYPE) == (SWITCH_DISP_TV_NTSC << SWITCH_PRIMARY_DISPLAY_TYPE_END);

            if (ImGui::MenuItem("NTSC Television", nullptr, &cond))
            {
                switches->switchState &= (~SWITCH_PRIMARY_DISPLAY_TYPE);
                FLIP_MASK_STATE(switches->switchState, (SWITCH_DISP_TV_NTSC << SWITCH_PRIMARY_DISPLAY_TYPE_END));
            }

            cond = (switches->switchState & SWITCH_PRIMARY_DISPLAY_TYPE) == (SWITCH_DISP_TV_PAL << SWITCH_PRIMARY_DISPLAY_TYPE_END);

            if (ImGui::MenuItem("PAL Television (\"bad\" - SGI)", nullptr, &cond))
            {
                switches->switchState &= (~SWITCH_PRIMARY_DISPLAY_TYPE);
                FLIP_MASK_STATE(switches->switchState, (SWITCH_DISP_TV_PAL << SWITCH_PRIMARY_DISPLAY_TYPE_END));
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("RS232 Baud Rate"))
        {
            cond = (switches->switchState & SWITCH_RS232_SPEED) == (SWITCH_RS232_SPEED_9600 << SWITCH_RS232_SPEED_END);

            if (ImGui::MenuItem("9600", nullptr, &cond))
            {
                switches->switchState &= (~SWITCH_RS232_SPEED);
                FLIP_MASK_STATE(switches->switchState, (SWITCH_RS232_SPEED_9600 << SWITCH_RS232_SPEED_END));
            }

            cond = (switches->switchState & SWITCH_RS232_SPEED) == (SWITCH_RS232_SPEED_300 << SWITCH_RS232_SPEED_END);

            if (ImGui::MenuItem("300", nullptr, &cond))
            {
                switches->switchState &= (~SWITCH_RS232_SPEED);
                FLIP_MASK_STATE(switches->switchState, (SWITCH_RS232_SPEED_300 << SWITCH_RS232_SPEED_END));
            }

            cond = (switches->switchState & SWITCH_RS232_SPEED) == (SWITCH_RS232_SPEED_1200 << SWITCH_RS232_SPEED_END);

            if (ImGui::MenuItem("1200", nullptr, &cond))
            {
                switches->switchState &= (~SWITCH_RS232_SPEED);
                FLIP_MASK_STATE(switches->switchState, (SWITCH_RS232_SPEED_1200 << SWITCH_RS232_SPEED_END));
            }

            cond = (switches->switchState & SWITCH_RS232_SPEED) == (SWITCH_RS232_SPEED_19200 << SWITCH_RS232_SPEED_END);

            if (ImGui::MenuItem("19200", nullptr, &cond))
            {
                switches->switchState &= (~SWITCH_RS232_SPEED);
                FLIP_MASK_STATE(switches->switchState, (SWITCH_RS232_SPEED_19200 << SWITCH_RS232_SPEED_END));
            }

            // there is 600 baud but it requires this to be 0x04 which it cannot be. maybe it works if you overflow....
            ImGui::EndMenu();
        }

        cond = (switches->switchState & SWITCH_USE_SECONDARY_DISP);

        if (ImGui::MenuItem("Boot using Secondary Display", nullptr, &cond))
            FLIP_MASK_STATE(switches->switchState, SWITCH_USE_SECONDARY_DISP);

        cond = (switches->switchState & SWITCH_SHUTUP_PROM);

        if (ImGui::MenuItem("Shut up PROM", nullptr, &cond))
            FLIP_MASK_STATE(switches->switchState, SWITCH_SHUTUP_PROM);

        cond = (switches->switchState & SWITCH_AUTOBOOT);

        if (ImGui::MenuItem("Autoboot", nullptr, &cond))
            FLIP_MASK_STATE(switches->switchState, SWITCH_AUTOBOOT);

        if (ImGui::BeginMenu("Boot Type"))
        {
            cond = (switches->switchState & SWITCH_BOOT_TYPE) == SWITCH_BOOT_STORAGER_HDD;

            if (ImGui::MenuItem("[Storager 3030] HDD", nullptr, &cond))
            {
                switches->switchState &= (~SWITCH_BOOT_TYPE);
                FLIP_MASK_STATE(switches->switchState, (SWITCH_BOOT_STORAGER_HDD << SWITCH_BOOT_TYPE_END));
            }

            cond = (switches->switchState & SWITCH_BOOT_TYPE) == SWITCH_BOOT_STORAGER_TAPE;

            if (ImGui::MenuItem("[Storager 3030] Tape", nullptr, &cond))
            {
                switches->switchState &= (~SWITCH_BOOT_TYPE);
                FLIP_MASK_STATE(switches->switchState, (SWITCH_BOOT_STORAGER_TAPE << SWITCH_BOOT_TYPE_END));
            }

            cond = (switches->switchState & SWITCH_BOOT_TYPE) == SWITCH_BOOT_STORAGER_FLOPPY;

            if (ImGui::MenuItem("[Storager 3030] Floppy Disk", nullptr, &cond))
            {
                switches->switchState &= (~SWITCH_BOOT_TYPE);
                FLIP_MASK_STATE(switches->switchState, (SWITCH_BOOT_STORAGER_FLOPPY << SWITCH_BOOT_TYPE_END));
            }

            cond = (switches->switchState & SWITCH_BOOT_TYPE) == SWITCH_BOOT_STORAGER_XNS;

            if (ImGui::MenuItem("XNS Ethernet Netboot", nullptr, &cond))
            {
                switches->switchState &= (~SWITCH_BOOT_TYPE);
                FLIP_MASK_STATE(switches->switchState, (SWITCH_BOOT_STORAGER_XNS << SWITCH_BOOT_TYPE_END));
            }

            cond = (switches->switchState & SWITCH_BOOT_TYPE) == SWITCH_BOOT_STORAGER_HDD_SMD;

            if (ImGui::MenuItem("Eagle SMD HDD", nullptr, &cond))
            {
                switches->switchState &= (~SWITCH_BOOT_TYPE);
                FLIP_MASK_STATE(switches->switchState, (SWITCH_BOOT_STORAGER_HDD_SMD << SWITCH_BOOT_TYPE_END));
            }
     
            cond = (switches->switchState & SWITCH_BOOT_TYPE) == SWITCH_BOOT_PROM_MONITOR;

            if (ImGui::MenuItem("Boot to PROM Monitor", nullptr, &cond))
            {
                switches->switchState &= (~SWITCH_BOOT_TYPE);
                FLIP_MASK_STATE(switches->switchState, (SWITCH_BOOT_PROM_MONITOR << SWITCH_BOOT_TYPE_END));
            }
         
            ImGui::EndMenu();
        }
        
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0, 0.0, 0.0, 1.0));
        ImGui::MenuItem("** Not all options will work **");
        ImGui::PopStyleColor();
    }
}