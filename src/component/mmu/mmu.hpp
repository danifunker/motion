/* 
    m  o  t  i  o  n
    The SGI Emulator

    Copyright (c)2026 starfrost

    mmu.hpp: Base MMU type
*/

#pragma once
#include <Iris.hpp>
#include <component/addrspace.hpp>
#include <component/component.hpp>

namespace Iris
{
    class ComponentMMU : public Component
    {
        virtual void BusError(uint32_t addr) { };

        bool IsMMU() override { return true; };
        
        // these methods indicate success by their return value and take a translated address
        virtual bool TranslateRead(uint32_t initialAddress, uint32_t* finalAddress) { };
        virtual bool TranslateWrite(uint32_t initialAddress, uint32_t* finalAddress) { };
    };
};