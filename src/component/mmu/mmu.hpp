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
    // a specific "segment" of memory, which basically has a certain set of codes
    // everything is done with lambdas
    class MemorySegment
    {

    }; 

    class ComponentMMU : public Component
    {

    };
};