/* 
    m  o  t  i  o  n
    The SGI Emulator

    Copyright (c)2026 starfrost

    addrspace.hpp : Implements the address space mapping system and also interfaces with an optional MMU
*/

#pragma once

#include <Motion.hpp>
#include <component/component.hpp>
#include <component/mmu/mmu.hpp>

#define LOG_PREFIX_MAPPING  "Emulation - Memory Mapping"

// Everything at or above this is devices rather than RAM, so a hole in it is a bus timeout.
#define ADDRSPACE_DEVICE_SPACE_START 0x30000000

namespace Motion
{
    // This class implements an address space mapping.
    // The 
    class AddrSpaceMapping
    {
    public: 
        // The start address is used as the key in the map.
        size_t startAddr;
        size_t endAddr; 

        Component* component;
    };

    // Class implementing address space.
    // It is tied to the Machine.
    // By default each machine has
    class AddrSpace
    {
        public:
            // 16-bit - 65536; 24-bit - 16777216; 
            inline static size_t maxAddr;

            static uint8_t ReadU8(size_t addr);
            static uint16_t ReadU16(size_t addr);
            static uint32_t ReadU32(size_t addr);
            static int8_t ReadS8(size_t addr);
            static int16_t ReadS16(size_t addr);
            static int32_t ReadS32(size_t addr);
 
            static void WriteU8(size_t addr, uint8_t value);
            static void WriteU16(size_t addr, uint16_t value);
            static void WriteU32(size_t addr, uint32_t value);
            static void WriteS8(size_t addr, int8_t value);
            static void WriteS16(size_t addr, int16_t value);
            static void WriteS32(size_t addr, int32_t value);
 
            static void AddMapping(AddrSpaceMapping mapping);
            static AddrSpaceMapping* GetMapping(size_t addr);

            /*
                An MMU can detect a fault, but it can't raise the exception: only the CPU core knows how to
                build the stack frame for one. So a failed translation is recorded here, and whoever made
                the access decides what to do with it. The CPU turns it into a bus error; the debugger and
                anything else reading memory behind the machine's back can just ignore it.
            */
            /// @brief Faults are ignored until the CPU is out of reset, the same way the MMU declines to
            /// translate during reset - the reset vector fetch happens before every device has mapped itself.
            static void SetFaultsEnabled(bool enabled) { faultsEnabled = enabled; };

            static void SignalFault(size_t addr, bool isWrite);
            static void SignalFaultIfDeviceSpace(size_t addr, bool isWrite);
            static void ClearFault() { faultPending = false; };
            static bool TakeFault(size_t* addr, bool* isWrite);

            /// @brief Reigister a memory management unit
            /// @param mmu The MMU to register.
            static void RegisterMMU(ComponentMMU* mmu);

            static void Shutdown();
        private: 
            inline static std::unordered_map<size_t, AddrSpaceMapping> mappings;
            

            ///pointer to an MMU component
            inline static ComponentMMU* mmu;

            /// @brief Set when a translation fails, consumed by TakeFault.
            inline static bool faultsEnabled = false;
            inline static bool faultPending = false;
            inline static size_t faultAddress = 0;
            inline static bool faultWasWrite = false;

    };
}