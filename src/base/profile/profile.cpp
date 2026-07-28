#include <base/profile/profile.hpp>

namespace Iris
{
    Cvar* profileFolder; 

    void Profile::Init()
    {

    }

    void Profile::GetProfileFolderPath(const char* fileName, char* buf)
    {
        
    }

    // helper methods

    FileStream* Profile::Open(const char* path, FileMode mode )
    {
        return Filesystem::Open(path, mode);
    }

    void Profile::Close(FileStream* fs)
    {
        return Filesystem::Close(fs);
    }

    void Profile::GetString(const char* key, char* buf)
    {

    }
}

