/*
Motion - The SGI Emulator!
Copyright (C) 2026 starfrost
*/

#include <Iris.hpp>
#include <base/emulation.hpp>
#include <base/profile/profile.hpp>
#include <iostream>

namespace Iris
{
    /// @brief Fatal error function
    /// @return Emulator shuts down after this runs
    void fatal()
    {
        Emulation::SetRunning(false);
    }

    int main(int argc, char** argv)
    {

        Logger::settings.SetAppName(APP_NAME);
        Logger::settings.SetDestinations((LogDestination)(LogDestination::Stdout | LogDestination::File));
        Logger::settings.SetFatalFunction(fatal);
        Logger::settings.SetChannelMask((LogChannels)(LogChannels::Debug | LogChannels::Message | LogChannels::Warning | LogChannels::Error | LogChannels::FatalError | LogChannels::UnsafeShutdown));
        Logger::settings.sendAnsiCodesToFile = false;
        Logger::settings.postLogMessageIgnoresAnsiCodes = true; //coherent
        Logger::Init();

        Logger::Log(APP_NAME " " APP_VERSION);
        Logger::Log(APP_SIGNON, LogChannels::Message);
        CommandLine::Parse(argc, argv);                     // parse command line
        Profile::Init();                                    // init config for user profile
        Emulation::Init();                                  // start emulation thread
        
        // run the emulation
        // todo: needs to run on its own thread

        while (Emulation::IsRunning())
            Emulation::Frame();

        Logger::Log("Shutting down...");

        // shut down the emulation
        Emulation::Shutdown();
        Logger::Shutdown();

        return EXIT_SUCCESS;
    }
}

int main(int argc, char** argv)
{
    return Iris::main(argc, argv);
}