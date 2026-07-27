/*
    m  o  t  i  o  n
    The SGI Emulator

    Copyright (c)2026 starfrost

    event.cpp: The implementation of a little backend independent arbitrary event system
*/

#include <base/event/event.hpp>
#include <base/emulation.hpp>

namespace Iris
{
    void EventSystem::FireEvent(Event evt)
    {
        Emulation::OnEvent(evt);
    }

    // debugging aide
    const char* eventTypeToString[] = 
    {
        "Key Down",
        "Key Up",
    };
}