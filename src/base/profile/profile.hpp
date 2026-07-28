

/*
    m  o  t  i  o  n
    The SGI Emulator

    Copyright (c)2026 starfrost

    profile.hpp : yet another damn static class for the fgod damn configuraiton system !!!
    this is a wrapper around filesystem that redirects specified certain specific file writes, to a folder, 
    the name of which is based on a cvar.
*/

#include <Iris.hpp>
#include <base/filesystem/filesystem.hpp>
#include <platform/formats/ini.hpp>

namespace Iris
{
    extern Cvar* profileFolder; 

    class Profile
    {
    public: 
        static void Init();

        static void GetProfileFolderPath(const char* fileName, char* buf);

        static FileStream* Open(const char* path, FileMode mode = FileMode::Text);
        static void Close(FileStream* fs);

        static void GetString(const char* key, char* buf);
    }; 
}; 
