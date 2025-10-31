/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SaveDataSystemComponent.h>

#include <AzCore/Android/Utils.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/string/conversions.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace SaveData
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Platform specific implementation for the save data system component on Android
    class SaveDataSystemComponentAndroid : public SaveDataSystemComponent::Implementation
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        static constexpr const char* DefaultSaveDataDirectoryName = "SaveData";

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(SaveDataSystemComponentAndroid, AZ::SystemAllocator);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] saveDataSystemComponent Reference to the parent being implemented
        SaveDataSystemComponentAndroid(SaveDataSystemComponent& saveDataSystemComponent);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destructor
        ~SaveDataSystemComponentAndroid() override;

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
    AZStd::string GetDefaultAndroidUserSaveDataPath()
    {
        return AZStd::string::format("%s/", AZ::Android::Utils::GetAppPublicStoragePath());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool IsAbsolutePath(const char* path)
    {
        return path && path[0] == '/';
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    SaveDataSystemComponent::Implementation* SaveDataSystemComponent::Implementation::Create(SaveDataSystemComponent& saveDataSystemComponent)
    {
        return aznew SaveDataSystemComponentAndroid(saveDataSystemComponent);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    SaveDataSystemComponentAndroid::SaveDataSystemComponentAndroid(SaveDataSystemComponent& saveDataSystemComponent)
        : SaveDataSystemComponent::Implementation(saveDataSystemComponent)
        , m_saveDataDircetoryPathAbsolute(GetDefaultAndroidUserSaveDataPath() +
                                          DefaultSaveDataDirectoryName + "/")
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    SaveDataSystemComponentAndroid::~SaveDataSystemComponentAndroid()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void SaveDataSystemComponentAndroid::SaveDataBuffer(const SaveDataRequests::SaveDataBufferParams& saveDataBufferParams)
    {
        const AZStd::string& absoluteFilePath = GetSaveDataFilePath(saveDataBufferParams.dataBufferName,
                                                                    saveDataBufferParams.localUserId);
        SaveDataBufferToFileSystem(saveDataBufferParams, absoluteFilePath);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void SaveDataSystemComponentAndroid::LoadDataBuffer(const SaveDataRequests::LoadDataBufferParams& loadDataBufferParams)
    {
        const AZStd::string& absoluteFilePath = GetSaveDataFilePath(loadDataBufferParams.dataBufferName,
                                                                    loadDataBufferParams.localUserId);
        LoadDataBufferFromFileSystem(loadDataBufferParams, absoluteFilePath);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void SaveDataSystemComponentAndroid::SetSaveDataDirectoryPath(const char* saveDataDirectoryPath)
    {
        if (IsAbsolutePath(saveDataDirectoryPath))
        {
            m_saveDataDircetoryPathAbsolute = saveDataDirectoryPath;
        }
        else
        {
            m_saveDataDircetoryPathAbsolute = GetDefaultAndroidUserSaveDataPath() + saveDataDirectoryPath;
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
    AZStd::string SaveDataSystemComponentAndroid::GetSaveDataFilePath(const AZStd::string& dataBufferName,
                                                                      AzFramework::LocalUserId localUserId)
    {
        AZStd::string saveDataFilePath = m_saveDataDircetoryPathAbsolute;
        if (localUserId != AzFramework::LocalUserIdNone)
        {
            saveDataFilePath += AZStd::string::format("User_%u/", localUserId);
        }
        saveDataFilePath += dataBufferName;
        return saveDataFilePath;
    }
} // namespace SaveData
