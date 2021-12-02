/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <FileIOHandler_wwise.h>

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/IStreamer.h>
#include <AzCore/Debug/Profiler.h>

#include <IAudioInterfacesCommonData.h>
#include <AkPlatformFuncs_Platform.h>
#include <AudioEngineWwise_Traits_Platform.h>
#include <cinttypes>

#define MAX_NUMBER_STRING_SIZE      (10)    // max digits in u32 base-10 number
#define MAX_EXTENSION_SIZE          (4)     // .xxx
#define MAX_FILETITLE_SIZE          (MAX_NUMBER_STRING_SIZE + MAX_EXTENSION_SIZE + 1)   // null-terminated


namespace Audio
{
    // AkFileHandle must be able to store our AZ::IO::HandleType
    static_assert(sizeof(AkFileHandle) >= sizeof(AZ::IO::HandleType), "AkFileHandle must be able to store at least the size of a AZ::IO::HandleType");

    namespace Platform
    {
        AkFileHandle GetAkFileHandle(AZ::IO::HandleType realFileHandle);
        AZ::IO::HandleType GetRealFileHandle(AkFileHandle akFileHandle);

        void SetThreadProperties(AkThreadProperties& threadProperties);
    }

    AkFileHandle GetAkFileHandle(AZ::IO::HandleType realFileHandle)
    {
        if (realFileHandle == AZ::IO::InvalidHandle)
        {
            return InvalidAkFileHandle;
        }

        return Platform::GetAkFileHandle(realFileHandle);
    }

    AZ::IO::HandleType GetRealFileHandle(AkFileHandle akFileHandle)
    {
        if (akFileHandle == InvalidAkFileHandle)
        {
            return AZ::IO::InvalidHandle;
        }

        return Platform::GetRealFileHandle(akFileHandle);
    }

    CBlockingDevice_wwise::~CBlockingDevice_wwise()
    {
        Destroy();
    }

    bool CBlockingDevice_wwise::Init(size_t poolSize)
    {
        Destroy();

        AkDeviceSettings deviceSettings;
        AK::StreamMgr::GetDefaultDeviceSettings(deviceSettings);
        deviceSettings.uIOMemorySize = aznumeric_cast<AkUInt32>(poolSize);
        deviceSettings.uSchedulerTypeFlags = AK_SCHEDULER_BLOCKING;
        Platform::SetThreadProperties(deviceSettings.threadProperties);

        m_deviceID = AK::StreamMgr::CreateDevice(deviceSettings, this);
        return m_deviceID != AK_INVALID_DEVICE_ID;
    }

    void CBlockingDevice_wwise::Destroy()
    {
        if (m_deviceID != AK_INVALID_DEVICE_ID)
        {
            AK::StreamMgr::DestroyDevice(m_deviceID);
            m_deviceID = AK_INVALID_DEVICE_ID;
        }
    }

    bool CBlockingDevice_wwise::Open(const char* filename, AkOpenMode openMode, AkFileDesc& fileDesc)
    {
        AZ::IO::OpenMode azOpenMode = AZ::IO::OpenMode::ModeBinary;
        switch (openMode)
        {
        case AK_OpenModeRead:
            azOpenMode |= AZ::IO::OpenMode::ModeRead;
            break;
        case AK_OpenModeWrite:
            azOpenMode |= AZ::IO::OpenMode::ModeWrite;
            break;
        case AK_OpenModeWriteOvrwr:
            azOpenMode |= (AZ::IO::OpenMode::ModeUpdate | AZ::IO::OpenMode::ModeWrite);
            break;
        case AK_OpenModeReadWrite:
            azOpenMode |= (AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeWrite);
            break;
        default:
            AZ_Assert(false, "Unknown Wwise file open mode.");
            return false;
        }

        auto fileIO = AZ::IO::FileIOBase::GetInstance();
        if (AZ::u64 fileSize = 0;
            fileIO->Size(filename, fileSize) && fileSize != 0)
        {
            AZ::IO::HandleType fileHandle = AZ::IO::InvalidHandle;
            fileIO->Open(filename, azOpenMode, fileHandle);
            if (fileHandle != AZ::IO::InvalidHandle)
            {
                fileDesc.hFile = GetAkFileHandle(fileHandle);
                fileDesc.iFileSize = aznumeric_cast<AkInt64>(fileSize);
                fileDesc.uSector = 0;
                fileDesc.deviceID = m_deviceID;
                fileDesc.pCustomParam = nullptr;
                fileDesc.uCustomParamSize = 0;

                return true;
            }
        }

        return false;
    }

    AKRESULT CBlockingDevice_wwise::Read(AkFileDesc& fileDesc, const AkIoHeuristics&, void* buffer, AkIOTransferInfo& transferInfo)
    {
        AZ_Assert(buffer, "Wwise didn't provide a valid desination buffer to Read into.");

        AZ::IO::HandleType fileHandle = GetRealFileHandle(fileDesc.hFile);
        auto fileIO = AZ::IO::FileIOBase::GetInstance();

        AZ::u64 currentFileReadPos = 0;
        fileIO->Tell(fileHandle, currentFileReadPos);

        if (currentFileReadPos != transferInfo.uFilePosition)
        {
            fileIO->Seek(fileHandle, aznumeric_cast<AZ::s64>(transferInfo.uFilePosition), AZ::IO::SeekType::SeekFromStart);
        }

        AZ::u64 bytesRead = 0;
        fileIO->Read(fileHandle, buffer, aznumeric_cast<AZ::u64>(transferInfo.uRequestedSize), &bytesRead);
        const bool readOk = (bytesRead == aznumeric_cast<AZ::u64>(transferInfo.uRequestedSize));

        AZ_Assert(readOk, 
            "Number of bytes read (%" PRIu64 ") for read request doesn't match the requested size (%u).",
            bytesRead, transferInfo.uRequestedSize);
        return readOk ? AK_Success : AK_Fail;
    }

    AKRESULT CBlockingDevice_wwise::Write(AkFileDesc& fileDesc, const AkIoHeuristics&, void* data, AkIOTransferInfo& transferInfo)
    {
        AZ_Assert(data, "Wwise didn't provide a valid source buffer to Write from.");

        AZ::IO::HandleType fileHandle = GetRealFileHandle(fileDesc.hFile);
        auto fileIO = AZ::IO::FileIOBase::GetInstance();

        AZ::u64 currentFileWritePos = 0;
        fileIO->Tell(fileHandle, currentFileWritePos);

        if (currentFileWritePos != transferInfo.uFilePosition)
        {
            fileIO->Seek(fileHandle, aznumeric_cast<AZ::s64>(transferInfo.uFilePosition), AZ::IO::SeekType::SeekFromStart);
        }

        AZ::u64 bytesWritten = 0;
        fileIO->Write(fileHandle, data, aznumeric_cast<AZ::u64>(transferInfo.uRequestedSize), &bytesWritten);
        const bool writeOk = (bytesWritten == aznumeric_cast<AZ::u64>(transferInfo.uRequestedSize));

        AZ_Error("Wwise", writeOk,
                "Number of bytes written (%" PRIu64 ") for write request doesn't match the requested size (%u).",
                bytesWritten, transferInfo.uRequestedSize);
        return writeOk ? AK_Success : AK_Fail;
    }

    AKRESULT CBlockingDevice_wwise::Close(AkFileDesc& fileDesc)
    {
        auto fileIO = AZ::IO::FileIOBase::GetInstance();
        return fileIO->Close(GetRealFileHandle(fileDesc.hFile)) ? AK_Success : AK_Fail;
    }

    AkUInt32 CBlockingDevice_wwise::GetBlockSize([[maybe_unused]] AkFileDesc& fileDesc)
    {
        // No constraint on block size (file seeking).
        return 1;
    }

    void CBlockingDevice_wwise::GetDeviceDesc(AkDeviceDesc& deviceDesc)
    {
        deviceDesc.bCanRead = true;
        deviceDesc.bCanWrite = true;
        deviceDesc.deviceID = m_deviceID;
        AK_CHAR_TO_UTF16(deviceDesc.szDeviceName, "IO::IArchive", AZ_ARRAY_SIZE(deviceDesc.szDeviceName));
        deviceDesc.uStringSize = aznumeric_cast<AkUInt32>(AKPLATFORM::AkUtf16StrLen(deviceDesc.szDeviceName));
    }
    
    AkUInt32 CBlockingDevice_wwise::GetDeviceData()
    {
        return 1;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////

    CStreamingDevice_wwise::~CStreamingDevice_wwise()
    {
        Destroy();
    }

    bool CStreamingDevice_wwise::Init(size_t poolSize)
    {
        Destroy();

        AkDeviceSettings deviceSettings;
        AK::StreamMgr::GetDefaultDeviceSettings(deviceSettings);
        deviceSettings.uIOMemorySize = aznumeric_cast<AkUInt32>(poolSize);
        deviceSettings.uSchedulerTypeFlags = AK_SCHEDULER_DEFERRED_LINED_UP;
        Platform::SetThreadProperties(deviceSettings.threadProperties);

        m_deviceID = AK::StreamMgr::CreateDevice(deviceSettings, this);
        return m_deviceID != AK_INVALID_DEVICE_ID;
    }

    void CStreamingDevice_wwise::Destroy()
    {
        if (m_deviceID != AK_INVALID_DEVICE_ID)
        {
            AK::StreamMgr::DestroyDevice(m_deviceID);
            m_deviceID = AK_INVALID_DEVICE_ID;
        }
    }

    bool CStreamingDevice_wwise::Open(const char* filename, [[maybe_unused]] AkOpenMode openMode, AkFileDesc& fileDesc)
    {
        AZ_Assert(openMode == AK_OpenModeRead, "Wwise Async File IO - Only supports opening files for reading.\n");
        auto fileIO = AZ::IO::FileIOBase::GetInstance();
        if (AZ::u64 fileSize = 0;
            fileIO->Size(filename, fileSize) && fileSize != 0)
        {
            AZStd::string* filenameStore = azcreate(AZStd::string, (filename));
            fileDesc.hFile = AkFileHandle();
            fileDesc.iFileSize = aznumeric_cast<AkInt64>(fileSize);
            fileDesc.uSector = 0;
            fileDesc.deviceID = m_deviceID;
            fileDesc.pCustomParam = filenameStore;
            fileDesc.uCustomParamSize = sizeof(AZStd::string*);

            auto streamer = AZ::Interface<AZ::IO::IStreamer>::Get();
            streamer->QueueRequest(streamer->CreateDedicatedCache(*filenameStore));

            return true;
        }
        return false;
    }

    AKRESULT CStreamingDevice_wwise::Read(AkFileDesc& fileDesc, const AkIoHeuristics& heuristics, AkAsyncIOTransferInfo& transferInfo)
    {
        AZ_Assert(fileDesc.pCustomParam, "Wwise Async File IO - Reading a file before it has been opened.\n");

        auto callback = [&transferInfo](AZ::IO::FileRequestHandle request)
        {
            AZ_PROFILE_FUNCTION(Audio);
            AZ::IO::IStreamerTypes::RequestStatus status = AZ::Interface<AZ::IO::IStreamer>::Get()->GetRequestStatus(request);
            switch (status)
            {
            case AZ::IO::IStreamerTypes::RequestStatus::Completed:
                transferInfo.pCallback(&transferInfo, AK_Success);
                break;
            case AZ::IO::IStreamerTypes::RequestStatus::Canceled:
                transferInfo.pCallback(&transferInfo, AK_Cancelled);
                break;
            default:
                transferInfo.pCallback(&transferInfo, AK_Fail);
                break;
            }
        };

        // The priorities for Wwise range from 0 (lowest priority) to 100 (highest priority). AZ::IO::Streamer has
        // a similar range except between 0 (lowest) and 255 (highest) so remap from one to the other.
        static_assert(AK_MIN_PRIORITY == 0, "The minimum priority for Wwise has changed, please update the conversion to AZ::IO::Streamers priority.");
        static_assert(AK_DEFAULT_PRIORITY == 50, "The default priority for Wwise has changed, please update the conversion to AZ::IO::Streamers priority.");
        static_assert(AK_MAX_PRIORITY == 100, "The maximum priority for Wwise has changed, please update the conversion to AZ::IO::Streamers priority.");
        static_assert(AZ::IO::IStreamerTypes::s_priorityLowest == 0, "The priority range for AZ::IO::Streamer has changed, please update Wwise to match.");
        static_assert(AZ::IO::IStreamerTypes::s_priorityHighest == 255, "The priority range for AZ::IO::Streamer has changed, please update Wwise to match.");
        AZ::u16 wwisePriority = aznumeric_caster(heuristics.priority);
        AZ::u8 priority = aznumeric_caster(
              (wwisePriority << 1) // 100 -> 200 
            + (wwisePriority >> 1) // 200 -> 250
            + (wwisePriority >> 4) // 250 -> 256
            - (wwisePriority >> 6));  // 256 -> 255

        auto filename = reinterpret_cast<AZStd::string*>(fileDesc.pCustomParam);
        auto offset = aznumeric_cast<size_t>(transferInfo.uFilePosition);
        auto readSize = aznumeric_cast<size_t>(transferInfo.uRequestedSize);
        auto bufferSize = aznumeric_cast<size_t>(transferInfo.uBufferSize);
        AZStd::chrono::microseconds deadline = AZStd::chrono::duration<float, AZStd::milli>(heuristics.fDeadline);

        auto streamer = AZ::Interface<AZ::IO::IStreamer>::Get();
        AZ::IO::FileRequestPtr request = streamer->Read(*filename, transferInfo.pBuffer, bufferSize, readSize, deadline, priority, offset);
        streamer->SetRequestCompleteCallback(request, AZStd::move(callback));
        streamer->QueueRequest(AZStd::move(request));
        return AK_Success;
    }

    AKRESULT CStreamingDevice_wwise::Write(AkFileDesc&, const AkIoHeuristics&, AkAsyncIOTransferInfo&)
    {
        AZ_Assert(false, "Wwise Async File IO - Writing audio data is not supported for AZ::IO::Streamer based device.\n");
        return AK_Fail;
    }

    AKRESULT CStreamingDevice_wwise::Close(AkFileDesc& fileDesc)
    {
        AZ_Assert(fileDesc.pCustomParam, "Wwise Async File IO - Closing a file before it has been opened.\n");
        auto filename = reinterpret_cast<AZStd::string*>(fileDesc.pCustomParam);
        auto streamer = AZ::Interface<AZ::IO::IStreamer>::Get();
        streamer->QueueRequest(streamer->DestroyDedicatedCache(*filename));
        azdestroy(filename);
        return AK_Success;
    }

    AkUInt32 CStreamingDevice_wwise::GetBlockSize([[maybe_unused]] AkFileDesc& fileDesc)
    {
        // No constraint on block size (file seeking).
        return 1;
    }

    void CStreamingDevice_wwise::GetDeviceDesc(AkDeviceDesc& deviceDesc)
    {
        deviceDesc.bCanRead = true;
        deviceDesc.bCanWrite = false;
        deviceDesc.deviceID = m_deviceID;
        AK_CHAR_TO_UTF16(deviceDesc.szDeviceName, "IO::IStreamer", AZ_ARRAY_SIZE(deviceDesc.szDeviceName));
        deviceDesc.uStringSize = aznumeric_cast<AkUInt32>(AKPLATFORM::AkUtf16StrLen(deviceDesc.szDeviceName));
    }

    AkUInt32 CStreamingDevice_wwise::GetDeviceData()
    {
        return 2;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    CFileIOHandler_wwise::CFileIOHandler_wwise()
        : m_useAsyncOpen(false)
    {
        ::memset(m_bankPath, 0, AK_MAX_PATH * sizeof(AkOSChar));
        ::memset(m_languageFolder, 0, AK_MAX_PATH * sizeof(AkOSChar));
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    AKRESULT CFileIOHandler_wwise::Init(size_t poolSize)
    {
        // If the Stream Manager's File Location Resolver was not set yet, set this object as the
        // File Location Resolver (this I/O hook is also able to resolve file location).
        if (!AK::StreamMgr::GetFileLocationResolver())
        {
            AK::StreamMgr::SetFileLocationResolver(this);
        }

        if (!m_streamingDevice.Init(poolSize))
        {
            return AK_Fail;
        }

        if (!m_blockingDevice.Init(poolSize))
        {
            return AK_Fail;
        }
        return AK_Success;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CFileIOHandler_wwise::ShutDown()
    {
        if (AK::StreamMgr::GetFileLocationResolver() == this)
        {
            AK::StreamMgr::SetFileLocationResolver(nullptr);
        }

        m_blockingDevice.Destroy();
        m_streamingDevice.Destroy();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    AKRESULT CFileIOHandler_wwise::Open(const AkOSChar* fileName, AkOpenMode openMode, AkFileSystemFlags* flags, bool& syncOpen, AkFileDesc& fileDesc)
    {
        AKRESULT akResult = AK_Fail;

        if (syncOpen || !m_useAsyncOpen)
        {
            syncOpen = true;
            AkOSChar finalFilePath[AK_MAX_PATH] = { '\0' };
            AKPLATFORM::SafeStrCat(finalFilePath, m_bankPath, AK_MAX_PATH);

            if (flags && openMode == AK_OpenModeRead)
            {
                // Add language folder if the file is localized.
                if (flags->uCompanyID == AKCOMPANYID_AUDIOKINETIC && flags->uCodecID == AKCODECID_BANK && flags->bIsLanguageSpecific)
                {
                    AKPLATFORM::SafeStrCat(finalFilePath, m_languageFolder, AK_MAX_PATH);
                }
            }

            AKPLATFORM::SafeStrCat(finalFilePath, fileName, AK_MAX_PATH);

            char* tempStr = nullptr;
            CONVERT_OSCHAR_TO_CHAR(finalFilePath, tempStr);
            if (openMode == AK_OpenModeRead)
            {
                return m_streamingDevice.Open(tempStr, openMode, fileDesc) ? AK_Success : AK_Fail;
            }
            return m_blockingDevice.Open(tempStr, openMode, fileDesc) ? AK_Success : AK_Fail;
        }
        return akResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    AKRESULT CFileIOHandler_wwise::Open(AkFileID fileID, AkOpenMode openMode, AkFileSystemFlags* flags, bool& syncOpen, AkFileDesc& fileDesc)
    {
        AKRESULT akResult = AK_Fail;

        if (flags && (syncOpen || !m_useAsyncOpen))
        {
            syncOpen = true;
            AkOSChar finalFilePath[AK_MAX_PATH] = { '\0' };
            AKPLATFORM::SafeStrCat(finalFilePath, m_bankPath, AK_MAX_PATH);

            if (openMode == AK_OpenModeRead)
            {
                // Add language folder if the file is localized.
                if (flags->uCompanyID == AKCOMPANYID_AUDIOKINETIC && flags->bIsLanguageSpecific)
                {
                    AKPLATFORM::SafeStrCat(finalFilePath, m_languageFolder, AK_MAX_PATH);
                }
            }

            AkOSChar fileName[MAX_FILETITLE_SIZE] = { 0 };

            if (flags->uCodecID == AKCODECID_BANK)
            {
                AK_OSPRINTF(fileName, MAX_FILETITLE_SIZE, AKTEXT("%u.bnk"), static_cast<unsigned int>(fileID));
            }
            else
            {
                AK_OSPRINTF(fileName, MAX_FILETITLE_SIZE, AKTEXT("%u.wem"), static_cast<unsigned int>(fileID));
            }

            AKPLATFORM::SafeStrCat(finalFilePath, fileName, AK_MAX_PATH);

            char* filePath = nullptr;
            CONVERT_OSCHAR_TO_CHAR(finalFilePath, filePath);
            if (openMode == AK_OpenModeRead)
            {
                return m_streamingDevice.Open(filePath, openMode, fileDesc) ? AK_Success : AK_Fail;
            }
            return m_blockingDevice.Open(filePath, openMode, fileDesc) ? AK_Success : AK_Fail;
        }
        return akResult;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CFileIOHandler_wwise::SetBankPath(const char* const bankPath)
    {
        const AkOSChar* akBankPath = nullptr;
        CONVERT_CHAR_TO_OSCHAR(bankPath, akBankPath);
        AKPLATFORM::SafeStrCpy(m_bankPath, akBankPath, AK_MAX_PATH);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    void CFileIOHandler_wwise::SetLanguageFolder(const char* const languageFolder)
    {
        const AkOSChar* akLanguageFolder = nullptr;
        CONVERT_CHAR_TO_OSCHAR(languageFolder, akLanguageFolder);
        AKPLATFORM::SafeStrCpy(m_languageFolder, akLanguageFolder, AK_MAX_PATH);
    }

} // namespace Audio
