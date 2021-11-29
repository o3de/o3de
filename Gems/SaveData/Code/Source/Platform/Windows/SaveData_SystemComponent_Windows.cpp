/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SaveDataSystemComponent.h>

#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/Utils/Utils.h>

#include <AzCore/PlatformIncl.h>
#include <shlobj.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace SaveData
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    //! Platform specific implementation for the save data system component on Windows
    class SaveDataSystemComponentWindows : public SaveDataSystemComponent::Implementation
    {
    public:
        ////////////////////////////////////////////////////////////////////////////////////////////
        static constexpr const char* DefaultSaveDataDirectoryName = "SaveData";

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Allocator
        AZ_CLASS_ALLOCATOR(SaveDataSystemComponentWindows, AZ::SystemAllocator, 0);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Constructor
        //! \param[in] saveDataSystemComponent Reference to the parent being implemented
        SaveDataSystemComponentWindows(SaveDataSystemComponent& saveDataSystemComponent);

        ////////////////////////////////////////////////////////////////////////////////////////////
        //! Destructor
        ~SaveDataSystemComponentWindows() override;

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
    AZStd::string GetDefaultWindowsUserSaveDataPath()
    {
        // Unfortunately, there is no universally accepted default "Save Data" directory on Windows,
        // so we are forced to choose between the following commonly used user save data locations:
        //
        // C:\Users\{username}\AppData\Local    (FOLDERID_LocalAppData)
        // C:\Users\{username}\AppData\Roaming  (FOLDERID_RoamingAppData)
        // C:\Users\{username}\Documents        (FOLDERID_Documents)
        // C:\Users\{username}\Saved Games      (FOLDERID_SavedGames)
        //
        // which are all best retrieved using the Windows SHGetKnownFolderPath function:
        // https://docs.microsoft.com/en-us/windows/desktop/api/shlobj_core/nf-shlobj_core-shgetknownfolderpath

        // Get the 'known folder path'
        wchar_t* knownFolderPathUTF16 = nullptr;
        [[maybe_unused]] long result = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &knownFolderPathUTF16);
        AZ_Assert(SUCCEEDED(result), "SHGetKnownFolderPath could not retrieve LocalAppData folder");

        // Convert it from UTF-16 to UTF-8
        AZStd::string defaultWindowsUserSaveDataPathUTF8;
        AZStd::to_string(defaultWindowsUserSaveDataPathUTF8, AZStd::wstring(knownFolderPathUTF16));

        // Free the memory allocated by SHGetKnownFolderPath
        CoTaskMemFree(knownFolderPathUTF16);

        // Append the trailing path separator and return
        defaultWindowsUserSaveDataPathUTF8 += '\\';
        return defaultWindowsUserSaveDataPathUTF8;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZStd::string GetExecutableName()
    {
        char moduleFileName[AZ_MAX_PATH_LEN];
        AZ::Utils::GetExecutablePath(moduleFileName, AZ_MAX_PATH_LEN);

        const AZStd::string moduleFileNameString(moduleFileName);
        const size_t executableNameStart = moduleFileNameString.find_last_of('\\') + 1;
        const size_t executableNameEnd = moduleFileNameString.find_last_of('.');
        const size_t executableNameLength = executableNameEnd - executableNameStart;

        AZ_Assert(executableNameLength > 0, "Could not extract executable name from: %s", moduleFileName);
        return moduleFileNameString.substr(executableNameStart, executableNameLength);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool IsAbsolutePath(const char* path)
    {
        char drive[16];
        _splitpath_s(path, drive, 16, nullptr, 0, nullptr, 0, nullptr, 0);
        return strlen(drive) > 0;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    SaveDataSystemComponent::Implementation* SaveDataSystemComponent::Implementation::Create(SaveDataSystemComponent& saveDataSystemComponent)
    {
        return aznew SaveDataSystemComponentWindows(saveDataSystemComponent);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    SaveDataSystemComponentWindows::SaveDataSystemComponentWindows(SaveDataSystemComponent& saveDataSystemComponent)
        : SaveDataSystemComponent::Implementation(saveDataSystemComponent)
        , m_saveDataDircetoryPathAbsolute(GetDefaultWindowsUserSaveDataPath() +
                                          GetExecutableName() + "\\" +
                                          DefaultSaveDataDirectoryName + "\\")
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    SaveDataSystemComponentWindows::~SaveDataSystemComponentWindows()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void SaveDataSystemComponentWindows::SaveDataBuffer(const SaveDataRequests::SaveDataBufferParams& saveDataBufferParams)
    {
        const AZStd::string& absoluteFilePath = GetSaveDataFilePath(saveDataBufferParams.dataBufferName,
                                                                    saveDataBufferParams.localUserId);
        SaveDataBufferToFileSystem(saveDataBufferParams, absoluteFilePath);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void SaveDataSystemComponentWindows::LoadDataBuffer(const SaveDataRequests::LoadDataBufferParams& loadDataBufferParams)
    {
        const AZStd::string& absoluteFilePath = GetSaveDataFilePath(loadDataBufferParams.dataBufferName,
                                                                    loadDataBufferParams.localUserId);
        LoadDataBufferFromFileSystem(loadDataBufferParams, absoluteFilePath);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void SaveDataSystemComponentWindows::SetSaveDataDirectoryPath(const char* saveDataDirectoryPath)
    {
        if (IsAbsolutePath(saveDataDirectoryPath))
        {
            m_saveDataDircetoryPathAbsolute = saveDataDirectoryPath;
        }
        else
        {
            m_saveDataDircetoryPathAbsolute = GetDefaultWindowsUserSaveDataPath() + saveDataDirectoryPath;
        }

        AZ_Assert(!m_saveDataDircetoryPathAbsolute.empty(), "Cannot set an empty save data directory path.");

        // Append the trailing path separator if needed
        if (m_saveDataDircetoryPathAbsolute.back() != '/' ||
            m_saveDataDircetoryPathAbsolute.back() != '\\')
        {
            m_saveDataDircetoryPathAbsolute += '\\';
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    AZStd::string SaveDataSystemComponentWindows::GetSaveDataFilePath(const AZStd::string& dataBufferName,
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
