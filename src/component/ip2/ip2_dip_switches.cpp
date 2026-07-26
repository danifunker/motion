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
            return (status >> 8) & 0xFF;
        else
            return (status & 0xFF00);
    };

    uint16_t IP2Switches::OnRead16(size_t addr) { return status; };

    // not sure how this behaves on real h/w, if its open bus, 0 or sign extended etc
    uint32_t IP2Switches::OnRead32(size_t addr) { return status; };
    
    void IP2Switches::OnWrite8(size_t addr, uint8_t value)
    {
        // BIG ENDIAN !!!! 
        if (addr & 0x01)
            status = (value & 0xFF00) | value;
        else
            status = (value << 8) | (value & 0xFF);
    };

    void IP2Switches::OnWrite16(size_t addr, uint16_t value) { status = value; };

    // not sure how this behaves on real h/w, if its open bus, 0 or sign extended etc
    void IP2Switches::OnWrite32(size_t addr, uint32_t value) { status = value; }; 

    //
    // COHERENT debugger extension
    //

    void CoherentExtensionIP2Switches::AddMenu()
    {
        // cond for determing checkbox state of menu
        bool cond = false;

        // bits 15:14
        if (ImGui::MenuItem("This IRIS is Slave (switch multibus area)"))
        {
            
        }

        if (ImGui::BeginMenu("Display Type [14:13]"))
        {
            ImGui::MenuItem("Non-interlaced 60Hz Monitor");
            ImGui::MenuItem("Interlaced 30Hz monitor");
            ImGui::MenuItem("NTSC Television");
            ImGui::MenuItem("PAL Television (\"bad\" - SGI)");
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("RS232 Baud Rate"))
        {
            ImGui::MenuItem("9600");
            ImGui::MenuItem("300");
            ImGui::MenuItem("1200");
            ImGui::MenuItem("19200");
            ImGui::MenuItem("600 (DO NOT USE)");
            ImGui::EndMenu();
        }

        if (ImGui::MenuItem("Boot using Secondary Display"))
        {
            
        }

        if (ImGui::MenuItem("Shut up PROM"))
        {

        }

        if (ImGui::MenuItem("Autoboot"))
        {
        }

        if (ImGui::BeginMenu("Boot Type"))
        {
            ImGui::MenuItem("[Storager 3030] HDD");
            ImGui::MenuItem("[Storager 3030] Tape");
            ImGui::MenuItem("[Storager 3030] Floppy Disk");
            ImGui::MenuItem("XNS Ethernet Netboot");
            ImGui::MenuItem("Eagle SMD HDD");
            ImGui::MenuItem("Boot to PROM Monitor");
            ImGui::EndMenu();
        }

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0, 0.0, 0.0, 1.0));
        ImGui::MenuItem("** Not all options will work **");
        ImGui::PopStyleColor();
    }
}