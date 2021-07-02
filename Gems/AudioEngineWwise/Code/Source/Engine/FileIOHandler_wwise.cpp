/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/PlatformIncl.h>
#include <FileIOHandler_wwise.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/IO/IStreamer.h>

#include <IAudioInterfacesCommonData.h>
#include <AkPlatformFuncs_Platform.h>
#include <AudioEngineWwise_Traits_Platform.h>

#include <platform.h>
#include <ISystem.h>
#include <AzFramework/Archive/IArchive.h>

#define MAX_NUMBER_STRING_SIZE      (10)    // 4G
#define ID_TO_STRING_FORMAT_BANK    AKTEXT("%u.bnk")
#define ID_TO_STRING_FORMAT_WEM     AKTEXT("%u.wem")
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
        deviceSettings.uIOMemorySize = poolSize;
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
        const char* openModeString = nullptr;
        switch (openMode)
        {
        case AK_OpenModeRead:
            openModeString = "rbx";
            break;
        case AK_OpenModeWrite:
            openModeString = "wbx";
            break;
        case AK_OpenModeWriteOvrwr:
            openModeString = "w+bx";
            break;
        case AK_OpenModeReadWrite:
            openModeString = "abx";
            break;
        default:
            AZ_Assert(false, "Unknown Wwise file open mode.");
            return false;
        }

        const size_t fileSize = gEnv->pCryPak->FGetSize(filename);
        if (fileSize > 0)
        {
            AZ::IO::HandleType fileHandle = gEnv->pCryPak->FOpen(filename, openModeString, AZ::IO::IArchive::FOPEN_HINT_DIRECT_OPERATION);
            if (fileHandle != AZ::IO::InvalidHandle)
            {
                fileDesc.hFile = GetAkFileHandle(fileHandle);
                fileDesc.iFileSize = static_cast<AkInt64>(fileSize);
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
        AZ_Assert(buffer, "Wwise didn't provide a valid buffer to write to.");

        AZ::IO::HandleType fileHandle = GetRealFileHandle(fileDesc.hFile);
        const uint64_t currentFileReadPos = gEnv->pCryPak->FTell(fileHandle);
        const uint64_t wantedFileReadPos = static_cast<uint64_t>(transferInfo.uFilePosition);

        if (currentFileReadPos != wantedFileReadPos)
        {
            gEnv->pCryPak->FSeek(fileHandle, wantedFileReadPos, SEEK_SET);
        }

        const size_t bytesRead = gEnv->pCryPak->FReadRaw(buffer, 1, transferInfo.uRequestedSize, fileHandle);
        AZ_Assert(bytesRead == static_cast<size_t>(transferInfo.uRequestedSize), 
            "Number of bytes read (%zu) for Wwise request doesn't match the requested size (%u).", bytesRead, transferInfo.uRequestedSize);
        return (bytesRead > 0) ? AK_Success : AK_Fail;
    }

    AKRESULT CBlockingDevice_wwise::Write(AkFileDesc& fileDesc, const AkIoHeuristics&, void* data, AkIOTransferInfo& transferInfo)
    {
        AZ_Assert(data, "Wwise didn't provide a valid buffer to read from.");

        AZ::IO::HandleType fileHandle = GetRealFileHandle(fileDesc.hFile);

        const uint64_t currentFileWritePos = gEnv->pCryPak->FTell(fileHandle);
        const uint64_t wantedFileWritePos = static_cast<uint64_t>(transferInfo.uFilePosition);

        if (currentFileWritePos != wantedFileWritePos)
        {
            gEnv->pCryPak->FSeek(fileHandle, wantedFileWritePos, SEEK_SET);
        }

        const size_t bytesWritten = gEnv->pCryPak->FWrite(data, 1, static_cast<size_t>(transferInfo.uRequestedSize), fileHandle);
        if (bytesWritten != static_cast<size_t>(transferInfo.uRequestedSize))
        {
            AZ_Error("Wwise", false, "Number of bytes written (%zu) for Wwise request doesn't match the requested size (%u).",
                bytesWritten, transferInfo.uRequestedSize);
            return AK_Fail;
        }
        return AK_Success;
    }

    AKRESULT CBlockingDevice_wwise::Close(AkFileDesc& fileDesc)
    {
        return gEnv->pCryPak->FClose(GetRealFileHandle(fileDesc.hFile)) ? AK_Success : AK_Fail;
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
        AK_CHAR_TO_UTF16(deviceDesc.szDeviceName, "CryPak", AZ_ARRAY_SIZE(deviceDesc.szDeviceName));
        deviceDesc.uStringSize = AKPLATFORM::AkUtf16StrLen(deviceDesc.szDeviceName);
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
        deviceSettings.uIOMemorySize = poolSize;
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
        const size_t fileSize = gEnv->pCryPak->FGetSize(filename);
        if (fileSize)
        {
            AZStd::string* filenameStore = azcreate(AZStd::string, (filename));
            fileDesc.hFile = AkFileHandle();
            fileDesc.iFileSize = static_cast<AkInt64>(fileSize);
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
            AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Audio);
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
        AK_CHAR_TO_UTF16(deviceDesc.szDeviceName, "Streamer", AZ_ARRAY_SIZE(deviceDesc.szDeviceName));
        deviceDesc.uStringSize = AKPLATFORM::AkUtf16StrLen(deviceDesc.szDeviceName);
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

            AkOSChar fileName[MAX_FILETITLE_SIZE] = { '\0' };

            const AkOSChar* const filenameFormat = (flags->uCodecID == AKCODECID_BANK ? ID_TO_STRING_FORMAT_BANK : ID_TO_STRING_FORMAT_WEM);

            AK_OSPRINTF(fileName, MAX_FILETITLE_SIZE, filenameFormat, static_cast<int unsigned>(fileID));

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
