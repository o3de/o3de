/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <FileIOHandler_wwise_Platform.h>

#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/SoundEngine/Common/AkStreamMgrModule.h>

namespace Audio
{
    //! Wwise file IO device that access the Open 3D Engine file system through standard blocking file IO calls. Wwise will still
    //! run these in separate threads so it won't be blocking the audio playback, but it will interfere with the internal
    //! file IO scheduling of Open 3D Engine. This class can also write, so it's intended use is for one-off file reads and
    //! for tools to be able to write files.
    class CBlockingDevice_wwise
        : public AK::StreamMgr::IAkIOHookBlocking
    {
    public:
        ~CBlockingDevice_wwise() override;

        bool Init(size_t poolSize);
        void Destroy();
        AkDeviceID GetDeviceID() const { return m_deviceID; }
        bool Open(const char* filename, AkOpenMode openMode, AkFileDesc& fileDesc);

        AKRESULT Read(AkFileDesc& fileDesc, const AkIoHeuristics& heuristics, void* buffer, AkIOTransferInfo& transferInfo) override;
        AKRESULT Write(AkFileDesc& fileDesc, const AkIoHeuristics& heuristics, void* data, AkIOTransferInfo& transferInfo) override;

        AKRESULT Close(AkFileDesc& fileDesc) override;
        AkUInt32 GetBlockSize(AkFileDesc& fileDesc) override;
        void GetDeviceDesc(AkDeviceDesc& deviceDesc) override;
        AkUInt32 GetDeviceData() override;

    protected:
        AkDeviceID m_deviceID = AK_INVALID_DEVICE_ID;
    };

    //! Wwise file IO device that uses AZ::IO::Streamer to asynchronously handle file requests. By using AZ::IO::Streamer file requests
    //! can be scheduled along side other file requests for optimal disk usage. This class can't write and is intended to be used 
    //! as part of a streaming system.
    class CStreamingDevice_wwise
        : public AK::StreamMgr::IAkIOHookDeferred
    {
    public:
        ~CStreamingDevice_wwise() override;

        bool Init(size_t poolSize);
        void Destroy();
        AkDeviceID GetDeviceID() const { return m_deviceID; }
        bool Open(const char* filename, AkOpenMode openMode, AkFileDesc& fileDesc);

        AKRESULT Read(AkFileDesc& fileDesc, const AkIoHeuristics& heuristics, AkAsyncIOTransferInfo& transferInfo) override;
        AKRESULT Write(AkFileDesc& fileDesc, const AkIoHeuristics& heuristics, AkAsyncIOTransferInfo& transferInfo) override;
        void Cancel([[maybe_unused]] AkFileDesc& fileDesc, [[maybe_unused]] AkAsyncIOTransferInfo& transferInfo, [[maybe_unused]] bool& cancelAllTransfersForThisFile) override {}

        AKRESULT Close(AkFileDesc& fileDesc) override;
        AkUInt32 GetBlockSize(AkFileDesc& fileDesc) override;
        void GetDeviceDesc(AkDeviceDesc& deviceDesc) override;
        AkUInt32 GetDeviceData() override;

    protected:
        AkDeviceID m_deviceID = AK_INVALID_DEVICE_ID;
    };

    class CFileIOHandler_wwise
        : public AK::StreamMgr::IAkFileLocationResolver
    {
    public:

        CFileIOHandler_wwise();
        ~CFileIOHandler_wwise() override = default;

        CFileIOHandler_wwise(const CFileIOHandler_wwise&) = delete;
        CFileIOHandler_wwise& operator=(const CFileIOHandler_wwise&) = delete;

        AKRESULT Init(size_t poolSize);
        void ShutDown();

        // IAkFileLocationResolver overrides.
        AKRESULT Open(const AkOSChar* fileName, AkOpenMode openMode, AkFileSystemFlags* flags, bool& syncOpen, AkFileDesc& fileDesc) override;
        AKRESULT Open(AkFileID fileID, AkOpenMode openMode, AkFileSystemFlags* flags, bool& syncOpen, AkFileDesc& fileDesc) override;
        // ~IAkFileLocationResolver overrides.

        void SetBankPath(const char* const bankPath);
        void SetLanguageFolder(const char* const languageFolder);

    private:
        CStreamingDevice_wwise m_streamingDevice;
        CBlockingDevice_wwise m_blockingDevice;
        AkOSChar m_bankPath[AK_MAX_PATH];
        AkOSChar m_languageFolder[AK_MAX_PATH];
        bool m_useAsyncOpen;
    };
} // namespace Audio
