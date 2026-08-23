/* 
    m  o  t  i  o  n
    The SGI Emulator

    Copyright (c)2026 starfrost

    multibus.cpp: The Intel Multibus bus implementation.
*/

#include <base/emulation.hpp>
#include <component/multibus/multibus.hpp>

namespace Motion
{
    void Multibus::Start()
    {
        // add the multibus state
        multibusExtension = new CoherentExtensionMultibus(this);
        Coherent::RegisterExtension(multibusExtension);

        // map ip2 segment 4 (multibus memory)

        AddrSpaceMapping mappingMultibus = AddrSpaceMapping();
        mappingMultibus.startAddr = MULTIBUS_MEMORY_START;
        mappingMultibus.endAddr = MULTIBUS_MEMORY_END;
        mappingMultibus.component = this;

        AddrSpace::AddMapping(mappingMultibus);

        // map ip2 segment 5 (multibus IO)

        AddrSpaceMapping mappingIo = AddrSpaceMapping();
        mappingIo.startAddr = MULTIBUS_IO_START;
        mappingIo.endAddr = MULTIBUS_IO_END;
        mappingIo.component = this; 

        AddrSpace::AddMapping(mappingIo);

        // map the last megabyte of physical memory, so that multibus is the exclusive provider of writes to it
        // WARNING: Very KLUDGE! BAN THIS MAN FROM WRITING EMULATORS RIGHT NOW!
        // For example if System RAM is ever not at 0x0, it will explode.

        AddrSpaceMapping* memMapping = AddrSpace::GetMapping(0);
        
        // no physical memory
        if (!memMapping->endAddr)
        {
            Logger::Log(MULTIBUS_LOG_PREFIX, "No physical memory, multibus won't work anyway, skipping rest of init", LogChannels::Warning);
            return;
        }

        AddrSpaceMapping mappingMultibusMemory = AddrSpaceMapping();

        multibusMemoryStart = memMapping->endAddr - 0x100000;
        multibusMemoryEnd = memMapping->endAddr;
        mappingMultibusMemory.startAddr = multibusMemoryStart;
        mappingMultibusMemory.endAddr = multibusMemoryEnd;
        mappingMultibusMemory.component = this; 

        memMapping->endAddr = (memMapping->endAddr); // nuke 1 megabyte of system RAM so we can redirect last-megabyte i/o here

        AddrSpace::AddMapping(mappingMultibusMemory);

        // find the memory so we can use it
        if (!memory)
            memory = Emulation::GetMachine()->FindComponentByType<Memory>();

        // guaranteed, the CPU Initialises before this.
        if (!cpu)
            cpu = Emulation::GetMachine()->FindComponentByType<ComponentCPU>();
    }

    void Multibus::FireMultibusIRQ(int32_t number)
    {
        if (number > MULTIBUS_NUM_IRQ)
        {
            Logger::Log(MULTIBUS_LOG_PREFIX, std::format("Tried to fire invalid IRQ #{}", number).c_str(), LogChannels::Warning);
            return;
        }

        cpu->SetIRQLine(number);
    }

    // is this stuff even faster 

    bool Multibus::UseCachedReadSlot(size_t addr)
    {
        if (!lastSlotRead)
            return false;

        return (addr >= lastSlotRead->ioStart
        && addr <= lastSlotRead->ioEnd);
    }

    bool Multibus::UseCachedWriteSlot(size_t addr)
    {
        if (!lastSlotWritten)
            return false;

        return ((addr >= lastSlotWritten->memStart && addr <= lastSlotWritten->memEnd) 
            || (addr >= lastSlotWritten->ioStart && addr <= lastSlotWritten->ioEnd));
    }

    bool Multibus::SetCachedReadMapping(size_t addr)
    {
        for (Multibus::SlotMapping& slot : slotMappings)
        {
            if (!slot.active)
                continue;

            if ((addr >= slot.memStart
            && addr <= slot.memEnd) 
            || (addr >= slot.ioStart
            && addr <= slot.ioEnd))
            {
                lastSlotRead = &slot;
                return true;
            }
        }

        // fail
        lastSlotRead = nullptr;
        return false; 
    }

    bool Multibus::SetCachedWriteMapping(size_t addr)
    {
        for (Multibus::SlotMapping& slot : slotMappings)
        {
            if (!slot.active)
                continue;

            if ((addr >= slot.memStart
            && addr <= slot.memEnd) 
            || (addr >= slot.ioStart
            && addr <= slot.ioEnd))
            {
                lastSlotWritten = &slot;
                return true;
            }
        }

        // fail
        lastSlotWritten = nullptr;
        return false; 
    }

    // this simulates the action of the user inserting a slot into the Multibus backplane.
    bool Multibus::AddSlotMapping(SlotMapping& slot)
    {
        slot.active = true; 

        // multibus is 24 bit
        if (slot.memStart
        && slot.memEnd)
        {
            slot.memStart = (multibusMemoryEnd - 0x100000) + (slot.memStart & 0xFFFFF);
            slot.memEnd = (multibusMemoryEnd - 0x100000) + (slot.memEnd & 0xFFFFF);
        }

        if (!slot.memStart && !slot.memEnd && !slot.ioStart && !slot.ioEnd)
        {
            Logger::Log(MULTIBUS_LOG_PREFIX, "Multibus::AddSlotMapping: At least one of I/O and Memory address range must be set for a multibus mapping!",
            LogChannels::Error);
            return false;
        }

        Logger::Log(MULTIBUS_LOG_PREFIX, "Added new multibus slot:", LogChannels::Debug);

        if (slot.memStart || slot.memEnd)
            Logger::Log(MULTIBUS_LOG_PREFIX, std::format("Memory range is {:x}-{:x}", slot.memStart, slot.memEnd).c_str(), LogChannels::Debug);
        
        if (slot.ioStart || slot.ioEnd)
            Logger::Log(MULTIBUS_LOG_PREFIX, std::format("I/O range is {:x}-{:x}", slot.ioStart, slot.ioEnd).c_str(), LogChannels::Debug);

        Logger::Log(MULTIBUS_LOG_PREFIX, std::format("Slot is {}, component is {}", slot.id + 1, slot.component->GetName()).c_str(), LogChannels::Debug);

        slotMappings.push_back(slot);

        return true; 
    }   

    uint8_t Multibus::Read8(size_t addr) 
    {
        if (!UseCachedReadSlot(addr))
            if (!SetCachedReadMapping(addr))
            {
                // for MEMORY reads, if there is no Multibus device decoding this ram, we need to send them to the memory.
                // the switch register can disable multibus
                // for IO reads on IP2 (but not on PM2 ???) it's safe to do this

                if (addr >= multibusMemoryStart
                && addr <= multibusMemoryEnd)
                {
                    return memory->Read8(addr); 
                }
                else
                {
                    // Nothing on the backplane answers, so the cycle times out and BERR is asserted.
                    // The PROM relies on this: gl2_probe decides whether a GF2 is fitted by reading the
                    // FBC flags at 0x50002400 and seeing whether it bus errors.
                    AddrSpace::SignalFault(addr, false);

                    Logger::Log(MULTIBUS_LOG_PREFIX,
                    std::format("Multibus::Read8: SetCachedReadMapping FAILED: Unmapped Multibus read from 0x{:x}", addr).c_str(),
                    LogChannels::Warning);
                    return 0x00;
                }
            }

            return lastSlotRead->component->Read8(addr);
    }

    uint16_t Multibus::Read16(size_t addr)
    {
        if (!UseCachedReadSlot(addr))
            if (!SetCachedReadMapping(addr))
            {
                // for MEMORY reads, if there is no Multibus device decoding this ram, we need to send them to the memory.
                // the switch register can disable multibus
                // for IO reads on IP2 (but not on PM2 ???) it's safe to do this

                if (addr >= multibusMemoryStart
                && addr <= multibusMemoryEnd)
                {
                    return memory->Read16(addr); 
                }
                else
                {
                    // Nothing on the backplane answers, so the cycle times out and BERR is asserted.
                    // The PROM relies on this: gl2_probe decides whether a GF2 is fitted by reading the
                    // FBC flags at 0x50002400 and seeing whether it bus errors.
                    AddrSpace::SignalFault(addr, false);

                    Logger::Log(MULTIBUS_LOG_PREFIX,
                    std::format("Multibus::Read16: SetCachedReadMapping FAILED: Unmapped Multibus read from 0x{:x}", addr).c_str(),
                    LogChannels::Warning);
                    return 0x00;
                }
            }

        return lastSlotRead->component->Read16(addr);
    }

    uint32_t Multibus::Read32(size_t addr) 
    {
        if (!UseCachedReadSlot(addr))
            if (!SetCachedReadMapping(addr))
            {
                // for MEMORY reads, if there is no Multibus device decoding this ram, we need to send them to the memory.
                // the switch register can disable multibus
                // for IO reads on IP2 (but not on PM2 ???) it's safe to do this

                if (addr >= multibusMemoryStart
                && addr <= multibusMemoryEnd)
                {
                    return memory->Read32(addr); 
                }
                else
                {
                    // Nothing on the backplane answers, so the cycle times out and BERR is asserted.
                    // The PROM relies on this: gl2_probe decides whether a GF2 is fitted by reading the
                    // FBC flags at 0x50002400 and seeing whether it bus errors.
                    AddrSpace::SignalFault(addr, false);

                    Logger::Log(MULTIBUS_LOG_PREFIX,
                    std::format("Multibus::Read32: SetCachedReadMapping FAILED: Unmapped Multibus read from 0x{:x}", addr).c_str(),
                    LogChannels::Warning);
                    return 0x00;
                }
            }

        return lastSlotRead->component->Read32(addr);
    }

    void Multibus::Write8(size_t addr, uint8_t value) 
    {
        if (!UseCachedWriteSlot(addr))
            if (!SetCachedWriteMapping(addr))
            {
                // for MEMORY reads, if there is no Multibus device decoding this ram, we need to send them to the memory.
                // the switch register can disable multibus
                // for IO reads on IP2 (but not on PM2 ???) it's safe to do this

                if (addr >= multibusMemoryStart
                && addr <= multibusMemoryEnd)
                {
                    memory->Write8(addr, value); 
                }
                else
                {
                    AddrSpace::SignalFault(addr, true);

                    Logger::Log(MULTIBUS_LOG_PREFIX,
                    std::format("Multibus::Write8: SetCachedWriteMapping FAILED: Unmapped Multibus write of 0x{:x} to 0x{:x}", value, addr).c_str(),
                    LogChannels::Warning);
                }
                return;
            }
            
        lastSlotWritten->component->Write8(addr, value);
    }

    void Multibus::Write16(size_t addr, uint16_t value)
    {
        if (!UseCachedWriteSlot(addr))
            if (!SetCachedWriteMapping(addr))
            {
                // for MEMORY reads, if there is no Multibus device decoding this ram, we need to send them to the memory.
                // the switch register can disable multibus
                // for IO reads on IP2 (but not on PM2 ???) it's safe to do this

                if (addr >= multibusMemoryStart
                && addr <= multibusMemoryEnd)
                {
                    memory->Write16(addr, value); 
                }
                else
                {
                    AddrSpace::SignalFault(addr, true);

                    Logger::Log(MULTIBUS_LOG_PREFIX,
                    std::format("Multibus::Write16: SetCachedWriteMapping FAILED: Unmapped Multibus write of 0x{:x} to 0x{:x}", value, addr).c_str(),
                    LogChannels::Warning);
                }
                return;
            }

        lastSlotWritten->component->Write16(addr, value);
    }
    
    void Multibus::Write32(size_t addr, uint32_t value)
    {
        if (!UseCachedWriteSlot(addr))
            if (!SetCachedWriteMapping(addr))
            {
                // for MEMORY reads, if there is no Multibus device decoding this ram, we need to send them to the memory.
                // the switch register can disable multibus
                // for IO reads on IP2 (but not on PM2 ???) it's safe to do this

                if (addr >= multibusMemoryStart
                && addr <= multibusMemoryEnd)
                {
                    memory->Write32(addr, value); 
                }
                else
                {
                    AddrSpace::SignalFault(addr, true);

                    Logger::Log(MULTIBUS_LOG_PREFIX,
                    std::format("Multibus::Write32: SetCachedWriteMapping FAILED: Unmapped Multibus write of 0x{:x} to 0x{:x}", value, addr).c_str(),
                    LogChannels::Warning);
                }
                return;
            }

        lastSlotWritten->component->Write32(addr, value);       
    }

    void Multibus::Shutdown()
    {
        cpu = nullptr;
        lastSlotRead = lastSlotWritten = nullptr; 
        slotMappings.clear();
    }
}; 