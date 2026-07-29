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
    public: 
        bool IsMMU() override { return true; };
        
        // these methods indicate success by their return value, generate a translated address and then send it back to AddrSpace
        virtual bool Translate(size_t addr, size_t* finalAddress, bool isWrite) { return true;  };
    };
};