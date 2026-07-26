ROMs go here

A basic guide for using the emulator. May be out of date it is always changing

Emulator Log window - shows you the log of what is happening
Coherent window - debugger
    TOP BAR:
        Pause CPU - Pause the CPU
        Reset - Reset emulation 
        Step - single step (only if paused)
        
    LEFT BAR:
        Shows register view of the 68020 CPU.

    MIDDLE BAR:
        Shows the next 30 instructions of the 68020 CPU; the currently executed instruction is highlighted in blue.

    RIGHT BAR:
        Breakpoints
            Input an address into the text box and press Add to add a breakpoint.
            Click on a breakpoint to select it (due to IMGUI weirdness, currently it doesn't show a colour while selected.)
            If you have any breakpoints selected clicking remove will remove them.
        Watchpoints
            Will show you the 32-bit value of any memory address you put in.
        Catchpoints
            Don't work yet

    Peripherals - lets you access the peripheral debuggers.
        IP2 MMU - Debug SGI's TTL MMU and view the pagetable.
        DUART - Debug the serial DUARTs.

    Style - lets you change style. The styles currently suck
    System Configuration - reconfigure the IRIS's back panel switches.

Command line:
+set - set a convar.
    logIP2MMU - Log the IP2 MMU.

Not done:
    - Reconfigurable machines
    - Debugger commands
    - Configuration
    - Most things emulation wise