/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SaveDataSystemComponent.h>

#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/Utils/Utils.h>
#include <AzCore/PlatformIncl.h>

#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace SaveData
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Platform specific implementation for the save data system component on Linux
    class SaveDataSystemComponentLinux : public SaveDataSystemComponent::Implementation
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        static constexpr const char* DefaultSaveDataDirectoryName = "SaveData";

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(SaveDataSystemComponentLinux, AZ::SystemAllocator, 0);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] saveDataSystemComponent Reference to the parent being implemented
        SaveDataSystemComponentLinux(SaveDataSystemComponent& saveDataSystemComponent);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destructor
        ~SaveDataSystemComponentLinux() override;

    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref SaveData::SaveDataSystemComponent::Implementation::SaveDataBuffer
        void SaveDataBuffer(const SaveDataRequests::SaveDataBufferParams& saveDataBufferParams) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref SaveData::SaveDataSystemComponent::Implementation::LoadDataBuffer
        void LoadDataBuffer(const SaveDataRequests::LoadDataBufferParams& loadDataBufferParams) override;

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! \ref SaveData::SaveDataSystemComponent::Implementation::SetSaveDataDirectoryPath
        void SetSaveDataDirectoryPath(const char* saveDataDirectoryPath) override;

    private:
        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Convenience function to construct the full save data file path.
        //! \param[in] dataBufferName The name of the save data buffer.
        //! \param[in] localUserId The local user id the save data buffer is associated with.
        AZ::IO::Path GetSaveDataFilePath(const AZStd::string& dataBufferName,
                                         AzFramework::LocalUserId localUserId);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The absolute path to the application's save data dircetory.
        AZ::IO::Path m_saveDataDirectoryPathAbsolute;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::IO::Path GetDefaultLinuxUserSaveDataPath()
    {
        // First priority for the home directory is the 'HOME' environment variable
        const char* homeDir = getenv("HOME");
        if (homeDir == nullptr)
        {
            // If the 'HOME' environment variable is not set, then retrieve it from the 'getpwuid' 
            // system call
            auto uid = getuid();
            auto pwuid = getpwuid(uid);
            homeDir = pwuid->pw_dir;
        }

        AZ_Assert(homeDir, "Unable to determine home directory for current Linux user");
        if (homeDir == nullptr)
        {
            homeDir = "/tmp";
        }

        AZ::IO::Path homePath {homeDir};

        // $HOME/.local/share is the standard directory where user data is stored on Ubuntu
        return homePath / ".local" / "share";
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZStd::string GetExecutableName()
    {
        char moduleFileName[AZ_MAX_PATH_LEN];
        AZ::Utils::GetExecutablePath(moduleFileName, AZ_MAX_PATH_LEN);

        AZ::IO::Path executableFullPath {moduleFileName};
        AZStd::string moduleFileNameString {executableFullPath.Filename().Native()};
        return moduleFileNameString;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    SaveDataSystemComponent::Implementation* SaveDataSystemComponent::Implementation::Create(SaveDataSystemComponent& saveDataSystemComponent)
    {
        return aznew SaveDataSystemComponentLinux(saveDataSystemComponent);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    SaveDataSystemComponentLinux::SaveDataSystemComponentLinux(SaveDataSystemComponent& saveDataSystemComponent)
        : SaveDataSystemComponent::Implementation(saveDataSystemComponent)
        , m_saveDataDirectoryPathAbsolute(GetDefaultLinuxUserSaveDataPath() /
                                          GetExecutableName().c_str() /
                                          DefaultSaveDataDirectoryName)
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    SaveDataSystemComponentLinux::~SaveDataSystemComponentLinux()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void SaveDataSystemComponentLinux::SaveDataBuffer(const SaveDataRequests::SaveDataBufferParams& saveDataBufferParams)
    {
        const AZStd::string absoluteFilePath = GetSaveDataFilePath(saveDataBufferParams.dataBufferName,
                                                                   saveDataBufferParams.localUserId).c_str();
        SaveDataBufferToFileSystem(saveDataBufferParams, absoluteFilePath);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void SaveDataSystemComponentLinux::LoadDataBuffer(const SaveDataRequests::LoadDataBufferParams& loadDataBufferParams)
    {
        const AZStd::string absoluteFilePath = GetSaveDataFilePath(loadDataBufferParams.dataBufferName,
                                                                   loadDataBufferParams.localUserId).c_str();
        LoadDataBufferFromFileSystem(loadDataBufferParams, absoluteFilePath);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void SaveDataSystemComponentLinux::SetSaveDataDirectoryPath(const char* saveDataDirectoryPath)
    {
        AZ::IO::Path saveDataDirectoryBasicPath { saveDataDirectoryPath };

        if (saveDataDirectoryBasicPath.IsAbsolute())
        {
            m_saveDataDirectoryPathAbsolute = saveDataDirectoryBasicPath;
        }
        else
        {
            m_saveDataDirectoryPathAbsolute = GetDefaultLinuxUserSaveDataPath() / saveDataDirectoryBasicPath;
        }

        AZ_Assert(!m_saveDataDirectoryPathAbsolute.empty(), "Cannot set an empty save data directory path.");
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZ::IO::Path SaveDataSystemComponentLinux::GetSaveDataFilePath(const AZStd::string& dataBufferName,
                                                                   AzFramework::LocalUserId localUserId)
    {
        AZ::IO::Path saveDataFilePath = m_saveDataDirectoryPathAbsolute;
        if (localUserId != AzFramework::LocalUserIdNone)
        {
            saveDataFilePath /= AZStd::string::format("User_%u", localUserId);
        }
        saveDataFilePath /= dataBufferName;
        return saveDataFilePath;
    }
} // namespace SaveData
