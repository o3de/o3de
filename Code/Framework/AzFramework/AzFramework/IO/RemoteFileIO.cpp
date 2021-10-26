/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Crc.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/std/algorithm.h> // for GetMin()
#include <AzCore/std/functional.h> // for function<> in the find files callback.
#include <AzCore/std/parallel/lock.h>
#ifdef REMOTEFILEIO_CACHE_FILETREE
#include <AzCore/std/string/wildcard.h>
#endif

#include <AzFramework/Asset/AssetProcessorMessages.h>
#include <AzFramework/Network/AssetProcessorConnection.h>
#include <AzFramework/IO/RemoteFileIO.h>

namespace AZ
{
    namespace IO
    {
        //////////////////////////////////////////////////////////////////////////
        [[maybe_unused]] const char* const NetworkFileIOChannel = "NetworkFileIO";
#ifndef REMOTEFILEIO_IS_NETWORKFILEIO
        [[maybe_unused]] const char* const RemoteFileIOChannel = "RemoteFileIO";
    #ifdef REMOTEFILEIO_SYNC_CHECK
        const char* const RemoteFileCacheChannel = "RemoteFileCache";
    #endif
#endif

        const size_t READ_CHUNK_SIZE = 1024 * 256;

#ifdef NETWORKFILEIO_LOG
        AZ::OSString s_IOLog;
        AZ::u64 s_fileOperation = 0;

        class LogCall
        {
        public:
            LogCall(const char* name)
                :m_name(name)
            {
                m_fileOperation = s_fileOperation++;
                s_IOLog.append(AZStd::string::format("%u Start ", m_fileOperation));
                s_IOLog.append(m_name);
                s_IOLog.append("\r\n");
            }

            void Append(const char* line)
            {
                s_IOLog.append(AZStd::string::format("%u ", m_fileOperation));
                s_IOLog.append(line);
                s_IOLog.append("\r\n");
            }

            ~LogCall()
            {
                s_IOLog.append(AZStd::string::format("%u End ", m_fileOperation));
                s_IOLog.append(m_name);
                s_IOLog.append("\r\n");
            }

            AZStd::string m_name;
            AZ::u64 m_fileOperation = 0;
        };

        #define REMOTEFILE_LOG_CALL(x) LogCall lc(x)
        #define REMOTEFILE_LOG_APPEND(x) lc.Append(x)
#else
        #define REMOTEFILE_LOG_CALL(x) {}
        #define REMOTEFILE_LOG_APPEND(x) {}

#endif


        NetworkFileIO::NetworkFileIO()
        {
            REMOTEFILE_LOG_CALL("NetworkFileIO()::NetworkFileIO()");
        }

        NetworkFileIO::~NetworkFileIO()
        {
            REMOTEFILE_LOG_CALL("NetworkFileIO()::~NetworkFileIO()");
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_remoteFilesGuard);
            while (!m_remoteFiles.empty())
            {
                HandleType fileHandle = m_remoteFiles.begin()->first;
                Close(fileHandle);
            }
        }

        Result NetworkFileIO::Open(const char* filePath, OpenMode openMode, HandleType& fileHandle)
        {
            REMOTEFILE_LOG_CALL(AZStd::string::format("NetworkFileIO()::Open(filepath=%s, openMode=%i, fileHandle=OUT)",filePath?filePath:"nullptr", openMode).c_str());
            //error checks
            if (!filePath)
            {
                REMOTEFILE_LOG_APPEND("NetworkFileIO()::Open(filePath=nullptr) return Error");
                return ResultCode::Error;
            }

            if (openMode == OpenMode::Invalid)
            {
                REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO()::Open(filepath=%s, openMode=%i, fileHandle=OUT) openMode=InvalidMode return Error", filePath, openMode).c_str());
                return ResultCode::Error;
            }

            //build a request
            uint32_t mode = static_cast<uint32_t>(openMode);
            AzFramework::AssetSystem::FileOpenRequest request(filePath, mode);
            AzFramework::AssetSystem::FileOpenResponse response;
            if (!SendRequest(request, response))
            {
                AZ_Assert(false, "NetworkFileIO()::Open(filepath=%s, openMode=%i, fileHandle=OUT) unable to send request. return Error", filePath, openMode);
                REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO()::Open(filepath=%s, openMode=%i, fileHandle=OUT) unable to send request. return Error", filePath, openMode).c_str());
                return ResultCode::Error;
            }

            ResultCode returnValue = static_cast<ResultCode>(response.m_returnCode);
            if (returnValue == ResultCode::Success)
            {
                fileHandle = response.m_fileHandle;

                //track the open file handles
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_remoteFilesGuard);

                m_remoteFiles.insert(AZStd::make_pair(fileHandle, filePath));
            }

            REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO()::Open(filepath=%s, openMode=%i, fileHandle=%u) return %s", filePath, openMode, fileHandle, returnValue == ResultCode::Success ? "Success" : "Error").c_str());
            return returnValue;
        }

        Result NetworkFileIO::Close(HandleType fileHandle)
        {
            REMOTEFILE_LOG_CALL(AZStd::string::format("NetworkFileIO()::Close(fileHandle=%u)", fileHandle).c_str());
            //build a request
            AzFramework::AssetSystem::FileCloseRequest request(fileHandle);
            if (!SendRequest(request))
            {
                AZ_Assert(false, "NetworkFileIO()::Close(fileHandle=%u) Failed to send request. return Error", fileHandle);
                REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO()::Close(fileHandle=%u) Failed to send request. return Error", fileHandle).c_str());
                return ResultCode::Error;
            }

            //clean up the handle and cache
            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_remoteFilesGuard);
                m_remoteFiles.erase(fileHandle);
            }

            REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO()::Close(fileHandle=%u) return Success", fileHandle).c_str());
            return ResultCode::Success;
        }

        Result NetworkFileIO::Tell(HandleType fileHandle, AZ::u64& offset)
        {
            REMOTEFILE_LOG_CALL(AZStd::string::format("NetworkFileIO()::Tell(fileHandle=%u, offset=OUT)", fileHandle).c_str());
            AzFramework::AssetSystem::FileTellRequest request(fileHandle);
            AzFramework::AssetSystem::FileTellResponse responce;
            if(!SendRequest(request, responce))
            {
                AZ_Assert(false, "NetworkFileIO::Tell(fileHandle=%u) Failed to send tell request. return Error", fileHandle);
                REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::Tell(fileHandle=%u) Failed to send tell request. return Error", fileHandle).c_str());
                return ResultCode::Error;
            }

            ResultCode returnValue = static_cast<ResultCode>(responce.m_resultCode);
            if (returnValue == ResultCode::Error)
            {
                AZ_TracePrintf(NetworkFileIOChannel, "NetworkFileIO::Tell(fileHandle=%u) tell request failed", fileHandle);
                REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::Tell(fileHandle=%u) tell request failed", fileHandle).c_str());
                return ResultCode::Error;
            }

            offset = responce.m_offset;
            REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::Tell(fileHandle=%u) offset=%u return Success", fileHandle, offset).c_str());
            return ResultCode::Success;
        }

        Result NetworkFileIO::Seek(HandleType fileHandle, AZ::s64 offset, SeekType type)
        {
            REMOTEFILE_LOG_CALL(AZStd::string::format("NetworkFileIO()::Seek(fileHandle=%u, offset=%i, type=%s)", fileHandle, offset, type == SeekType::SeekFromCurrent ? "SeekFromCurrent" : type == SeekType::SeekFromEnd ? "SeekFromEnd" : type == SeekType::SeekFromStart ? "SeekFromStart" : "Unknown").c_str());
            AzFramework::AssetSystem::FileSeekRequest request(fileHandle, static_cast<AZ::u32>(type), offset);
            if(!SendRequest(request))
            {
                AZ_Assert(false, "NetworkFileIO::Seek() Failed to send request, fileHandle=%u. return Error", fileHandle);
                REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::Seek(fileHandle=%u) Failed to send request. return Error", fileHandle).c_str());
                return ResultCode::Error;
            }
            REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::Seek(fileHandle=%u) return Success", fileHandle).c_str());
            return ResultCode::Success;
        }

        Result NetworkFileIO::Size(HandleType fileHandle, AZ::u64& size)
        {
            size = 0;
            REMOTEFILE_LOG_CALL(AZStd::string::format("NetworkFileIO()::Size(fileHandle=%u, size=OUT)", fileHandle).c_str());
            AZStd::string fileName;
            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_remoteFilesGuard);
                auto remoteIter = m_remoteFiles.find(fileHandle);
                if (remoteIter != m_remoteFiles.end())
                {
                    fileName = remoteIter->second.c_str();
                }
            }

            if (!fileName.empty())
            {
                return Size(fileName.c_str(), size);
            }
            REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::Size(fileHandle=%u) fileHandle not found! return Error", fileHandle).c_str());
            return ResultCode::Error;
        }

        Result NetworkFileIO::Size(const char* filePath, AZ::u64& size)
        {
            size = 0;
            REMOTEFILE_LOG_CALL(AZStd::string::format("NetworkFileIO()::Size(filePath=%s, size=OUT)", filePath?filePath:"nullptr").c_str());
            //error checks
            if (!filePath)
            {
                REMOTEFILE_LOG_APPEND("NetworkFileIO::Size(filePath=nullptr) return Error");
                return ResultCode::Error;
            }

            if (!strlen(filePath))
            {
                REMOTEFILE_LOG_APPEND("NetworkFileIO::Size(filePath=\"\") strlen(filePath)==0 return Error");
                return ResultCode::Error;
            }

            AzFramework::AssetSystem::FileSizeRequest request(filePath);
            AzFramework::AssetSystem::FileSizeResponse response;
            if (!SendRequest(request, response))
            {
                AZ_Assert(false, "NetworkFileIO::Size(filePath=%s) failed to send request. return Error", filePath);
                REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::Size(filePath=%s) failed to send request. return Error", filePath).c_str());
                return ResultCode::Error;
            }

            ResultCode returnValue = static_cast<ResultCode>(response.m_resultCode);
            if (returnValue == ResultCode::Error)
            {
                AZ_TracePrintf(NetworkFileIOChannel, "NetworkFileIO::Size(filePath=%s) size request failed. return Error", filePath);
                REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::Size(filePath=%s) size request failed. return Error", filePath).c_str());
                return ResultCode::Error;
            }

            size = response.m_size;
            REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::Size(filePath=%s) size=%u. return Success", filePath, size).c_str());
            return ResultCode::Success;
        }

        Result NetworkFileIO::Read(HandleType fileHandle, void* buffer, AZ::u64 size, bool failOnFewerThanSizeBytesRead, AZ::u64* bytesRead)
        {
            REMOTEFILE_LOG_CALL(AZStd::string::format("NetworkFileIO()::Read(filehandle=%i, buffer=OUT, size=%u, failOnFewerThanSizeBytesRead=%s, bytesRead=OUT)", fileHandle, size, failOnFewerThanSizeBytesRead ? "True" : "False").c_str());
            size_t remainingBytesToRead = size;
            AZ::u64 actualRead = 0;
            while(remainingBytesToRead)
            {
                AZ::u64 readSize = GetMin<AZ::u64>(remainingBytesToRead, READ_CHUNK_SIZE);
                AzFramework::AssetSystem::FileReadRequest request(fileHandle, readSize, false);
                AzFramework::AssetSystem::FileReadResponse response;
                if (!SendRequest(request, response))
                {
                    AZ_Assert(false, "NetworkFileIO::Read(filehandle=%i, size=%u) request failed. return Error", fileHandle, size);
                    REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::Read(filehandle=%i, size=%u) request failed. return Error", fileHandle, size).c_str());
                    return ResultCode::Error;
                }

                //note the response could be ANY size, could be less so be careful
                AZ::u64 responseDataSize = response.m_data.size();
                if (responseDataSize <= remainingBytesToRead == false)
                {
                    AZ_TracePrintf(NetworkFileIOChannel, "NetworkFileIO::Read(filehandle=%i, size=%u) responseDataSize too large!!! responseDataSize=%u <= remainingBytesToRead=%u", fileHandle, size, responseDataSize, remainingBytesToRead);
                    REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::Read(filehandle=%i, size=%u) responseDataSize too large!!! responseDataSize=%u <= remainingBytesToRead=%u", fileHandle, size, responseDataSize, remainingBytesToRead).c_str());
                }

                ResultCode returnValue = static_cast<ResultCode>(response.m_resultCode);

                //only copy as much as we can
                memcpy(buffer, response.m_data.data(), responseDataSize);
                buffer = reinterpret_cast<char*>(buffer) + responseDataSize;

                //only reduce by what should have come back
                remainingBytesToRead -= responseDataSize;

                //only record read bytes
                actualRead += responseDataSize;
                if(bytesRead)
                {
                    *bytesRead = actualRead;
                }

                //if we get an error, we only return an error if failOnFewerThanSizeBytesRead
                if(returnValue == ResultCode::Error)
                {
                    AZ_TracePrintf(NetworkFileIOChannel, "NetworkFileIO::Read: request failed, fileHandle=%u", fileHandle);
                    if(failOnFewerThanSizeBytesRead && remainingBytesToRead)
                    {
                        REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::Read(fileHandle=%u, size=%u) actualRead=%u failed On Fewer Than Size Bytes Read. return Error", fileHandle, size, actualRead).c_str());
                        return ResultCode::Error;
                    }
                    REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::Read(fileHandle=%u, size=%u) actualRead=%u return Success", fileHandle, size, actualRead).c_str());
                    return ResultCode::Success;
                }
                else if(!responseDataSize)
                {
                    break;
                }
            }

            if(failOnFewerThanSizeBytesRead && remainingBytesToRead)
            {
                REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::Read(fileHandle=%u, size=%u) actualRead=%u failed On Fewer Than Size Bytes Read. return Error", fileHandle, size, actualRead).c_str());
                return ResultCode::Error;
            }
            REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::Read(fileHandle=%u, size=%u) actualRead=%u return Success", fileHandle, size, actualRead).c_str());
            return ResultCode::Success;
        }

        Result NetworkFileIO::Write(HandleType fileHandle, const void* buffer, AZ::u64 size, AZ::u64* bytesWritten)
        {
            REMOTEFILE_LOG_CALL(AZStd::string::format("NetworkFileIO()::Write(fileHandle=%u, buffer=OUT, size=%u, bytesWritten=OUT)", fileHandle, size).c_str());
            AzFramework::AssetSystem::FileWriteRequest request(fileHandle, buffer, size);
            //always async and just return success unless bytesWritten is set then synchronous
            if (bytesWritten)
            {
                AzFramework::AssetSystem::FileWriteResponse response;
                if (!SendRequest(request, response))
                {
                    AZ_Assert(false, "NetworkFileIO::Write(fileHandle=%u, size=%u) failed to send sync write request. return Error", fileHandle, size);
                    REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::Write(fileHandle=%u, size=%u) failed to send sync write request. return Error", fileHandle, size).c_str());
                    return ResultCode::Error;
                }

                if (static_cast<ResultCode>(response.m_resultCode) == ResultCode::Error)
                {
                    AZ_TracePrintf(NetworkFileIOChannel, "NetworkFileIO::Write(fileHandle=%u, size=%u) sync write request failed. return Error", fileHandle, size);
                    REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::Write(fileHandle=%u, size=%u) sync write request failed. return Error", fileHandle, size).c_str());
                    return ResultCode::Error;
                }

                *bytesWritten = response.m_bytesWritten;
                REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::Write(fileHandle=%u, size=%u) actualWrite=%u return Success", fileHandle, size, response.m_bytesWritten).c_str());
                return ResultCode::Success;
            }
            else
            {
                // just send the message and assume we wrote it all successfully.
                if (!SendRequest(request))
                {
                    AZ_Assert(false, "NetworkFileIO::Write(fileHandle=%u, size=%u) failed to send async write request. return Error", fileHandle, size);
                    REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::Write(fileHandle=%u, size=%u) failed to send async write request. return Error", fileHandle, size).c_str());
                    return ResultCode::Error;
                }
                REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::Write(fileHandle=%u, size=%u) return Success", fileHandle).c_str());
                return ResultCode::Success;
            }
        }

        Result NetworkFileIO::Flush(HandleType fileHandle)
        {
            REMOTEFILE_LOG_CALL(AZStd::string::format("NetworkFileIO()::Flush(fileHandle=%u)", fileHandle).c_str());
            // just send the message, no need to wait for flush response.
            AzFramework::AssetSystem::FileFlushRequest request(fileHandle);
            if (!SendRequest(request))
            {
                AZ_Assert(false, "NetworkFileIO::Flush(fileHandle=%u) failed to send request. return Error", fileHandle);
                REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::Flush(fileHandle=%u) failed to send request. return Error", fileHandle).c_str());
                return ResultCode::Error;
            }

            REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::Flush(fileHandle=%u) return Success", fileHandle).c_str());
            return ResultCode::Success;
        }

        bool NetworkFileIO::Eof(HandleType fileHandle)
        {
            REMOTEFILE_LOG_CALL(AZStd::string::format("NetworkFileIO()::Eof(fileHandle=%u)", fileHandle).c_str());
            AZ::u64 sizeValue = 0;
            Result res = Size(fileHandle, sizeValue);
            AZ::u64 offset = 0;
            res = Tell(fileHandle, offset);

            REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::Eof(fileHandle=%u) return %s", fileHandle, offset >= sizeValue ? "True" : "False").c_str());
            return offset >= sizeValue;
        }

        bool NetworkFileIO::Exists(const char* filePath)
        {
            REMOTEFILE_LOG_CALL(AZStd::string::format("NetworkFileIO()::Exists(filePath=%s)", filePath?filePath:"nullptr").c_str());
            //error checks
            if (!filePath)
            {
                REMOTEFILE_LOG_APPEND("NetworkFileIO::Exists(filePath=nullptr) return False");
                return false;
            }

            if (!strlen(filePath))
            {
                REMOTEFILE_LOG_APPEND("NetworkFileIO::Exists(filePath=\"\") strlen(filePath)==0. return False");
                return false;
            }

            AzFramework::AssetSystem::FileExistsRequest request(filePath);
            AzFramework::AssetSystem::FileExistsResponse response;
            if (!SendRequest(request, response))
            {
                AZ_Assert(false, "NetworkFileIO::Exists(filePath=%s) failed to send request. return False", filePath);
                REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::Exists(filePath=%s) failed to send request. return False", filePath).c_str());
                return false;
            }

            REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::Exists(filePath=%s) return %s", filePath, response.m_exists ? "True" : "False").c_str());
            return response.m_exists;
        }

        AZ::u64 NetworkFileIO::ModificationTime(const char* filePath)
        {
            REMOTEFILE_LOG_CALL(AZStd::string::format("NetworkFileIO()::ModificationTime(filePath=%s)", filePath?filePath:"nullptr").c_str());
            //error checks
            if (!filePath)
            {
                REMOTEFILE_LOG_APPEND("NetworkFileIO::ModificationTime(filePath=nullptr) return 0");
                return 0;
            }

            if (!strlen(filePath))
            {
                REMOTEFILE_LOG_APPEND("NetworkFileIO::ModificationTime(filePath=\"\") strlen(filePath)==0. return 0");
                return 0;
            }

            AzFramework::AssetSystem::FileModTimeRequest request(filePath);
            AzFramework::AssetSystem::FileModTimeResponse response;
            if (!SendRequest(request, response))
            {
                AZ_Assert(false, "NetworkFileIO::ModificationTime(filePath=%s) failed to send request. return 0", filePath);
                REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::ModificationTime(filePath=%s) failed to send request. return 0", filePath).c_str());
                return 0;
            }
            REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::ModificationTime(filePath=%s) return %i", filePath, response.m_modTime).c_str());
            return response.m_modTime;
        }

        AZ::u64 NetworkFileIO::ModificationTime(HandleType fileHandle)
        {
            REMOTEFILE_LOG_CALL(AZStd::string::format("NetworkFileIO()::ModificationTime(fileHandle=%u)", fileHandle).c_str());
            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_remoteFilesGuard);
                auto remoteIter = m_remoteFiles.find(fileHandle);
                if (remoteIter != m_remoteFiles.end())
                {
                    return ModificationTime(remoteIter->second.c_str());
                }
            }
            REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::ModificationTime(fileHandle=%u) return 0!", fileHandle).c_str());
            return 0;
        }

        bool NetworkFileIO::IsDirectory(const char* filePath)
        {
            REMOTEFILE_LOG_CALL(AZStd::string::format("NetworkFileIO()::IsDirectory(filePath=%s)", filePath?filePath:"nullptr").c_str());
            //error checks
            if (!filePath)
            {
                REMOTEFILE_LOG_APPEND("NetworkFileIO::IsDirectory(filePath=nullptr) filePath=nullptr. return False");
                return false;
            }

            if (!strlen(filePath))
            {
                REMOTEFILE_LOG_APPEND("NetworkFileIO::IsDirectory(filePath=\"\") strlen(filePath)=0. return False");
                return false;
            }

            AzFramework::AssetSystem::PathIsDirectoryRequest request(filePath);
            AzFramework::AssetSystem::PathIsDirectoryResponse response;
            if (!SendRequest(request, response))
            {
                AZ_Assert(false, "NetworkFileIO::IsDirectory(filePath=%s) failed to send request. return False", filePath);
                REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::IsDirectory(filePath=%s) failed to send request. return False", filePath).c_str());
                return false;
            }
            REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::IsDirectory(filePath=%s) return %s", filePath, response.m_isDir ? "True": "False").c_str());
            return response.m_isDir;
        }

        bool NetworkFileIO::IsReadOnly(const char* filePath)
        {
            REMOTEFILE_LOG_CALL(AZStd::string::format("NetworkFileIO()::IsReadOnly(filePath=%s)", filePath?filePath:"nullptr").c_str());
            //error checks
            if (!filePath)
            {
                REMOTEFILE_LOG_APPEND("NetworkFileIO::IsReadOnly(filePath=nullptr) return False");
                return false;
            }

            if (!strlen(filePath))
            {
                REMOTEFILE_LOG_APPEND("NetworkFileIO::IsReadOnly(filePath=\"\") strlen(filePath)==0. return False");
                return false;
            }

            AzFramework::AssetSystem::FileIsReadOnlyRequest request(filePath);
            AzFramework::AssetSystem::FileIsReadOnlyResponse response;
            if (!SendRequest(request, response))
            {
                AZ_Assert(false, "NetworkFileIO::IsReadOnly(filePath=%s) failed to send request. return False", filePath);
                REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::IsReadOnly(filePath=%s) failed to send request. return False", filePath).c_str());
                return false;
            }

            REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::IsReadOnly(filePath=%s) return %s", filePath, response.m_isReadOnly ? "True" : "False").c_str());
            return response.m_isReadOnly;
        }

        Result NetworkFileIO::CreatePath(const char* filePath)
        {
            REMOTEFILE_LOG_CALL(AZStd::string::format("NetworkFileIO()::CreatePath(filePath=%s)", filePath?filePath:"nullptr").c_str());
            //error checks
            if (!filePath)
            {
                REMOTEFILE_LOG_APPEND("NetworkFileIO::CreatePath(filePath=nullptr) filePath=nullptr. return Error");
                return ResultCode::Error;
            }

            if (!strlen(filePath))
            {
                REMOTEFILE_LOG_APPEND("NetworkFileIO::CreatePath(filePath=\"\") strlen(filePath)==0. return Error");
                return ResultCode::Error;
            }

            AzFramework::AssetSystem::PathCreateRequest request(filePath);
            AzFramework::AssetSystem::PathCreateResponse response;
            if (!SendRequest(request, response))
            {
                AZ_Assert(false, "NetworkFileIO::CreatePath(filePath=%s) failed to send request. return Error", filePath);
                REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::CreatePath(filePath=%s) failed to send request. return Error", filePath).c_str());
                return ResultCode::Error;
            }

            ResultCode returnValue = static_cast<ResultCode>(response.m_resultCode);
            REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::CreatePath(filePath=%s) return %s", filePath, returnValue == ResultCode::Success ? "Success" : "Error").c_str());
            return returnValue;
        }

        Result NetworkFileIO::DestroyPath(const char* filePath)
        {
            REMOTEFILE_LOG_CALL(AZStd::string::format("NetworkFileIO()::DestroyPath(filePath=%s)", filePath?filePath:"nullptr").c_str());
            //error checks
            if (!filePath)
            {
                REMOTEFILE_LOG_APPEND("NetworkFileIO::DestroyPath(filePath=nullptr) filePth=nullptr. return Error");
                return ResultCode::Error;
            }

            if (!strlen(filePath))
            {
                REMOTEFILE_LOG_APPEND("NetworkFileIO::DestroyPath(filePath=\"\") strlen(filePath)==0. return Error");
                return ResultCode::Error;
            }

            AzFramework::AssetSystem::PathDestroyRequest request(filePath);
            AzFramework::AssetSystem::PathDestroyResponse response;
            if (!SendRequest(request, response))
            {
                AZ_Assert(false, "NetworkFileIO::DestroyPath(filePath=%s) failed to send request. return Error", filePath);
                REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::DestroyPath(filePath=%s) failed to send request. return Error", filePath).c_str());
                return ResultCode::Error;
            }

            ResultCode returnValue = static_cast<ResultCode>(response.m_resultCode);
            REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::DestroyPath(filePath=%s) return %s", filePath, returnValue == ResultCode::Success ? "Success" : "Error").c_str());
            return returnValue;
        }

        Result NetworkFileIO::Remove(const char* filePath)
        {
            REMOTEFILE_LOG_CALL(AZStd::string::format("NetworkFileIO()::Remove(filePath=%s)", filePath?filePath:"nullptr").c_str());
            //error checks
            if (!filePath)
            {
                REMOTEFILE_LOG_APPEND("NetworkFileIO::Remove(filePath=nullptr) filePth=nullptr. return Error");
                return ResultCode::Error;
            }

            if (!strlen(filePath))
            {
                REMOTEFILE_LOG_APPEND("NetworkFileIO::Remove(filePath=\"\") strlen(filePath)==0. return Error");
                return ResultCode::Error;
            }

            AzFramework::AssetSystem::FileRemoveRequest request(filePath);
            AzFramework::AssetSystem::FileRemoveResponse response;
            if (!SendRequest(request, response))
            {
                AZ_Assert(false, "NetworkFileIO::Remove(filePath=%s) failed to send request. return Error", filePath);
                REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::Remove(filePath=%s) failed to send request. return Error", filePath).c_str());
                return ResultCode::Error;
            }

            ResultCode returnValue = static_cast<ResultCode>(response.m_resultCode);
            REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::Remove(filePath=%s) return %s", filePath, returnValue == ResultCode::Success ? "Success" : "Error").c_str());
            return returnValue;
        }

        Result NetworkFileIO::Copy(const char* sourceFilePath, const char* destinationFilePath)
        {
            REMOTEFILE_LOG_CALL(AZStd::string::format("NetworkFileIO()::Copy(sourceFilePath=%s, destinationFilePath=%s)", sourceFilePath?sourceFilePath:"nullptr", destinationFilePath?destinationFilePath:"nullptr").c_str());
            //error checks
            if (!sourceFilePath)
            {
                REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::Copy(sourceFilePath=nullptr, destinationFilePath=%s) return Error", destinationFilePath?destinationFilePath:"nullptr").c_str());
                return ResultCode::Error;
            }

            if (!destinationFilePath)
            {
                REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::Copy(sourceFilePath=%s, destinationFilePath=nullptr) return Error", sourceFilePath?sourceFilePath:"nullptr").c_str());
                return ResultCode::Error;
            }

            if (!strlen(sourceFilePath))
            {
                REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::Copy(sourceFilePath=\"\", destinationFilePath=%s) strlen(sourceFilePath)==0. return Error", destinationFilePath?destinationFilePath:"nullptr").c_str());
                return ResultCode::Error;
            }

            //fail if the source doesn't exist
            if (!Exists(sourceFilePath))
            {
                REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::Copy(sourceFilePath=%s, destinationFilePath=%s) Exists(sourceFilePath=%s)==False. return Error", sourceFilePath, destinationFilePath, sourceFilePath).c_str());
                return ResultCode::Error;
            }


            //else both are remote so just issue the remote copy command
            AzFramework::AssetSystem::FileCopyRequest request(sourceFilePath, destinationFilePath);
            AzFramework::AssetSystem::FileCopyResponse response;
            if (!SendRequest(request, response))
            {
                AZ_Assert(false, "NetworkFileIO::Copy(sourceFilePath=%s, destinationFilePath=%s) failed to send request. return Error", sourceFilePath, destinationFilePath);
                REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::Copy() failed to send request for %s -> %s. return Error", sourceFilePath, destinationFilePath).c_str());
                return ResultCode::Error;
            }

            ResultCode returnValue = static_cast<ResultCode>(response.m_resultCode);
            REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::Copy(sourceFilePath=%s, destinationFilePath=%s) return %s", sourceFilePath, destinationFilePath, returnValue == ResultCode::Success ? "Success" : "Error").c_str());
            return returnValue;
        }

        Result NetworkFileIO::Rename(const char* sourceFilePath, const char* destinationFilePath)
        {
            REMOTEFILE_LOG_CALL(AZStd::string::format("NetworkFileIO()::Rename(sourceFilePath=%s, destinationFilePath=%s)", sourceFilePath?sourceFilePath:"nullptr", destinationFilePath?destinationFilePath:"nullptr").c_str());
            //error checks
            if (!sourceFilePath)
            {
                REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::Rename(sourceFilePath=nullptr, destinationFilePath=%s) sourceFilePath=nullptr. return Error", destinationFilePath?destinationFilePath:"nullptr").c_str());
                return ResultCode::Error;
            }

            if (!destinationFilePath)
            {
                REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::Rename(sourceFilePath=%s, destinationFilePath=nullptr) destinationFilePath=nullptr. return Error", sourceFilePath).c_str());
                return ResultCode::Error;
            }

            if (!strlen(sourceFilePath))
            {
                REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::Rename(sourceFilePath=\"\", destinationFilePath=%s) strlen(sourceFilePath)==0. return Error", destinationFilePath).c_str());
                return ResultCode::Error;
            }

            //fail if the source doesn't exist
            if (!Exists(sourceFilePath))
            {
                REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::Rename(sourceFilePath=%s, destinationFilePath=%s) Exists(sourceFilePath)=False. return Error", sourceFilePath, destinationFilePath).c_str());
                return ResultCode::Error;
            }

            if (!strlen(destinationFilePath))
            {
                REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::Rename(sourceFilePath=%s, destinationFilePath=\"\") strlen(destinationFilePath)==0. return Error", sourceFilePath).c_str());
                return ResultCode::Error;
            }

            //we are going to access shared memory so lock and copy the results into our memory

            //if the source and destination are the same, shortcut
            if (!strcmp(sourceFilePath, destinationFilePath))
            {
                REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::Rename(sourceFilePath=%s, destinationFilePath=%s) sourceFilePath=destinationFilePath. return Error", sourceFilePath, destinationFilePath).c_str());
                return ResultCode::Error;
            }

            //if the destination exists
            if (Exists(destinationFilePath))
            {
                //if its read only fail
                if (IsReadOnly(destinationFilePath))
                {
                    REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::Rename(sourceFilePath=%s, destinationFilePath=%s) IsReadOnly(destinationFilePath)=True. return Error", sourceFilePath, destinationFilePath).c_str());
                    return ResultCode::Error;
                }
            }

            //else both are remote so just issue the remote command
            AzFramework::AssetSystem::FileRenameRequest request(sourceFilePath, destinationFilePath);
            AzFramework::AssetSystem::FileRenameResponse response;
            if (!SendRequest(request, response))
            {
                AZ_Assert(false, "NetworkFileIO::Rename(sourceFilePath=%s, destinationFilePath=%s) failed to send request. return Error", sourceFilePath, destinationFilePath);
                REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::Rename(sourceFilePath=%s, destinationFilePath=%s) failed to send request. return Error", sourceFilePath, destinationFilePath).c_str());
                return ResultCode::Error;
            }

            ResultCode returnValue = static_cast<ResultCode>(response.m_resultCode);
            REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::Rename(sourceFilePath=%s, destinationFilePath=%s) return %s", sourceFilePath, destinationFilePath, returnValue == ResultCode::Success ? "Success" : "Error").c_str());
            return returnValue;
        }

        Result NetworkFileIO::FindFiles(const char* filePath, const char* filter, FindFilesCallbackType callback)
        {
            REMOTEFILE_LOG_CALL(AZStd::string::format("NetworkFileIO()::FindFiles(filePath=%s, filter=%s, callback=OUT)", filePath?filePath:"nullptr", filter?filter:"nullptr").c_str());
            //error checks
            if (!filePath)
            {
                REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::FindFiles(filePath=nullptr, filter=%s) filePath=nullptr. return Error", filter?filter:"nulltpr").c_str());
                return ResultCode::Error;
            }

            if (!filter)
            {
                REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::FindFiles(filePath=%s, filter=nullptr) filter=nullptr. return Error", filePath).c_str());
                return ResultCode::Error;
            }

            if (!strlen(filePath))
            {
                REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::FindFiles(filePath=\"\", filter=%s) strlen(filePath)==0. return Error", filter).c_str());
                return ResultCode::Error;
            }

            AzFramework::AssetSystem::FindFilesRequest request(filePath, filter);
            AzFramework::AssetSystem::FindFilesResponse response;
            if (!SendRequest(request, response))
            {
                AZ_Assert(false, "NetworkFileIO::FindFiles(filePath=%s, filter=%s) could not send request. return Error", filePath, filter);
                REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::FindFiles(filePath=%s, filter=%s) could not send request. return Error", filePath, filter).c_str());
                return ResultCode::Error;
            }

            ResultCode returnValue = static_cast<ResultCode>(response.m_resultCode);
            if (returnValue == ResultCode::Success)
            {
                // callbacks
                const uint64_t numFiles = response.m_files.size();
                for (uint64_t fileIdx = 0; fileIdx < numFiles; ++fileIdx)
                {
                    const char* fileName = response.m_files[fileIdx].c_str();
                    if (!callback(fileName))
                    {
                        fileIdx = numFiles;//we are done
                    }
                }
            }
            REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::FindFiles(filePath=%s, filter=%s) return %s", filePath, filter, returnValue == ResultCode::Success ? "Success" : "Error").c_str());
            return returnValue;
        }

        void NetworkFileIO::SetAlias([[maybe_unused]] const char* alias, [[maybe_unused]] const char* path)
        {
            REMOTEFILE_LOG_CALL(AZStd::string::format("NetworkFileIO()::SetAlias(alias=%s, path=%s)", alias?alias:"nullptr", path?path:"nullptr").c_str());
        }

        const char* NetworkFileIO::GetAlias([[maybe_unused]] const char* alias) const
        {
            REMOTEFILE_LOG_CALL(AZStd::string::format("NetworkFileIO()::GetAlias(alias=%s)", alias?alias:"nullptr").c_str());
            REMOTEFILE_LOG_APPEND("NetworkFileIO::GetAlias() return nullptr");
            return nullptr;
        }

        void NetworkFileIO::ClearAlias([[maybe_unused]] const char* alias)
        {
            REMOTEFILE_LOG_CALL(AZStd::string::format("NetworkFileIO()::ClearAlias(alias=%s)", alias?alias:"nullptr").c_str());
        }

        void NetworkFileIO::SetDeprecatedAlias([[maybe_unused]] AZStd::string_view oldAlias, [[maybe_unused]] AZStd::string_view newAlias)
        {
            REMOTEFILE_LOG_CALL(AZStd::string::format("NetworkFileIO()::SetDeprecatedAlias(oldAlias=%.*s, newAlias=%.*s)",
                AZ_STRING_ARG(oldAlias), AZ_STRING_ARG(newAlias)).c_str());
        }

        AZStd::optional<AZ::u64> NetworkFileIO::ConvertToAlias(char* inOutBuffer, [[maybe_unused]] AZ::u64 bufferLength) const
        {
            REMOTEFILE_LOG_CALL(AZStd::string::format("NetworkFileIO()::ConvertToAlias(inOutBuffer=%s, bufferLength=%u)", inOutBuffer?inOutBuffer:"nullptr", bufferLength).c_str());
            REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::ConvertToAlias() return %u", strlen(inOutBuffer)).c_str());
            return strlen(inOutBuffer);
        }

        bool NetworkFileIO::ConvertToAlias([[maybe_unused]] AZ::IO::FixedMaxPath& convertedPath, [[maybe_unused]] const AZ::IO::PathView& path) const
        {
            REMOTEFILE_LOG_CALL(AZStd::string::format("NetworkFileIO()::ConvertToAlias(path=%.*s)",
                aznumeric_cast<int>(path.Native().size()), path.Native().data()).c_str());
            REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::ConvertToAlias() return false", convertedPath.Native().size()).c_str());
            return false;
        }

        bool NetworkFileIO::ResolvePath([[maybe_unused]] const char* path, [[maybe_unused]] char* resolvedPath, [[maybe_unused]] AZ::u64 resolvedPathSize) const
        {
            REMOTEFILE_LOG_CALL(AZStd::string::format("NetworkFileIO()::ResolvePath(path=%s, resolvedPath=%s, resolvedPathsize=%u)", path?path:"nullptr", resolvedPath?resolvedPath:"nullptr", resolvedPathSize).c_str());
            REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::ResolvePath(path=%s, resolvedPath=%s, resolvedPathsize=%u) return False", path?path:"nullptr", resolvedPath?resolvedPath:"nullptr", resolvedPathSize).c_str());
            return false;
        }

        bool NetworkFileIO::ResolvePath([[maybe_unused]] AZ::IO::FixedMaxPath& resolvedPath, [[maybe_unused]] const AZ::IO::PathView& path) const
        {
            REMOTEFILE_LOG_CALL(AZStd::string::format("NetworkFileIO()::ResolvePath(path=%.*s)",
                aznumeric_cast<int>(path.Native().size()), path.Native.data()).c_str());
            REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::ResolvePath(path=%.*s) return false",
                aznumeric_cast<int>(path.Native().size()), path.Native().data()).c_str());
            return false;
        }

        bool NetworkFileIO::ReplaceAlias([[maybe_unused]] AZ::IO::FixedMaxPath& replaceAliasPath, [[maybe_unused]] const AZ::IO::PathView& path) const
        {
            REMOTEFILE_LOG_CALL(AZStd::string::format("NetworkFileIO()::ReplaceAlias(path=%.*s)",
                aznumeric_cast<int>(path.Native().size()), path.Native.data()).c_str());
            REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::ReplaceAlias(path=%.*s) return false",
                aznumeric_cast<int>(path.Native().size()), path.Native().data()).c_str());
            return false;
        }

        bool NetworkFileIO::GetFilename(HandleType fileHandle, char* filename, AZ::u64 filenameSize) const
        {
            REMOTEFILE_LOG_CALL(AZStd::string::format("NetworkFileIO()::GetFilename(fileHandle=%u, filename=%s, filenamesize=%u)", fileHandle, filename?filename:"nullptr", filenameSize).c_str());
            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_remoteFilesGuard);
                const auto fileIt = m_remoteFiles.find(fileHandle);
                if (fileIt != m_remoteFiles.end())
                {
                    if (filenameSize >= fileIt->second.length())
                    {
                        azstrncpy(filename, filenameSize, fileIt->second.c_str(), fileIt->second.length());
                        REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO()::GetFilename(fileHandle=%u, filename=%s, filenamesize=%u) return True", fileHandle, filename?filename:"nullptr", filenameSize).c_str());
                        return true;
                    }
                    else
                    {
                        AZ_TracePrintf(NetworkFileIOChannel, "NetworkFileIO::GetFilename(fileHandle=%u, filename=%s, filenamesize=%u) Result buffer is too small %u", fileHandle, filename?filename:"nullptr", filenameSize, fileIt->second.length());
                        REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO::GetFilename(fileHandle=%u, filename=%s, filenamesize=%u) Result buffer is too small %u", fileHandle, filename?filename:"nullptr", filenameSize, fileIt->second.length()).c_str());
                    }
                }
            }
            REMOTEFILE_LOG_APPEND(AZStd::string::format("NetworkFileIO()::GetFilename(fileHandle=%u, filename=%s, filenamesize=%u) return False", fileHandle, filename?filename:"nullptr", filenameSize).c_str());
            return false;
        }

        bool NetworkFileIO::IsRemoteIOEnabled()
        {
            REMOTEFILE_LOG_CALL("NetworkFileIO()::IsRemoteIOEnabled()");
            REMOTEFILE_LOG_APPEND("NetworkFileIO()::IsRemoteIOEnabled() return True");
            return true;
        }

#ifndef REMOTEFILEIO_IS_NETWORKFILEIO
        //////////////////////////////////////////////////////////////////////////
        const int REFRESH_FILESIZE_TIME = 500;// ms
        const size_t CACHE_LOOKAHEAD_SIZE = 1024 * 256;

        RemoteFileCache::RemoteFileCache(RemoteFileCache&& other)
        {
            REMOTEFILE_LOG_CALL("RemoteFileCache()::RemoteFileCache(other)");
            *this = AZStd::move(other);
        }

        AZ::IO::RemoteFileCache& RemoteFileCache::operator=(RemoteFileCache&& other)
        {
            REMOTEFILE_LOG_CALL("RemoteFileCache()::operator=(other)");
            if (this != &other)
            {
                m_cacheLookaheadBuffer = AZStd::move(other.m_cacheLookaheadBuffer);
                m_cacheLookaheadPos = other.m_cacheLookaheadPos;
                m_fileSize = other.m_fileSize;
                m_fileSizeTime = other.m_fileSizeTime;
                m_filePosition = other.m_filePosition;
                m_fileHandle = other.m_fileHandle;
                m_openMode = other.m_openMode;
            }
            return *this;
        }

        void RemoteFileCache::Invalidate()
        {
            m_cacheLookaheadPos = 0;
            m_cacheLookaheadBuffer.clear();
        }

        AZ::u64 RemoteFileCache::RemainingBytes()
        {
            return m_cacheLookaheadBuffer.size() - m_cacheLookaheadPos;
        }

        AZ::u64 RemoteFileCache::CacheFilePosition()
        {
            return m_filePosition - RemainingBytes();
        }

        AZ::u64 RemoteFileCache::CacheStartFilePosition()
        {
            return m_filePosition - m_cacheLookaheadBuffer.size();
        }

        AZ::u64 RemoteFileCache::CacheEndFilePosition()
        {
            return m_filePosition;
        }

        bool RemoteFileCache::IsFilePositionInCache(AZ::u64 filePosition)
        {
            return filePosition >= CacheStartFilePosition() && filePosition < CacheEndFilePosition();
        }

        void RemoteFileCache::SetCachePositionFromFilePosition(AZ::u64 filePosition)
        {
            m_cacheLookaheadPos = filePosition - CacheStartFilePosition();
        }

        void RemoteFileCache::SyncCheck()
        {
#ifdef REMOTEFILEIO_SYNC_CHECK
            //don't sync check files open for write
            //they can be written to asynchronously so tell may return a different position than is cached
            //because we dont wait for the network request to finish before we immediately ask for tell
            if (AnyFlag(m_openMode & OpenMode::ModeWrite) ||
                AnyFlag(m_openMode & OpenMode::ModeAppend) ||
                AnyFlag(m_openMode & OpenMode::ModeUpdate))
            {
                return;
            }
            REMOTEFILE_LOG_CALL(AZStd::string::format("RemoteFileCache()::SyncCheck(m_fileHandle=%u)", m_fileHandle).c_str());
            FileTellRequest request(m_fileHandle);
            FileTellResponse responce;
            if (!SendRequest(request, responce))
            {
                AZ_Assert(false, "RemoteFileCache::SyncCheck(m_fileHandle=%u) Failed to send tell request.", m_fileHandle);
                REMOTEFILE_LOG_APPEND(AZStd::string::format("RemoteFileCache::SyncCheck(m_fileHandle=%u) Failed to send tell request.", m_fileHandle).c_str());
            }

            ResultCode returnValue = static_cast<ResultCode>(responce.m_resultCode);
            if (returnValue == ResultCode::Error)
            {
                AZ_TracePrintf(RemoteFileCacheChannel, "RemoteFileCache::SyncCheck(m_fileHandle=%u) tell request failed.", m_fileHandle);
                REMOTEFILE_LOG_APPEND(AZStd::string::format("RemoteFileCache::SyncCheck(m_fileHandle=%u) tell request failed.", m_fileHandle).c_str());
            }

            if (responce.m_offset != m_filePosition)
            {
                AZ_TracePrintf(RemoteFileCacheChannel, "RemoteFileCache::SyncCheck(m_fileHandle=%u) failed!!! m_filePosition=%u tell=%u", m_fileHandle, m_filePosition, responce.m_offset);
                REMOTEFILE_LOG_APPEND(AZStd::string::format("RemoteFileCache::SyncCheck(m_fileHandle=%u) failed!!! m_filePosition=%u tell=%u", m_fileHandle, m_filePosition, responce.m_offset).c_str());
            }
            else
            {
                REMOTEFILE_LOG_APPEND(AZStd::string::format("RemoteFileCache::SyncCheck(m_fileHandle=%u) Success m_filePosition=%u tell=%u", m_fileHandle, m_filePosition, responce.m_offset).c_str());
            }
#endif
        }


        void RemoteFileCache::SetFilePosition(AZ::u64 filePosition)
        {
            m_filePosition = filePosition;
            SyncCheck();
        }

        void RemoteFileCache::OffsetFilePosition(AZ::s64 offset)
        {
            m_filePosition += offset;
            SyncCheck();
        }

        bool RemoteFileCache::Eof()
        {
            return CacheFilePosition() == m_fileSize;
        }


        //////////////////////////////////////////////////////////////////////////
        RemoteFileIO::RemoteFileIO(FileIOBase* excludedFileIO)
        {
            REMOTEFILE_LOG_CALL(AZStd::string::format("RemoteFileIO()::RemoteFileIO()").c_str());
            m_excludedFileIO = excludedFileIO;
#ifdef REMOTEFILEIO_CACHE_FILETREE
            CacheFileTree();
#endif
        }

        RemoteFileIO::~RemoteFileIO()
        {
            delete m_excludedFileIO; //for now delete it, when We change to always create local file io We won't
#ifdef REMOTEFILEIO_CACHE_FILETREE
            AzFramework::AssetCatalogEventBus::Handler::BusDisconnect();
#endif
            REMOTEFILE_LOG_CALL(AZStd::string::format("RemoteFileIO()::RemoteFileIO()").c_str());
        }

        Result RemoteFileIO::Open(const char* filePath, OpenMode openMode, HandleType& fileHandle)
        {
            REMOTEFILE_LOG_CALL(AZStd::string::format("RemoteFileIO()::Open(filePath=%s, openMode=%i, fileHandle=OUT)", filePath?filePath:"nullptr", openMode).c_str());
            Result returnValue = NetworkFileIO::Open(filePath, openMode, fileHandle);
            if (returnValue == ResultCode::Success)
            {
                {
                    AZStd::lock_guard<AZStd::recursive_mutex> lock(m_remoteFileCacheGuard);
                    m_remoteFileCache.insert(fileHandle);
                    RemoteFileCache& cache = GetCache(fileHandle);
                    cache.m_fileHandle = fileHandle;
                    cache.m_openMode = openMode;
                }
            }
            REMOTEFILE_LOG_APPEND(AZStd::string::format("RemoteFileIO::Open(filePath=%s, fileHandle=%u) return %s", filePath?filePath:"nullptr", fileHandle, returnValue == ResultCode::Success ? "Success" : "Error").c_str());
            return returnValue;
        }

        Result RemoteFileIO::Close(HandleType fileHandle)
        {
            REMOTEFILE_LOG_CALL(AZStd::string::format("RemoteFileIO()::Close(fileHandle=%u)", fileHandle).c_str());
            Result returnValue = NetworkFileIO::Close(fileHandle);

            if (returnValue == ResultCode::Success)
            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_remoteFileCacheGuard);
                m_remoteFileCache.erase(fileHandle);
            }
            REMOTEFILE_LOG_APPEND(AZStd::string::format("RemoteFileIO::Open(fileHandle=%u) return %s", fileHandle, returnValue == ResultCode::Success ? "Success" : "Error").c_str());
            return returnValue;
        }

        Result RemoteFileIO::Tell(HandleType fileHandle, AZ::u64& offset)
        {
            REMOTEFILE_LOG_CALL(AZStd::string::format("RemoteFileIO()::Tell(fileHandle=%u, offset=OUT)", fileHandle).c_str());
            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_remoteFileCacheGuard);
                RemoteFileCache& cache = GetCache(fileHandle);
                offset = cache.CacheFilePosition();
            }
            REMOTEFILE_LOG_APPEND(AZStd::string::format("RemoteFileIO::Tell(fileHandle=%u, offset=%i) return Success", fileHandle, offset).c_str());
            return ResultCode::Success;
        }

        Result RemoteFileIO::Seek(HandleType fileHandle, AZ::s64 offset, SeekType type)
        {
            REMOTEFILE_LOG_CALL(AZStd::string::format("RemoteFileIO()::Seek(fileHandle=%u, offset=%i, type=%s)", fileHandle, offset, type==SeekType::SeekFromCurrent ? "SeekFromCurrent" : type==SeekType::SeekFromEnd ? "SeekFromEnd" : type==SeekType::SeekFromStart ? "SeekFromStart" : "Unknown").c_str());
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_remoteFileCacheGuard);
            RemoteFileCache& cache = GetCache(fileHandle);

            //if we land in the cache all we need to do is adjust the cache position and return
            //calculate the new position in the file
            AZ::u64 newFilePosition = 0;
            if (type == AZ::IO::SeekType::SeekFromCurrent)
            {
                newFilePosition = cache.CacheFilePosition() + offset;
            }
            else if (type == AZ::IO::SeekType::SeekFromStart)
            {
                newFilePosition = offset;
            }
            else if (type == AZ::IO::SeekType::SeekFromEnd)
            {
                AZ::u64 fileSize = 0;
                if(Size(fileHandle, fileSize)== ResultCode::Error)
                {
                    return ResultCode::Error;
                }
                newFilePosition = fileSize + offset;
            }
            else
            {
                AZ_Assert(false, "RemoteFileIO::Seek(fileHandle=%u, offset=%i, type=%s) unknown seektype. return Error", fileHandle, offset, type == SeekType::SeekFromCurrent ? "SeekFromCurrent" : type == SeekType::SeekFromEnd ? "SeekFromEnd" : type == SeekType::SeekFromStart ? "SeekFromStart" : "Unknown");
                REMOTEFILE_LOG_APPEND(AZStd::string::format("RemoteFileIO::Seek(fileHandle=%u, offset=%i, type=%s) unknown seektype. return Error", fileHandle, offset, type == SeekType::SeekFromCurrent ? "SeekFromCurrent" : type == SeekType::SeekFromEnd ? "SeekFromEnd" : type == SeekType::SeekFromStart ? "SeekFromStart" : "Unknown").c_str());
                return ResultCode::Error;
            }

            {
                AZ::u64 fileSize = 0;
                Size(fileHandle, fileSize);

                if (newFilePosition > fileSize)
                {
                    AZ_TracePrintf(RemoteFileIOChannel, "RemoteFileIO::Seek(fileHandle=%u, offset=%i, type=%s) seek to a position after the end of a file!", fileHandle, offset, type == SeekType::SeekFromCurrent ? "SeekFromCurrent" : type == SeekType::SeekFromEnd ? "SeekFromEnd" : type == SeekType::SeekFromStart ? "SeekFromStart" : "Unknown");
                    REMOTEFILE_LOG_APPEND(AZStd::string::format("RemoteFileIO::Seek(fileHandle=%u, offset=%i, type=%s) seek to a position after the end of a file!", fileHandle, offset, type == SeekType::SeekFromCurrent ? "SeekFromCurrent" : type == SeekType::SeekFromEnd ? "SeekFromEnd" : type == SeekType::SeekFromStart ? "SeekFromStart" : "Unknown").c_str());
                    newFilePosition = fileSize;
                }
            }

            //see if the new calculated position is in the cache
            if (cache.IsFilePositionInCache(newFilePosition))
            {
                //it is, so calculate what the cache position should be given the new file position and
                //set it and return success
                cache.SetCachePositionFromFilePosition(newFilePosition);
                cache.SyncCheck();
                return ResultCode::Success;
            }
            else if (newFilePosition == cache.m_filePosition)
            {
                cache.SyncCheck();
                return ResultCode::Success;
            }

            //we didn't land in the cache
            //perform the seek for real, invalidate and set new file position
            //note when setting a new absolute position we always use SeekFromStart, not the passed in seek type
            AzFramework::AssetSystem::FileSeekRequest request(fileHandle, static_cast<AZ::u32>(AZ::IO::SeekType::SeekFromStart), newFilePosition);
            if (!SendRequest(request))
            {
                AZ_Assert(false, "RemoteFileIO::Seek(fileHandle=%u, offset=%i, type=%s) Failed to send request. return Error", fileHandle, offset, type == SeekType::SeekFromCurrent ? "SeekFromCurrent" : type == SeekType::SeekFromEnd ? "SeekFromEnd" : type == SeekType::SeekFromStart ? "SeekFromStart" : "Unknown");
                REMOTEFILE_LOG_APPEND(AZStd::string::format("RemoteFileIO::Seek(fileHandle=%u, offset=%i, type=%s) Failed to send request. return Error", fileHandle, offset, type == SeekType::SeekFromCurrent ? "SeekFromCurrent" : type == SeekType::SeekFromEnd ? "SeekFromEnd" : type == SeekType::SeekFromStart ? "SeekFromStart" : "Unknown").c_str());
                return ResultCode::Error;
            }

            cache.Invalidate();
            cache.SetFilePosition(newFilePosition);
            REMOTEFILE_LOG_APPEND(AZStd::string::format("RemoteFileIO::Seek(fileHandle=%u, offset=%i, type=%s) return Success", fileHandle, offset, type == SeekType::SeekFromCurrent ? "SeekFromCurrent" : type == SeekType::SeekFromEnd ? "SeekFromEnd" : type == SeekType::SeekFromStart ? "SeekFromStart" : "Unknown").c_str());
            return ResultCode::Success;
        }

        Result RemoteFileIO::Size(HandleType fileHandle, AZ::u64& size)
        {
            REMOTEFILE_LOG_CALL(AZStd::string::format("RemoteFileIO::Size(fileHandle=%u, size=OUT)", fileHandle).c_str());
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_remoteFileCacheGuard);
            RemoteFileCache& cache = GetCache(fileHandle);

            // do we even have to check?
            AZ::u64 msNow = AZStd::GetTimeUTCMilliSecond();
            if (msNow - cache.m_fileSizeTime > REFRESH_FILESIZE_TIME)
            {
                if (NetworkFileIO::Size(fileHandle, cache.m_fileSize))
                {
                    cache.m_fileSizeTime = msNow;
                    size = cache.m_fileSize;
                    REMOTEFILE_LOG_APPEND(AZStd::string::format("RemoteFileIO::Size(fileHandle=%u) size=%u. return Success", fileHandle, size).c_str());
                    return ResultCode::Success;
                }
            }
            else
            {
                size = cache.m_fileSize;
                REMOTEFILE_LOG_APPEND(AZStd::string::format("RemoteFileIO::Size(fileHandle=%u) size=%u return Success", fileHandle, size).c_str());
                return ResultCode::Success;
            }
            REMOTEFILE_LOG_APPEND(AZStd::string::format("RemoteFileIO::Size(fileHandle=%u) return Error", fileHandle).c_str());
            return ResultCode::Error;
        }

        Result RemoteFileIO::Read(HandleType fileHandle, void* buffer, AZ::u64 size, bool failOnFewerThanSizeBytesRead, AZ::u64* bytesRead)
        {
            REMOTEFILE_LOG_CALL(AZStd::string::format("RemoteFileIO()::Read(fileHandle=%u, buffer=OUT, size=%u, failOnFewerThanSizeBytesRead=%s, bytesRead=OUT)", fileHandle, size, failOnFewerThanSizeBytesRead ? "True" : "False").c_str());
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_remoteFileCacheGuard);
            RemoteFileCache& cache = GetCache(fileHandle);

            AZ::u64 remainingBytesToRead = size;
            AZ::u64 bytesReadFromCache = 0;
            AZ::u64 remainingBytesInCache = cache.RemainingBytes();
            if (remainingBytesInCache)
            {
                AZ::u64 bytesToReadFromCache = AZStd::GetMin<AZ::u64>(remainingBytesInCache, remainingBytesToRead);
                memcpy(buffer, cache.m_cacheLookaheadBuffer.data() + cache.m_cacheLookaheadPos, bytesToReadFromCache);
                bytesReadFromCache = bytesToReadFromCache;
                remainingBytesToRead -= bytesToReadFromCache;
                cache.m_cacheLookaheadPos += bytesToReadFromCache;
            }

            REMOTEFILE_LOG_APPEND(AZStd::string::format("RemoteFileIO::Read(fileHandle=%u) bytesReadFromCache=%u remainingBytesToRead=%u", fileHandle, bytesReadFromCache, remainingBytesToRead).c_str());
            AZ::u64 bytesThatHaveBeenRead = bytesReadFromCache;
            if (bytesRead)
            {
                *bytesRead = bytesThatHaveBeenRead;
            }

            buffer = reinterpret_cast<char*>(buffer) + bytesReadFromCache;

            if (remainingBytesToRead)
            {
                AZ::u64 actualRead = 0;
                Result returnValue = NetworkFileIO::Read(fileHandle, buffer, remainingBytesToRead, true, &actualRead);

                remainingBytesToRead -= actualRead;

                if (actualRead)
                {
                    cache.OffsetFilePosition(actualRead);
                }

                bytesThatHaveBeenRead += actualRead;
                if (bytesRead)
                {
                    *bytesRead = bytesThatHaveBeenRead;
                }

                REMOTEFILE_LOG_APPEND(AZStd::string::format("RemoteFileIO::Read(fileHandle=%u, size=%u) %s remainingBytesToRead=%u. actualRead=%u", fileHandle, remainingBytesToRead, returnValue==ResultCode::Success ? "Success" : "Fail", remainingBytesToRead, actualRead).c_str());
                //if we get an error, we only return an error if failOnFewerThanSizeBytesRead
                if (returnValue == ResultCode::Error)
                {
                    if (failOnFewerThanSizeBytesRead && remainingBytesToRead)
                    {
                        REMOTEFILE_LOG_APPEND(AZStd::string::format("RemoteFileIO::Read(fileHandle=%u, bytesThatHaveBeenRead=%u) failOnFewerThanSizeBytesRead. return Error", fileHandle, bytesThatHaveBeenRead).c_str());
                        return ResultCode::Error;
                    }
                    REMOTEFILE_LOG_APPEND(AZStd::string::format("RemoteFileIO::Read(fileHandle=%u, bytesThatHaveBeenRead=%u) return Success", fileHandle, bytesThatHaveBeenRead).c_str());
                    return ResultCode::Success;
                }
            }

            //they could have asked for more bytes than there is in the file
            if (failOnFewerThanSizeBytesRead && remainingBytesToRead && Eof(fileHandle))
            {
                REMOTEFILE_LOG_APPEND(AZStd::string::format("RemoteFileIO::Read(fileHandle=%u, bytesThatHaveBeenRead=%u) failOnFewerThanSizeBytesRead. return Error", fileHandle, bytesThatHaveBeenRead).c_str());
                return ResultCode::Error;
            }

            //if we get here we have satisfied the read request.
            //if the cache is empty try to refill it if not eof.
            if (!cache.RemainingBytes() && !Eof(fileHandle))
            {
                //make sure the cache file size is up to date
                AZ::u64 fsize = 0;
                Size(fileHandle, fsize);

                AZ::u64 remainingFileBytes = cache.m_fileSize - cache.m_filePosition;
                AZ::u64 readSize = AZStd::GetMin<AZ::u64>(remainingFileBytes, CACHE_LOOKAHEAD_SIZE);

                cache.m_cacheLookaheadBuffer.clear();
                cache.m_cacheLookaheadBuffer.resize_no_construct(readSize);
                REMOTEFILE_LOG_APPEND(AZStd::string::format("RemoteFileIO::Read(fileHandle=%u, size=%u) -=CACHE READ=-", fileHandle, readSize).c_str());
                AZ::u64 actualRead = 0;
                Result returnValue = NetworkFileIO::Read(fileHandle, cache.m_cacheLookaheadBuffer.data(), readSize, false, &actualRead);
                if (actualRead)
                {
                    cache.m_cacheLookaheadBuffer.resize(actualRead);
                    cache.OffsetFilePosition(actualRead);
                    cache.m_cacheLookaheadPos = 0;
                }

                if (returnValue == ResultCode::Error)
                {
                    AZ_TracePrintf(RemoteFileIOChannel, "RemoteFileIO::Read(fileHandle=%u, size=%u) -=CACHE READ=- actualRead=%i Failed", fileHandle, readSize, actualRead);
                }
                REMOTEFILE_LOG_APPEND(AZStd::string::format("RemoteFileIO::Read(fileHandle=%u, size=%u) -=CACHE READ=- actualRead=%i %s", fileHandle, readSize, actualRead, returnValue == ResultCode::Success ? "Success" : "Fail").c_str());
            }
            REMOTEFILE_LOG_APPEND(AZStd::string::format("RemoteFileIO::Read(fileHandle=%u, bytesThatHaveBeenRead=%u) return Success", fileHandle, bytesThatHaveBeenRead).c_str());
            return ResultCode::Success;
        }

        Result RemoteFileIO::Write(HandleType fileHandle, const void* buffer, AZ::u64 size, AZ::u64* bytesWritten)
        {
            REMOTEFILE_LOG_CALL(AZStd::string::format("RemoteFileIO()::Write(fileHandle=%u, buffer=OUT, size=%u, bytesWritten=OUT)", fileHandle, size).c_str());
            // We need to seek back to where we should be in the file before we commit a write.
            // This is unnecessary if the cache is empty, or we're at the end of the cache.
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_remoteFileCacheGuard);
            RemoteFileCache& cache = GetCache(fileHandle);
            if (cache.m_cacheLookaheadBuffer.size() && cache.RemainingBytes())
            {
                // find out where we are
                AZ::u64 seekPosition = cache.CacheFilePosition();

                // note, seeks are predicted, and do not ask for a response.
                AzFramework::AssetSystem::FileSeekRequest request(fileHandle, static_cast<AZ::u32>(AZ::IO::SeekType::SeekFromStart), seekPosition);
                if (!SendRequest(request))
                {
                    AZ_Assert(false, "RemoteFileIO::Write(fileHandle=%u, size=%u) Seek Failed to send request. return Error", fileHandle, size);
                    REMOTEFILE_LOG_APPEND(AZStd::string::format("RemoteFileIO::Write(fileHandle=%u, size=%u) Seek Failed to send request. return Error", fileHandle, size).c_str());
                    return ResultCode::Error;
                }

                cache.SetFilePosition(seekPosition);
            }

            cache.Invalidate();
            REMOTEFILE_LOG_APPEND("RemoteFileIO::Write() cache.Invalidate()");
            cache.m_fileSizeTime = 0; // invalidate file size after write.

            AzFramework::AssetSystem::FileWriteRequest request(fileHandle, buffer, size);
            //always async and just return success unless bytesWritten is set then synchronous
            if (bytesWritten)
            {
                AzFramework::AssetSystem::FileWriteResponse response;
                if (!SendRequest(request, response))
                {
                    AZ_Assert(false, "RemoteFileIO::Write(fileHandle=%u, size=%u) failed to send sync write request. return Error", fileHandle, size);
                    REMOTEFILE_LOG_APPEND(AZStd::string::format("RemoteFileIO::Write(fileHandle=%u, size=%u) failed to send sync write request. return Error", fileHandle, size).c_str());
                    return ResultCode::Error;
                }
                ResultCode res = static_cast<ResultCode>(response.m_resultCode);
                if (res == ResultCode::Error)
                {
                    AZ_TracePrintf(RemoteFileIOChannel, "RemoteFileIO::Write(fileHandle=%u, size=%u) sync write request failed", fileHandle, size);
                }
                *bytesWritten = response.m_bytesWritten;

                cache.OffsetFilePosition(response.m_bytesWritten);
            }
            else
            {
                // just send the message and assume we wrote it all successfully.
                if (!SendRequest(request))
                {
                    AZ_Assert(false, "RemoteFileIO::Write(fileHandle=%u, size=%u) failed to send async write request. return Error", fileHandle, size);
                    REMOTEFILE_LOG_APPEND(AZStd::string::format("RemoteFileIO::Write(fileHandle=%u, size=%u) failed to send async write request. return Error", fileHandle, size).c_str());
                    return ResultCode::Error;
                }
                cache.OffsetFilePosition(size);
            }
            REMOTEFILE_LOG_APPEND("RemoteFileIO::Write() Success");
            return ResultCode::Success;
        }

        bool RemoteFileIO::Eof(HandleType fileHandle)
        {
            REMOTEFILE_LOG_CALL(AZStd::string::format("RemoteFileIO()::Eof(fileHandle=%u)", fileHandle).c_str());
            //make sure the cache file size is up to date
            AZ::u64 size = 0;
            Size(fileHandle, size);

            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_remoteFileCacheGuard);
            RemoteFileCache& cache = GetCache(fileHandle);
            bool isEof = cache.Eof();
            REMOTEFILE_LOG_APPEND(AZStd::string::format("RemoteFileIO::Eof(fileHandle=%u) return %s", fileHandle, isEof ? "True" : "False").c_str());
            return isEof;
        }

        //get cache should only be called while AZStd::lock_guard<AZStd::recursive_mutex> lock(m_remoteFileCacheGuard);
        AZ::IO::RemoteFileCache& RemoteFileIO::GetCache(HandleType fileHandle)
        {
            auto found = m_remoteFileCache.find(fileHandle);
            // check this because it is a serious error since it may be that you're in an unguarded access to a non-existent handle.
            AZ_Assert(found != m_remoteFileCache.end(), "RemoteFileIO::GetCache(fileHandle=%u) Missing! Did something go wrong with open?", fileHandle);
            return found->second;
        }

        void RemoteFileIO::SetAlias(const char* alias, const char* path)
        {
            if (m_excludedFileIO)
            {
                m_excludedFileIO->SetAlias(alias, path);
            }
        }

        const char* RemoteFileIO::GetAlias(const char* alias) const
        {
            return m_excludedFileIO ? m_excludedFileIO->GetAlias(alias) : nullptr;
        }

        void RemoteFileIO::ClearAlias(const char* alias)
        {
            if (m_excludedFileIO)
            {
                m_excludedFileIO->ClearAlias(alias);
            }
        }

        void RemoteFileIO::SetDeprecatedAlias(AZStd::string_view oldAlias, AZStd::string_view newAlias)
        {
            if (m_excludedFileIO)
            {
                m_excludedFileIO->SetDeprecatedAlias(oldAlias, newAlias);
            }
        }

        AZStd::optional<AZ::u64> RemoteFileIO::ConvertToAlias(char* inOutBuffer, AZ::u64 bufferLength) const
        {
            return m_excludedFileIO ? m_excludedFileIO->ConvertToAlias(inOutBuffer, bufferLength) : strlen(inOutBuffer);
        }

        bool RemoteFileIO::ConvertToAlias(AZ::IO::FixedMaxPath& convertedPath, const AZ::IO::PathView& path) const
        {
            return m_excludedFileIO ? m_excludedFileIO->ConvertToAlias(convertedPath, path) : false;
        }

        bool RemoteFileIO::ResolvePath(const char* path, char* resolvedPath, AZ::u64 resolvedPathSize) const
        {
            return m_excludedFileIO ? m_excludedFileIO->ResolvePath(path, resolvedPath, resolvedPathSize) : false;
        }

        bool RemoteFileIO::ResolvePath(AZ::IO::FixedMaxPath& resolvedPath, const AZ::IO::PathView& path) const
        {
            return m_excludedFileIO ? m_excludedFileIO->ResolvePath(resolvedPath, path) : false;
        }

        bool RemoteFileIO::ReplaceAlias(AZ::IO::FixedMaxPath& replaceAliasPath, const AZ::IO::PathView& path) const
        {
            return m_excludedFileIO ? m_excludedFileIO->ReplaceAlias(replaceAliasPath, path) : false;
        }

#ifdef REMOTEFILEIO_CACHE_FILETREE
        bool RemoteFileIO::Exists(const char* filePath)
        {
            REMOTEFILE_LOG_CALL(AZStd::string::format("RemoteFileIO()::Exists(filePath=%s)", filePath?filePath:"nullptr").c_str());
            //error checks
            if (!filePath)
            {
                REMOTEFILE_LOG_APPEND(AZStd::string::format("RemoteFileIO::Exists(filePath=nullptr) filePath=nullptr. return False").c_str());
                return false;
            }

            if (!strlen(filePath))
            {
                REMOTEFILE_LOG_APPEND("RemoteFileIO::Exists(filePath=\"\") strlen(filePath)==0. return False");
                return false;
            }

            AZStd::string filePathName(filePath);
            AZStd::replace(filePathName.begin(), filePathName.end(), '\\', '/');
            AZStd::size_t lastNonSlash = filePathName.find_last_not_of('/');
            AZStd::size_t lastSlash = filePathName.find_last_of('/');
            if (lastSlash != AZStd::string::npos && lastSlash > lastNonSlash)
            {
                filePathName.erase(lastSlash);
            }
            filePath = filePathName.c_str();

            const uint64_t numFiles = m_remoteFileTreeCache.size();
            for (uint64_t fileIdx = 0; fileIdx < numFiles; ++fileIdx)
            {
                const char* fileName = m_remoteFileTreeCache[fileIdx].c_str();
                if (!azstricmp(filePath, fileName))
                {
                    return true;
                }
            }

            const uint64_t numFolders = m_remoteFolderTreeCache.size();
            for (uint64_t folderIdx = 0; folderIdx < numFolders; ++folderIdx)
            {
                const char* folderName = m_remoteFolderTreeCache[folderIdx].c_str();
                if (!azstricmp(filePath, folderName))
                {
                    return true;
                }
            }

#ifdef REMOTEFILEIO_CACHE_FILETREE_FALLBACK
            //fall back
            bool bExists = NetworkFileIO::Exists(filePath);
            if (bExists)
            {
                //it wasn't found before, but it is now, update the cache
                if (NetworkFileIO::IsDirectory(filePath))
                {
                    m_remoteFolderTreeCache.push_back(filePath);
                }
                else
                {
                    m_remoteFileTreeCache.push_back(filePath);
                }
            }

            return bExists;
#else
            return false;
#endif
        }

        bool RemoteFileIO::IsDirectory(const char* filePath)
        {
            REMOTEFILE_LOG_CALL(AZStd::string::format("RemoteFileIO()::IsDirectory(filePath=%s)", filePath?filepath:"nullptr").c_str());
            //error checks
            if (!filePath)
            {
                REMOTEFILE_LOG_APPEND("RemoteFileIO::IsDirectory(filePath=nullptr) filePath=nullptr. return False");
                return false;
            }

            if (!strlen(filePath))
            {
                REMOTEFILE_LOG_APPEND("RemoteFileIO::IsDirectory(filePath=\"\") strlen(filePath)==0. return False");
                return false;
            }

            AZStd::string filePathName(filePath);
            AZStd::replace(filePathName.begin(), filePathName.end(), '\\', '/');
            AZStd::size_t lastNonSlash = filePathName.find_last_not_of('/');
            AZStd::size_t lastSlash = filePathName.find_last_of('/');
            if (lastSlash != AZStd::string::npos && lastSlash > lastNonSlash)
            {
                filePathName.erase(lastSlash);
            }
            filePath = filePathName.c_str();

            const uint64_t numFolders = m_remoteFolderTreeCache.size();
            for (uint64_t folderIdx = 0; folderIdx < numFolders; ++folderIdx)
            {
                const char* folderName = m_remoteFolderTreeCache[folderIdx].c_str();
                if (!azstricmp(filePath, folderName))
                {
                    return true;
                }
            }

#ifdef REMOTEFILEIO_CACHE_FILETREE_FALLBACK
            //fallback
            bool bExists = NetworkFileIO::IsDirectory(filePath);
            if (bExists)
            {
                //it wasn't found before, but it is now, update the cache
                m_remoteFolderTreeCache.push_back(filePath);
            }
            return bExists;
#else
            return false;
#endif
        }

        Result RemoteFileIO::FindFiles(const char* filePath, const char* filter, FindFilesCallbackType callback)
        {
            REMOTEFILE_LOG_CALL(AZStd::string::format("RemoteFileIO()::FindFiles(filePath=%s, filter=%s, callback=OUT)", filePath?filePath:"nullptr", filter?filter:"nullptr").c_str());
            //error checks
            if (!filePath)
            {
                REMOTEFILE_LOG_APPEND(AZStd::string::format("RemoteFileIO::FindFiles(filePath=nullptr, filter=%s) filePath=nullptr. return Error", filter?filter:"nullptr").c_str());
                return ResultCode::Error;
            }

            if (!filter)
            {
                REMOTEFILE_LOG_APPEND(AZStd::string::format("RemoteFileIO::FindFiles(filePath=%s, filter=nullptr) filter=nullptr. return Error", filePath).c_str());
                return ResultCode::Error;
            }

            if (!strlen(filePath))
            {
                REMOTEFILE_LOG_APPEND(AZStd::string::format("RemoteFileIO::FindFiles(filePath=\"\", filter=%s) strlen(filePath)==0. return Error", filter).c_str());
                return ResultCode::Error;
            }

            AZStd::string filePathName(filePath);
            AZStd::replace(filePathName.begin(), filePathName.end(), '\\', '/');
            AZStd::size_t lastNonSlash = filePathName.find_last_not_of('/');
            AZStd::size_t lastSlash = filePathName.find_last_of('/');
            if (lastSlash != AZStd::string::npos && lastSlash > lastNonSlash)
            {
                filePathName.erase(lastSlash);
            }
            filePath = filePathName.c_str();
            AZ::u64 filePathLen = filePathName.length();

            // files callbacks
            const uint64_t numFiles = m_remoteFileTreeCache.size();
            for (uint64_t fileIdx = 0; fileIdx < numFiles; ++fileIdx)
            {
                const char* cachedFilePath = m_remoteFileTreeCache[fileIdx].c_str();
                if (!azstrnicmp(filePath, cachedFilePath, filePathLen))
                {
                    if (strlen(cachedFilePath) > filePathLen)
                    {
                        const char* cachedFileName = cachedFilePath + filePathLen + 1;
                        if (!strchr(cachedFileName, '/'))
                        {
                            //no slash was found so this file is in this folder
                            if (AZStd::wildcard_match(filter, cachedFileName))
                            {
                                if (!callback(cachedFilePath))
                                {
                                    return ResultCode::Success;
                                }
                            }
                        }
                    }
                }
            }

            // folders
            const uint64_t numFolders = m_remoteFolderTreeCache.size();
            for (uint64_t folderIdx = 0; folderIdx < numFolders; ++folderIdx)
            {
                const char* cachedFolderPath = m_remoteFolderTreeCache[folderIdx].c_str();
                if (!azstrnicmp(filePath, cachedFolderPath, filePathLen))
                {
                    if (strlen(cachedFolderPath) > filePathLen)
                    {
                        const char* cachedFolderName = cachedFolderPath + filePathLen + 1;
                        if (!strchr(cachedFolderName, '/'))
                        {
                            //no slash was found so this is not a sub folder
                            if (AZStd::wildcard_match(filter, cachedFolderName))
                            {
                                if (!callback(cachedFolderPath))
                                {
                                    return ResultCode::Success;
                                }
                            }
                        }
                    }
                }
            }

            return ResultCode::Success;
        }

        Result RemoteFileIO::CacheFileTree()
        {
            m_remoteFileTreeCache.clear();
            m_remoteFolderTreeCache.clear();

            REMOTEFILE_LOG_CALL("RemoteFileIO()::CacheFileTree()");
            FileTreeRequest request;
            FileTreeResponse response;
            if (!SendRequest(request, response))
            {
                AZ_Assert(false, "RemoteFileIO::CacheFileTree() could not send request. return Error");
                REMOTEFILE_LOG_APPEND("RemoteFileIO::CacheFileTree() could not send request. return Error");
                return ResultCode::Error;
            }

            Result returnValue = static_cast<ResultCode>(response.m_resultCode);
            if (returnValue == ResultCode::Success)
            {
                for (auto& it: response.m_fileList)
                {
                    AZStd::replace(it.begin(), it.end(), '\\', '/');
                    m_remoteFileTreeCache.push_back(it);
                }

                for (auto& it: response.m_folderList)
                {
                    AZStd::replace(it.begin(), it.end(), '\\', '/');
                    AZStd::size_t lastNonSlash = it.find_last_not_of('/');
                    AZStd::size_t lastSlash = it.find_last_of('/');
                    if (lastSlash != AZStd::string::npos && lastSlash > lastNonSlash)
                    {
                        it.erase(lastSlash);
                    }
                    m_remoteFolderTreeCache.push_back(it);
                }
            }

            AzFramework::AssetCatalogEventBus::Handler::BusConnect();

            return returnValue;
        }

        //=========================================================================
        // AssetCatalogEventBus::OnCatalogAssetChanged
        //=========================================================================
        void RemoteFileIO::OnCatalogAssetChanged(const AZ::Data::AssetId& assetId)
        {
            AZ_UNUSED(assetId);
            CacheFileTree();
        }

        //=========================================================================
        // AssetSystemBus::OnCatalogAssetRemoved
        //=========================================================================
        void RemoteFileIO::OnCatalogAssetRemoved(const AZ::Data::AssetId& assetId)
        {
            AZ_UNUSED(assetId);
            CacheFileTree();
        }
#endif //REMOTEFILEIO_CACHE_FILETREE
#endif //REMOTEFILEIO_IS_NETWORKFILEIO
    } // namespace IO
}//namespace AZ
