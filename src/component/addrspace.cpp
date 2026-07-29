#include <component/addrspace.hpp>
#include <base/emulation.hpp>

namespace Iris
{
    // yes, this checks exactly for one. becaue we don't want duplicatges and check for them 
    AddrSpaceMapping* AddrSpace::GetMapping(size_t addr) 
    { 
        // we are oging to have to optimise this    ``
        for (auto it : mappings)
        {
            if (addr >= it.second.startAddr
                && addr <= it.second.endAddr)
            {
                // guaranteed to succeed since we *know* that the start address is *always* the key. if the impl changes we'll have to change this
                // yes, it could create an implicit kv pair if we don't
                return &mappings[it.second.startAddr];
            }
        }
        
        return nullptr; 
    }

    uint8_t AddrSpace::ReadU8(size_t addr)
    {
        size_t realAddr = addr;

        if (mmu)
        {
            if (!mmu->Translate(addr, &realAddr, false))
                return 0xFF; // ****temp - add bus error ****
        }

        AddrSpaceMapping* mapping = GetMapping(realAddr);

        if (mapping)
        {
            return mapping->component->OnRead8(realAddr);
        }
        else
        {
            Logger::Log(LOG_PREFIX_MAPPING, std::format("AddrSpace::ReadU8 - Unmapped read from 0x{:x}!", addr).c_str(), LogChannels::Warning);
            return 0;
        }
    }
    
    uint16_t AddrSpace::ReadU16(size_t addr)
    {
        size_t realAddr = addr;

        if (mmu)
        {
            if (!mmu->Translate(addr, &realAddr, false))
                return 0xFF; // ****temp - add bus error ****
        }

        AddrSpaceMapping* mapping = GetMapping(realAddr);

        if (mapping)
        {
            auto value = mapping->component->OnRead16(realAddr);
            // IRIS is a big-endian system
            value = (value >> 8) | (value << 8); 

            return value;
        }
        else
        {
            Logger::Log(LOG_PREFIX_MAPPING, std::format("AddrSpace::ReadU16 - Unmapped read from 0x{:x}!", addr).c_str(), LogChannels::Warning);
            return 0;
        }
    }
    
    uint32_t AddrSpace::ReadU32(size_t addr)
    {
        size_t realAddr = addr;

        if (mmu)
        {
            if (!mmu->Translate(addr, &realAddr, false))
                return 0xFF; // ****temp - add bus error ****
        }

        AddrSpaceMapping* mapping = GetMapping(realAddr);

        if (mapping)
        {
            // IRIS is a big-endian system
            auto value = mapping->component->OnRead32(realAddr);

            // IRIS is a big-endian system.
            value = ((((value) & 0xff000000) >> 24)|
                (((value) & 0x00ff0000) >>  8) |
                (((value) & 0x0000ff00) <<  8) |
                (((value) & 0x000000ff) << 24));

            return value;
        }
        else
        {
            Logger::Log(LOG_PREFIX_MAPPING, std::format("AddrSpace::ReadU32 - Unmapped read from 0x{:x}!", addr).c_str(), LogChannels::Warning);
            return 0;
        }
    }
    
    int8_t AddrSpace::ReadS8(size_t addr)
    {
        return (int8_t)ReadU8(addr);
    }
    
    int16_t AddrSpace::ReadS16(size_t addr)
    {
        return (int16_t)ReadU16(addr);
    }
    
    int32_t AddrSpace::ReadS32(size_t addr)
    {
        return (int32_t)ReadU32(addr);
    }

    void AddrSpace::AddMapping(AddrSpaceMapping mapping)
    {
        // this seems like a good place to put this
        // paranoid assumption of the mmu location
        // BAD CODE HACK

        if (mapping.startAddr > mapping.endAddr)
        {
            Logger::Log(LOG_PREFIX_MAPPING, "AddrSpace::AddMapping - mapping.StartAddr > mapping.endAddr", LogChannels::Error);
            return;
        }

        auto mappingCount = mappings.count(mapping.startAddr);

        if (mappingCount > 1)
        {
            Logger::Log(LOG_PREFIX_MAPPING, "AddrSpace::AddMapping - mapping already exists", LogChannels::Error);
            return;
        }

        if (!mapping.component)
        {
            Logger::Log(LOG_PREFIX_MAPPING, "AddrSpace::AddMapping - mapping doesn't have an attached component!", LogChannels::Error);
            return; 
        }

        Logger::Log(LOG_PREFIX_MAPPING, std::format("Added address mapping from 0x{:x} to 0x{:x} (size 0x{:x}) for component {}",
            mapping.startAddr, mapping.endAddr, (mapping.endAddr - mapping.startAddr), mapping.component->GetName()).c_str(), LogChannels::Debug);
        
        mappings[mapping.startAddr] = mapping;
    }

    void AddrSpace::WriteU8(size_t addr, uint8_t value)
    {
        size_t realAddr = addr;

        if (mmu)
        {
            if (!mmu->Translate(addr, &realAddr, true))
                return; // ****temp - add bus error ****
        }

        AddrSpaceMapping* mapping = GetMapping(realAddr);

        if (mapping)
        {
            return mapping->component->OnWrite8(realAddr, value);
        }
        else
            Logger::Log(LOG_PREFIX_MAPPING, std::format("AddrSpace::WriteU8 - Unmapped write of 0x{:x} to 0x{:x}!", value, addr).c_str(), LogChannels::Warning);
    }

    void AddrSpace::WriteU16(size_t addr, uint16_t value)
    {
        size_t realAddr = addr;

        if (mmu)
        {
            if (!mmu->Translate(addr, &realAddr, true))
                return; // ****temp - add bus error ****
        }

        AddrSpaceMapping* mapping = GetMapping(realAddr);

        if (mapping)
        {
            // IRIS is a big-endian system
            value = (value >> 8) | (value << 8);
            return mapping->component->OnWrite16(realAddr, value);
        }
        else
            Logger::Log(LOG_PREFIX_MAPPING, std::format("AddrSpace::WriteU16 - Unmapped write of 0x{:x} to 0x{:x}!", value, addr).c_str(), LogChannels::Warning);
    }

    void AddrSpace::WriteU32(size_t addr, uint32_t value)
    {
        size_t realAddr = addr;

        if (mmu)
        {
            if (!mmu->Translate(addr, &realAddr, true))
                return; // ****temp - add bus error ****
        }

        AddrSpaceMapping* mapping = GetMapping(realAddr);

        if (mapping)
        {
            // IRIS is a big-endian system.
            value = ((((value) & 0xff000000) >> 24)|
                (((value) & 0x00ff0000) >>  8) |
                (((value) & 0x0000ff00) <<  8) |
                (((value) & 0x000000ff) << 24));

            return mapping->component->OnWrite32(realAddr, value);
        }
        else
            Logger::Log(LOG_PREFIX_MAPPING, std::format("AddrSpace::WriteU32 - Unmapped write of 0x{:x} to 0x{:x}!", value, addr).c_str(), LogChannels::Warning);
    }

    void AddrSpace::WriteS8(size_t addr, int8_t value)
    {
        WriteU8(addr, (uint8_t)value);
    }

    void AddrSpace::WriteS16(size_t addr, int16_t value)
    {
        WriteU16(addr, (uint16_t)value);
    }

    void AddrSpace::WriteS32(size_t addr, int32_t value)
    {
        WriteU32(addr, (uint32_t)value);
    }

    void AddrSpace::RegisterMMU(ComponentMMU* mmu)
    {
        Logger::Log(LOG_PREFIX_MAPPING, std::format("Addressing system registered an MMU: {}", mmu->GetName()).c_str(), LogChannels::Debug);
        AddrSpace::mmu = mmu;
    }

    void AddrSpace::Shutdown()
    {
        mappings.clear();
    }
}