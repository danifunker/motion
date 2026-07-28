#include <base/filesystem/filesystem.hpp>

//
// It's a FIlesystem API! THERE IS NO HOPE! 
// THis one just provides a very narrow thing...
//

namespace Iris
{
    FileStream* Filesystem::Open(const char* path, FileFlags mode)
    {
        FileStream* fc = new FileStream;

        auto flags = std::ios_base::in | std::ios_base::out;

        if (mode & FileFlags::Binary)
            flags |= std::ios_base::binary;

        //these should probably be exclusive
        if (mode & FileFlags::CreateOrOpen)
            flags |= std::ios_base::app;
        else if (mode & FileFlags::Create)
            flags |= std::ios_base::trunc;
        
        fc->stream.open(path, flags);
    
        if (fc->stream.bad())
        {
            delete fc;
            //todo: fileflags for optional logging

            if (mode & FileFlags::LogOnFail)
                Logger::Log(std::format("Filesystem::Open with FileFlags::LogOnFail set - Failed to Open file {}, FileFlags {}", path, mode).c_str());

            return nullptr;
        }

        // we don't have an open boolean because they are explicitly open until closed currently

        return fc; 

    }

    void Filesystem::Seek(FileStream* fs, size_t offset)
    {
        fs->stream.seekp(0, std::ios_base::beg);
    }

    void Filesystem::Close(FileStream* fc)
    {
        // YOU MUST SET TO NULLPTR!
        delete fc; 
    }
}