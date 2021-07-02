/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <errno.h> // for EACCES

#include <AzCore/Android/APKFileHandler.h>
#include <AzCore/Android/JNI/Object.h>
#include <AzCore/Android/Utils.h>
#include <AzCore/IO/Path/Path.h>

//Note: Switching on verbose logging will give you a lot of detailed information about what files are being read from the APK
//      but there is a likelihood it could cause logcat to terminate with a 'buffer full' error. Restarting logcat will resume logging
//      but you may lose information
#define VERBOSE_IO_LOGGING 0

#if VERBOSE_IO_LOGGING
    #define FILE_IO_LOG(...) AZ_Printf("LMBR", __VA_ARGS__)
#else
    #define FILE_IO_LOG(...)
#endif

namespace AZ
{
    namespace Android
    {
        AZ::EnvironmentVariable<APKFileHandler> APKFileHandler::s_instance;


        bool APKFileHandler::Create()
        {
            if (!s_instance)
            {
                s_instance = AZ::Environment::CreateVariable<APKFileHandler>(AZ::AzTypeInfo<APKFileHandler>::Name());
            }

            if (s_instance->IsReady()) // already created in a different module
            {
                return true;
            }

            return s_instance->Initialize();
        }

        void APKFileHandler::Destroy()
        {
            s_instance.Reset();
        }

        bool APKFileHandler::ShouldLoadFileToMemory(const char* filePath)
        {
            if (!filePath)
            {
                return false;
            }

            for (const AZStd::string& fileName : m_memFileNames)
            {
                if (strstr(filePath, fileName.c_str()))
                {
                    return true;
                }
            }

            return false;
        }

        MemoryBuffer* APKFileHandler::GetInMemoryFileBuffer(void* asset)
        {
            for (auto it = m_memFileBuffers.begin(); it != m_memFileBuffers.end(); it++)
            {
                if (it->m_asset == asset)
                {
                    return &(*it);
                }
            }

            return nullptr;
        }

        void APKFileHandler::RemoveInMemoryFileBuffer(void* asset)
        {
            for (auto it = m_memFileBuffers.begin(); it != m_memFileBuffers.end(); it++)
            {
                if (it->m_asset == asset)
                {
                    m_memFileBuffers.erase(it);
                    break;
                }
            }
        }

        FILE* APKFileHandler::Open(const char* filename, const char* mode, AZ::u64& size)
        {
            ANDROID_IO_PROFILE_SECTION_ARGS("APK Open");
            FILE* fileHandle = nullptr;

            if (mode[0] != 'w')
            {
                FILE_IO_LOG("******* Attempting to open file in APK:[%s] ", filename);

                AAsset* asset = nullptr;
                bool loadFileToMemory = Get().ShouldLoadFileToMemory(filename);
                int assetMode = loadFileToMemory ? AASSET_MODE_BUFFER : AASSET_MODE_UNKNOWN;

                asset = AAssetManager_open(Utils::GetAssetManager(), Utils::StripApkPrefix(filename).c_str(), assetMode);

                if (asset != nullptr)
                {
                    // the pointer returned by funopen will allow us to use fread, fseek etc
                    fileHandle = funopen(asset, APKFileHandler::Read, APKFileHandler::Write, APKFileHandler::Seek, APKFileHandler::Close);

                    if (loadFileToMemory)
                    {
                        MemoryBuffer buf;
                        buf.m_buffer = (char*)AAsset_getBuffer(asset);
                        buf.m_totalSize = AAsset_getLength(asset);
                        buf.m_asset = asset;
                        if (buf.m_buffer)
                        {
                            Get().m_memFileBuffers.push_back(buf);
                        }
                        else
                        {
                            AZ_Assert(false, "Failed to load %s to memory", filename)
                        }
                    }

                    // the file pointer we return from funopen can't be used to get the length of the file so we need to capture that info while we have the AAsset pointer available
                    size = static_cast<AZ::u64>(AAsset_getLength64(asset));
                    FILE_IO_LOG("File loaded successfully");
                }
                else
                {
                    FILE_IO_LOG("####### Failed to open file in APK:[%s] ", filename);
                }
            }
            return fileHandle;
        }

        int APKFileHandler::Read(void* asset, char* buffer, int size)
        {
            ANDROID_IO_PROFILE_SECTION_ARGS("APK Read");

            APKFileHandler& apkHandler = Get();

            if (apkHandler.m_numBytesToRead < size && apkHandler.m_numBytesToRead > 0)
            {
                size = apkHandler.m_numBytesToRead;
            }
            apkHandler.m_numBytesToRead -= size;

            MemoryBuffer* buf = apkHandler.GetInMemoryFileBuffer(asset);
            if (buf)
            {
                const char* tempBuf = buf->m_buffer + buf->m_offset;
                memcpy(buffer, tempBuf, size);
                return size;
            }

            return AAsset_read(static_cast<AAsset*>(asset), buffer, static_cast<size_t>(size));
        }

        int APKFileHandler::Write(void* asset, const char* buffer, int size)
        {
            return EACCES;
        }

        fpos_t APKFileHandler::Seek(void* asset, fpos_t offset, int origin)
        {
            ANDROID_IO_PROFILE_SECTION_ARGS("APK Seek");
            MemoryBuffer* buf = Get().GetInMemoryFileBuffer(asset);
            if (buf)
            {
                if (origin == SEEK_SET)
                {
                    buf->m_offset = offset;
                }
                else if (origin == SEEK_CUR)
                {
                    buf->m_offset += offset;
                }
                else if (origin == SEEK_END)
                {
                    buf->m_offset = buf->m_totalSize - offset;
                }

                if (buf->m_offset > buf->m_totalSize)
                {
                    buf->m_offset = buf->m_totalSize;
                }
                if (buf->m_offset < 0)
                {
                    buf->m_offset = 0;
                }

                return buf->m_offset;
            }

            return AAsset_seek(static_cast<AAsset*>(asset), offset, origin);
        }

        int APKFileHandler::Close(void* asset)
        {
            Get().RemoveInMemoryFileBuffer(asset);

            AAsset_close(static_cast<AAsset*>(asset));
            return 0;
        }

        int APKFileHandler::FileLength(const char* filename)
        {
            AZ::u64 size = 0;
            FILE* asset = Open(filename, "r", size);
            if (asset != nullptr)
            {
                fclose(asset);
            }
            return static_cast<int>(size);
        }

        AZ::IO::Result APKFileHandler::ParseDirectory(const char* path, FindDirsCallbackType findCallback)
        {
            ANDROID_IO_PROFILE_SECTION_ARGS("APK ParseDirectory");
            FILE_IO_LOG("********* About to search for file in [%s] ******* ", path);

            APKFileHandler& apkHandler = Get();

            DirectoryCache::const_iterator it = apkHandler.m_cachedDirectories.find(path);
            if (it == apkHandler.m_cachedDirectories.end())
            {
                // The NDK version of the Asset Manager only returns files and not directories so we must use the Java version to get all the data we need
                JNIEnv* jniEnv = JNI::GetEnv();
                if (!jniEnv)
                {
                    return AZ::IO::ResultCode::Error;
                }

                auto newDirectory = apkHandler.m_cachedDirectories.emplace(path, StringVector());
                jstring dirPath = jniEnv->NewStringUTF(path);
                jobjectArray javaFileListObject = apkHandler.m_javaInstance->InvokeStaticObjectMethod<jobjectArray>("GetFilesAndDirectoriesInPath", dirPath);
                jniEnv->DeleteLocalRef(dirPath);

                int numObjects = jniEnv->GetArrayLength(javaFileListObject);
                bool parseResults = true;

                for (int i = 0; i < numObjects; i++)
                {
                    if (!parseResults)
                    {
                        break;
                    }

                    jstring str = static_cast<jstring>(jniEnv->GetObjectArrayElement(javaFileListObject, i));
                    const char* entryName = jniEnv->GetStringUTFChars(str, 0);
                    newDirectory.first->second.push_back(StringType(entryName));

                    parseResults = findCallback(entryName);

                    jniEnv->ReleaseStringUTFChars(str, entryName);
                    jniEnv->DeleteLocalRef(str);
                }

                jniEnv->DeleteGlobalRef(javaFileListObject);
            }
            else
            {
                bool parseResults = true;

                for (int i = 0; i < it->second.size(); i++)
                {
                    if (!parseResults)
                    {
                        break;
                    }

                    parseResults = findCallback(it->second[i].c_str());
                }
            }

            return AZ::IO::ResultCode::Success;
        }

        bool APKFileHandler::IsDirectory(const char* path)
        {
            ANDROID_IO_PROFILE_SECTION_ARGS("APK IsDir");

            APKFileHandler& apkHandler = Get();

            DirectoryCache::const_iterator it = apkHandler.m_cachedDirectories.find(path);
            if (it == apkHandler.m_cachedDirectories.end())
            {
                JNIEnv* jniEnv = JNI::GetEnv();
                if (!jniEnv)
                {
                    return false;
                }

                jstring dirPath = jniEnv->NewStringUTF(path);
                jboolean isDir = apkHandler.m_javaInstance->InvokeStaticBooleanMethod("IsDirectory", dirPath);
                jniEnv->DeleteLocalRef(dirPath);

                FILE_IO_LOG("########### [%s] %s a directory ######### ", path, retVal ? "IS" : "IS NOT");

                return (isDir == JNI_TRUE);
            }
            else
            {
                return (it->second.size() > 0);
            }
        }

        bool APKFileHandler::DirectoryOrFileExists(const char* path)
        {
            ANDROID_IO_PROFILE_SECTION_ARGS("APK DirOrFileExists");

            AZ::IO::FixedMaxPath insideApkPath(Utils::StripApkPrefix(path));

            // Check for the case where the input path is equal to the APK Assets Prefix of /APK
            // In that case the directory is the "root" of APK assets in which case the directory exist
            if (insideApkPath.empty() && Utils::IsApkPath(path))
            {
                return true;
            }

            AZ::IO::FixedMaxPathString filename{ insideApkPath.Filename().Native() };
            AZ::IO::FixedMaxPathString pathToFile{ insideApkPath.ParentPath().Native() };
            bool foundFile = false;

            ParseDirectory(pathToFile.c_str(), [&](const char* name)
            {
                if (strcasecmp(name, filename.c_str()) == 0)
                {
                    foundFile = true;
                }

                return true;
            });

            FILE_IO_LOG("########### Directory or file [%s] %s exist ######### ", filename.c_str(), foundFile ? "DOES" : "DOES NOT");
            return foundFile;
        }

        void APKFileHandler::SetNumBytesToRead(const size_t numBytesToRead)
        {
            // WARNING: This isn't a thread safe way of handling this problem, LY-65478 will fix it
            APKFileHandler& apkHandler = Get();
            apkHandler.m_numBytesToRead = numBytesToRead;
        }

        void APKFileHandler::SetLoadFilesToMemory(const char* fileNames)
        {
            AZStd::string names(fileNames);
            size_t pos = 0;
            bool stringProcessed = false;
            APKFileHandler& apkHandler = Get();

            while (!stringProcessed)
            {
                size_t newPos = names.find_first_of(',', pos);
                size_t len = 0;
                if (newPos == AZStd::string::npos)
                {
                    len = newPos;
                    stringProcessed = true;
                }
                else
                {
                    len = newPos - pos;
                }
                AZStd::string fileName = names.substr(pos, len);
                pos = newPos + 1;
                apkHandler.m_memFileNames.push_back(fileName);
            }
        }


        APKFileHandler::APKFileHandler()
                : m_javaInstance()
                , m_cachedDirectories()
                , m_numBytesToRead(0)
        {
        }

        APKFileHandler::~APKFileHandler()
        {
            m_memFileBuffers.set_capacity(0);
            m_memFileNames.set_capacity(0);

            if (s_instance)
            {
                AZ_Assert(s_instance.IsOwner(), "The Android APK file handler instance is being destroyed by someone other than the owner.");
            }
        }


        APKFileHandler& APKFileHandler::Get()
        {
            if (!s_instance)
            {
                s_instance = AZ::Environment::FindVariable<APKFileHandler>(AZ::AzTypeInfo<APKFileHandler>::Name());
                AZ_Assert(s_instance, "The Android APK file handler is NOT ready for use! Call Create first!");
            }
            return *s_instance;
        }


        bool APKFileHandler::Initialize()
        {
            JniObject* apkHandler = aznew JniObject("com/amazon/lumberyard/io/APKHandler", "APKHandler");
            if (!apkHandler)
            {
                return false;
            }

            m_javaInstance.reset(apkHandler);

            m_javaInstance->RegisterStaticMethod("IsDirectory", "(Ljava/lang/String;)Z");
            m_javaInstance->RegisterStaticMethod("GetFilesAndDirectoriesInPath", "(Ljava/lang/String;)[Ljava/lang/String;");

        #if VERBOSE_IO_LOGGING
            m_javaInstance->RegisterStaticField("s_debug", "Z");
            m_javaInstance->SetStaticBooleanField("s_debug", JNI_TRUE);
        #endif

            return true;
        }

        bool APKFileHandler::IsReady() const
        {
            return (m_javaInstance != nullptr);
        }

    } // namespace Android
} // namespace AZ
