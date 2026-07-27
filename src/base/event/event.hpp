
/*
    m  o  t  i  o  n
    The SGI Emulator

    Copyright (c)2026 starfrost

    event.hpp: A little backend independent arbitrary event system
*/

#pragma once
#include <Iris.hpp>

namespace Iris
{
    enum EventType
    {
        KeyDown = 0,
        KeyUp = 1,
    }; 

    extern const char* eventTypeToString[];
    
    class Event
    {
    public: 
        EventType type;    
        
        Event(EventType type)
        { 
            this->type = type; 
        }
    };
    
    // some prebuilt event types
  
    class KeyDownEvent : Event
    {
        uint32_t key; 

        KeyDownEvent() : Event(EventType::KeyDown) { };
    }; 

    class KeyUpEvent : Event
    {
        uint32_t key; 

        KeyUpEvent() : Event(EventType::KeyUp) { };
    }; 
    
    class EventSystem
    {
    public:
        static void FireEvent(Event evt);
    private: 
    }; 
}; 