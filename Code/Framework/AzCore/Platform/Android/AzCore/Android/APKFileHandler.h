/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Android/JNI/JNI.h>
#include <AzCore/Android/JNI/Object_fwd.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Module/Environment.h>
#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/string/osstring.h>


//NOTE: When running with the RAD Telemetry Gem enabled, a lot of the file IO methods will be captured for analysis.
//      However developers who want to profile performance of their game when using APK's containing assets can enable the flag
//      below to instrument their game in even more detail
#define AZ_ENABLED_VERBOSE_ANDROID_IO_PROFILING 0

#if AZ_ENABLED_VERBOSE_ANDROID_IO_PROFILING
    #include <AzCore/Debug/Profiler.h>

    #define ANDROID_IO_PROFILE_SECTION           AZ_PROFILE_FUNCTION(AzCore)
    #define ANDROID_IO_PROFILE_SECTION_ARGS(...) AZ_PROFILE_SCOPE(AzCore, __VA_ARGS__)
#else
    #define ANDROID_IO_PROFILE_SECTION
    #define ANDROID_IO_PROFILE_SECTION_ARGS(...)
#endif


namespace AZ
{
    namespace Android
    {
        struct MemoryBuffer
        {
            const char* m_buffer;
            AAsset* m_asset;
            int m_totalSize;
            int m_offset;

            MemoryBuffer()
            {
                m_offset = 0;
                m_totalSize = 0;
                m_buffer = nullptr;
                m_asset = nullptr;
            }
        };

        class APKFileHandler
        {
        public:
            AZ_TYPE_INFO(APKFileHandler, "{D16233A2-A183-40FE-8CF4-ABE8D53AB5B5}")
            AZ_CLASS_ALLOCATOR(APKFileHandler, AZ::OSAllocator);


            typedef AZStd::function<bool(const char*)> FindDirsCallbackType;


            //! The preferred entry point for the construction of the global APKFileHandler instance
            static bool Create();

            //! Public accessor to destroy the APKFileHandler global instance
            static void Destroy();


            //! Opens a file using the native assets manager and maps standard c file i/o
            //! \param filename The full path to the file
            //! \param mode Access mode of the file to be opened, will ignore write operations
            //! \param size[out] Returns the size of the file in bytes
            //! \return A standard c file handle
            static FILE* Open(const char* filename, const char* mode, AZ::u64& size);

            //! Reads \p size bytes of a given open file.  Is mapped to fread when a file is opened
            //! \param asset Raw pointer to the AAsset
            //! \param buffer Data blob to read into
            //! \param size Number of bytes to read.  When called from fread redirect, value could be ignored in favor of
            //!             using the internal cached version to ensure we are reading only the necessary number of bytes,
            //!             otherwise we would be reading more than necessary as the redirected seems to only pass in 1024.
            //! \return The number of bytes read, zero on EOF, or < 0 on error.
            static int Read(void* asset, char* buffer, int size);

            //! Writing to files inside an APK is unsupported.  Is mapped to fwrite when a file is opened in order to correctly
            //! return an access error.
            //! \return EACCES
            static int Write(void* asset, const char* buffer, int size);

            //! Same as, and is mapped to fseek when a file is opened.
            static fpos_t Seek(void* asset, fpos_t offset, int origin);

            //! Closes the file handle and frees it's allocated resources.  Is mapped to fclose when a file is opened.
            //! \return 0
            static int Close(void* asset);

            //! Get the size, in bytes, of a file
            //! \param filename The full path to the file
            //! \return The size of the file, in bytes.  Returns
            static int FileLength(const char* filename);

            //! Uses JNI to cache the contents of a given directory at \p path while providing each entry to \p findCallback
            //! \param path The full path to the desired directory
            //! \param findCallback Callback used to find a specific file within a given directory
            //! \return If the directory was already cached  \ref AZ::IO::ResultCode::Success if the directory was already cached or
            static AZ::IO::Result ParseDirectory(const char* path, FindDirsCallbackType findCallback);

            //! Check to see if a given path is a directory or not
            static bool IsDirectory(const char* path);

            //! Checks to see if a path (file or directory) exists
            static bool DirectoryOrFileExists(const char* path);

            //! Set the correct number of bytes to be read when calls to fread are redirected to \ref APKFileHandler::Read
            static void SetNumBytesToRead(const size_t numBytesToRead);

            //! Set the names of the files that should be loaded to memory
            static void SetLoadFilesToMemory(const char* fileNames);


            APKFileHandler();
            ~APKFileHandler();


        private:
            MemoryBuffer* GetInMemoryFileBuffer(void* asset);
            void RemoveInMemoryFileBuffer(void* asset);
            bool ShouldLoadFileToMemory(const char* filePath);

            typedef JNI::Internal::Object<AZ::OSAllocator> JniObject;

            typedef AZ::OSStdAllocator StdAllocatorType;
            typedef AZ::OSString StringType;

            typedef AZStd::vector<StringType, StdAllocatorType> StringVector;
            typedef AZStd::unordered_map<StringType, StringVector, AZStd::hash<StringType>, AZStd::equal_to<StringType>, StdAllocatorType> DirectoryCache;


            //! Internal accessor to the global APKFileHandler instance
            static APKFileHandler& Get();


            AZ_DISABLE_COPY_MOVE(APKFileHandler);


            bool Initialize();
            bool IsReady() const;


            static AZ::EnvironmentVariable<APKFileHandler> s_instance; //!< Reference to the global APK file handler object, created in the AndroidEnv

            AZStd::vector<MemoryBuffer> m_memFileBuffers;
            AZStd::vector<AZStd::string> m_memFileNames;


            AZStd::unique_ptr<JniObject> m_javaInstance; //!< JNI instance of the com.amazon.lumberyard.io.APKHandler Java object
            DirectoryCache m_cachedDirectories; //!< Cache of directories and their respective files already found through previous JNI calls
            size_t m_numBytesToRead; //!< Temp cache of the correct number of bytes to read when fread is called on an asset
        };
    } // namespace Android
} // namespace AZ
