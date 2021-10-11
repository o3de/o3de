/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SaveDataSystemComponent.h>

#include <AzCore/IO/SystemFile.h>

#include <Foundation/NSBundle.h>
#include <Foundation/NSPathUtilities.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace SaveData
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Platform specific implementation for the save data system component on iOS and macOS
    class SaveDataSystemComponentApple : public SaveDataSystemComponent::Implementation
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        static constexpr const char* DefaultSaveDataDirectoryName = "SaveData";

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(SaveDataSystemComponentApple, AZ::SystemAllocator, 0);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] saveDataSystemComponent Reference to the parent being implemented
        SaveDataSystemComponentApple(SaveDataSystemComponent& saveDataSystemComponent);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destructor
        ~SaveDataSystemComponentApple() override;

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
        AZStd::string GetSaveDataFilePath(const AZStd::string& dataBufferName,
                                          AzFramework::LocalUserId localUserId);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! The absolute path to the application's save data dircetory.
        AZStd::string m_saveDataDircetoryPathAbsolute;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZStd::string GetDefaultAppleUserSaveDataPath()
    {
        AZStd::string returnValue;
        NSArray* paths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory,
                                                             NSUserDomainMask,
                                                             YES);
        if ([paths count] != 0)
        {
            returnValue = [[paths objectAtIndex:0] UTF8String];
        }
        returnValue += '/';
        return returnValue;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZStd::string GetExecutableName()
    {
        const AZStd::string bundlePathString = [[[NSBundle mainBundle] bundlePath] UTF8String];
        const size_t executableNameStart = bundlePathString.find_last_of('/') + 1;
        const size_t executableNameEnd = bundlePathString.find_last_of('.');
        const size_t executableNameLength = executableNameEnd - executableNameStart;

        AZ_Assert(executableNameLength > 0, "Could not extract executable name from: %s", bundlePathString.c_str());
        return bundlePathString.substr(executableNameStart, executableNameLength);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool IsAbsolutePath(const char* path)
    {
        return path && path[0] == '/';
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    SaveDataSystemComponent::Implementation* SaveDataSystemComponent::Implementation::Create(SaveDataSystemComponent& saveDataSystemComponent)
    {
        return aznew SaveDataSystemComponentApple(saveDataSystemComponent);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    SaveDataSystemComponentApple::SaveDataSystemComponentApple(SaveDataSystemComponent& saveDataSystemComponent)
        : SaveDataSystemComponent::Implementation(saveDataSystemComponent)
        , m_saveDataDircetoryPathAbsolute(GetDefaultAppleUserSaveDataPath() +
                                          GetExecutableName() + "/" +
                                          DefaultSaveDataDirectoryName + "/")
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    SaveDataSystemComponentApple::~SaveDataSystemComponentApple()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void SaveDataSystemComponentApple::SaveDataBuffer(const SaveDataRequests::SaveDataBufferParams& saveDataBufferParams)
    {
        const AZStd::string& absoluteFilePath = GetSaveDataFilePath(saveDataBufferParams.dataBufferName,
                                                                    saveDataBufferParams.localUserId);
        SaveDataBufferToFileSystem(saveDataBufferParams, absoluteFilePath);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void SaveDataSystemComponentApple::LoadDataBuffer(const SaveDataRequests::LoadDataBufferParams& loadDataBufferParams)
    {
        const AZStd::string& absoluteFilePath = GetSaveDataFilePath(loadDataBufferParams.dataBufferName,
                                                                    loadDataBufferParams.localUserId);
        LoadDataBufferFromFileSystem(loadDataBufferParams, absoluteFilePath);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void SaveDataSystemComponentApple::SetSaveDataDirectoryPath(const char* saveDataDirectoryPath)
    {
        if (IsAbsolutePath(saveDataDirectoryPath))
        {
            m_saveDataDircetoryPathAbsolute = saveDataDirectoryPath;
        }
        else
        {
            m_saveDataDircetoryPathAbsolute = GetDefaultAppleUserSaveDataPath() + saveDataDirectoryPath;
        }

        AZ_Assert(!m_saveDataDircetoryPathAbsolute.empty(), "Cannot set an empty save data directory path.");

        // Append the trailing path separator if needed
        if (m_saveDataDircetoryPathAbsolute.back() != '/' ||
            m_saveDataDircetoryPathAbsolute.back() != '\\')
        {
            m_saveDataDircetoryPathAbsolute += '/';
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZStd::string SaveDataSystemComponentApple::GetSaveDataFilePath(const AZStd::string& dataBufferName,
                                                                    AzFramework::LocalUserId localUserId)
    {
        AZStd::string saveDataFilePath = m_saveDataDircetoryPathAbsolute;
        if (localUserId != AzFramework::LocalUserIdNone)
        {
            saveDataFilePath += AZStd::string::format("User_%u\\", localUserId);
        }
        saveDataFilePath += dataBufferName;
        return saveDataFilePath;
    }
} // namespace SaveData
