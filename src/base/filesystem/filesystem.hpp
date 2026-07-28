#pragma once

#include <Iris.hpp>
#include <filesystem>

//
// It's a FIlesystem API! THERE IS NO HOPE! 
// THis one just provides a very narrow thing...
//

namespace Iris
{
    class FileStream
    {
    public: 
        // the stream of the file
        std::fstream stream;
    };

    enum FileFlags
    {
        Text = 1,
        Binary = 1 << 1,
        Create = 1 << 2,
        CreateOrOpen = 1 << 3,
        LogOnFail = 1 << 4,
    }; 

    class Filesystem
    {
    public:
        static FileStream* Open(const char* path, FileFlags mode = FileFlags::Text);
        static void Seek(FileStream* fs, size_t offset);
        static void Close(FileStream* fs);

    };
}; 