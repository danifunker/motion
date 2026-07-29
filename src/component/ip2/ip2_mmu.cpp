/*
    m  o  t  i  o  n
    The SGI Emulator

    Copyright (c)2026 starfrost

    ip2_mmu.cpp: Implements the MMU sitting on the IP2 board
*/

#include <component/ip2/ip2_mmu.hpp>

namespace Iris
{
    Cvar* logIP2MMU; 

    // all of this code should be macroised or turned into  utility functions in /platform/util

    uint8_t IP2MMU::OnRead8(size_t addr)
    {
        if (addr & 1)
            return OnRead16(addr) & 0xFF;
        else
            return OnRead16(addr) >> 8;
    }

    uint16_t IP2MMU::OnRead16(size_t addr)
    {
        uint32_t ret = 0xFF;

        switch (addr)
        {
        case REG_OS_BASE:
            ret = osBase >> 8;
            break;
        case REG_STATUS:
            ret = status;
            break;
        case REG_PARITY:
            ret = parity;
            break;
        case REG_MULTIBUS_PROTECT:
            ret = multibusProtect;
            break;
        case REG_PAGETABLE_BASE ... PAGETABLE_INDEX(PAGETABLE_MAX_PAGES):
            if (addr & 2)
                ret = pagetable[(addr - REG_PAGETABLE_BASE) >> 2] & 0x0000FFFF;
            else
                ret = (pagetable[(addr - REG_PAGETABLE_BASE) >> 2] & 0xFFFF0000) >> 16;
            break;
        case REG_TEXTDATA_BASE:
            ret = textdataBase;
            break;
        case REG_TEXTDATA_LIMIT:
            ret = textdataLimit;
            break;
        case REG_STACK_BASE:
            ret = stackBase;
            break;
        case REG_STACK_LIMIT:
            ret = stackLimit;
            break;
        default:
            Logger::Log(LOG_PREFIX_IP2MMU, std::format("Invalid IP2 MMU read at address 0x{:x}", addr).c_str(), LogChannels::Warning);
            ret = 0xFF;
            return ret;
        }

        if (logEnabled)
            Logger::Log(LOG_PREFIX_IP2MMU, std::format("IP2 MMU read 0x{:x} from address 0x{:x} (check debug window)", ret, addr).c_str(), MMU_LOG_CHANNEL_NAME);
        
        return ret;
    }

    uint32_t IP2MMU::OnRead32(size_t addr)
    {
        return (OnRead16(addr) << 16
        | OnRead16(addr + 2));
    }

    void IP2MMU::OnWrite8(size_t addr, uint8_t value)
    {
        uint16_t val = OnRead16(addr);

        // big endian
        if (addr & 1)
            val = (val | 0xFF00) & value;
        else
            val = (val | 0x00FF) & (value << 8);

        OnWrite16(addr, val);
    }

    void IP2MMU::OnWrite16(size_t addr, uint16_t value)
    {
        switch (addr)
        {
            case REG_OS_BASE:
                osBase = ((uint16_t)value << 8);
                break;
            case REG_STATUS:
                status = value;
                break;
            case REG_PARITY:
                parity = value;
                break;
            case REG_MULTIBUS_PROTECT:
                multibusProtect = value;
                break;       
            case REG_PAGETABLE_BASE ... PAGETABLE_INDEX(PAGETABLE_MAX_PAGES):
                if (addr & 2)
                    pagetable[(addr - REG_PAGETABLE_BASE) >> 2] |= value;
                else
                    pagetable[(addr - REG_PAGETABLE_BASE) >> 2] |= (value << 16);
                    break;
            case REG_TEXTDATA_BASE:
                textdataBase = value;
                break;
            case REG_TEXTDATA_LIMIT:
                textdataLimit = value;
                break;
            case REG_STACK_BASE:
                stackBase = value;
                break;
            case REG_STACK_LIMIT:
                stackLimit = value;
                break;
            default:
                Logger::Log(LOG_PREFIX_IP2MMU, std::format("Invalid IP2 MMU write at address 0x{:x}", addr).c_str(), LogChannels::Warning);
                return;
        }

        if (logEnabled)
            Logger::Log(LOG_PREFIX_IP2MMU, std::format("IP2 MMU write 0x{:x} to address 0x{:x} (check debug window)", value, addr).c_str(), MMU_LOG_CHANNEL_NAME);   
    }

    void IP2MMU::OnWrite32(size_t addr, uint32_t value)
    {
        OnWrite16(addr, value >> 16);
        OnWrite16(addr + 2, value);
    }

    //
    // The actual MMU parts
    //

    bool IP2MMU::Translate(size_t addr, size_t* finalAddress, bool isWrite)
    {
        // IN order to minimise overhead, only get the CPU once (otherwise, everything would be massively slowed down)
        // ensure it irght here
        if (!cpu)
            cpu = Emulation::GetMachine().FindComponentByType<ComponentCPU>();

        uint8_t segment = MMU_SEGMENT_GET_ID(addr);
     
        // MAME uses templates for this, which is a bit simpler, but seems a bit silly
        // since it generates several verisons of each method and is not really how the h/w does it
        // this is most likely how the real SGI TTL stuff is doing it
        uint16_t limitValue = 0, baseValue = 0; 

        uint16_t finalPageNumber = 0;

        if (segment == MMU_SEGMENT_TEXTDATA)
        {
            limitValue = textdataLimit;
            baseValue = textdataBase;
        }
        else if (segment == MMU_SEGMENT_STACK)
        {
            limitValue = stackLimit;
            baseValue = stackBase;
        }
        else if (segment == MMU_SEGMENT_KERNEL)
        {
            baseValue = osBase;
            limitValue = 0xFFFF; // ??? hwere is register ???
        }
        else
        {
            // these don't seem to use virtual memory, so just ignore them
            *finalAddress = addr;
            return true; 
        }
        
        uint16_t pageNumber = 0;
        
        // stack is mapped in reverse
        if (segment == MMU_SEGMENT_STACK)
        {
            // bits 14-10
            pageNumber = (addr & 0x7C00) ^ 0x3FFF;
            finalPageNumber = baseValue - pageNumber;
        }
        else
        {
            pageNumber = (addr & 0x7C00);
            finalPageNumber = baseValue + pageNumber;
        }

        bool limitReached = limitValue && pageNumber > limitValue;
        uint32_t page = pagetable[finalPageNumber];

        bool busError = false;

        // read
        if (!isWrite)
        {
            if (limitReached 
                || !(page & MMU_MASK_IS_PROTECTED)
                || ((page & MMU_MASK_IS_PROTECTED) == MMU_MASK_SUPERVISOR_ONLY) && !(cpu->IsPrivilegedMode()))
            {
                busError = true; 
            }

            if (!busError)
                page |= MMU_MASK_REFERENCED;
        }
        else
        {
            if (limitReached 
                || !(page & MMU_MASK_IS_PROTECTED)
                || (page & MMU_MASK_IS_PROTECTED) == MMU_MASK_READ_ONLY // cannot write to readonly 
                || ((page & MMU_MASK_IS_PROTECTED) == MMU_MASK_SUPERVISOR_ONLY) && !(cpu->IsPrivilegedMode()))
            {
                busError = true; 
            }

            if (!busError)
                page |= (MMU_MASK_REFERENCED | MMU_MASK_MODIFIED);
        }

        if (busError)
        {
            //Logger::Log(LOG_PREFIX_IP2MMU, 
            //    std::format("***** VERY BIG PROBLEM ***** Bus error @ segment {} offset {} (haven't figured out the interface yet)", 
            //    segment, (addr << 2)).c_str(), LogChannels::FatalError);

            //return false; 
        }
        
        // calculate a real physical ram address with 13...0 page adn teh bottom1 0 bits of the real address
        *finalAddress = (page & 0x3FFF) << 10 | (addr & 0x7FF);
        Logger::Log(LOG_PREFIX_IP2MMU, std::format("Translated virtual address {} to physical address {}", addr, *finalAddress).c_str(), LogChannels::Debug);
        return true; 
    }
}