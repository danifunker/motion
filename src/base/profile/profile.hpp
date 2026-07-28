

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
        /// @brief Initialises the profile system, currently just gets the profileFolder convar.
        static void Init();

        /// @brief Builds a string with a profile folder path.
        /// @param fileName The file name to build. It will be automatically appended to the profileFolder convar.
        /// @param buf Must be at least 260 characters !!!!!
        static void GetProfileFolderPath(const char* fileName, char* buf);

        /// @brief open a file (see Filesystem::File)
        /// @param path the path to open
        /// @param mode the mode to open the file with
        /// @param skipProfileAddition if this is true, a temporary bufer of the right size will be created and passed into filesystem::open
        static FileStream* Open(const char* path, FileMode mode = FileMode::Text, bool skipProfileAddition = false);

        /// @brief this method does the same thing as Filesystem::Close.
        static void Close(FileStream* fs);
    }; 
}; 
