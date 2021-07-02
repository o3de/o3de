/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/Path/Path.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Module/DynamicModuleHandle.h>
#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/PlatformIncl.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Utils/Utils.h>

namespace AZ
{
    class DynamicModuleHandleWindows
        : public DynamicModuleHandle
    {
    public:
        AZ_CLASS_ALLOCATOR(DynamicModuleHandleWindows, OSAllocator, 0);

        DynamicModuleHandleWindows(const char* fullFileName)
            : DynamicModuleHandle(fullFileName)
            , m_handle(nullptr)
        {
            // Ensure filename ends in ".dll"
            // Otherwise filenames like "gem.1.0.0" fail to load (.0 is assumed to be the extension).
            if (!m_fileName.ends_with(AZ_TRAIT_OS_DYNAMIC_LIBRARY_EXTENSION))
            {
                m_fileName += AZ_TRAIT_OS_DYNAMIC_LIBRARY_EXTENSION;
            }

            AZ::IO::PathView modulePathView{ m_fileName };
            // If the module path doesn't have a directory within it, prepend the executable directory to the path
            // and check if the new path exist
            if (modulePathView.HasFilename() && !modulePathView.HasParentPath())
            {
                char exeDirectory[AZ::IO::MaxPathLength];
                if (AZ::Utils::GetExecutableDirectory(exeDirectory, AZ_ARRAY_SIZE(exeDirectory)) ==
                    AZ::Utils::ExecutablePathResult::Success)
                {
                    AZ::IO::FixedMaxPath candidatePath = exeDirectory;
                    candidatePath /= modulePathView;
                    // Check to see if the module exist before assigning it as the file name
                    if (AZ::IO::SystemFile::Exists(candidatePath.c_str()))
                    {
                        m_fileName.assign(candidatePath.Native().c_str(), candidatePath.Native().size());
                        return;
                    }
                }
            }

            // If the module file path does not exist, attempt to search for the module within
            // the project's build directory
            if (!AZ::IO::SystemFile::Exists(m_fileName.c_str()))
            {
                // The Settings Registry may not exist in early startup if modules are loaded
                // before the ComponentApplication is crated(such as in the Editor main.cpp)
                // Therefore an existence check is needed
                if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
                {
                    if (AZ::IO::FixedMaxPath projectModulePath;
                        settingsRegistry->Get(projectModulePath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectConfigurationBinPath))
                    {
                        projectModulePath /= AZStd::string_view(m_fileName);
                        if (AZ::IO::SystemFile::Exists(projectModulePath.c_str()))
                        {
                            m_fileName.assign(projectModulePath.c_str(), projectModulePath.Native().size());
                        }
                    }
                }
            }
            else
            {
                // The module does exist (in 'cwd'), but still needs to be an absolute path for the module to be loaded.
                AZStd::optional<AZ::IO::FixedMaxPathString> absPathOptional = AZ::Utils::ConvertToAbsolutePath(m_fileName);
                if (absPathOptional.has_value())
                {
                    m_fileName.assign(absPathOptional->c_str(), absPathOptional->size());
                }
            }
        }

        ~DynamicModuleHandleWindows() override
        {
            Unload();
        }

        LoadStatus LoadModule() override
        {
            if (IsLoaded())
            {
                return LoadStatus::AlreadyLoaded;
            }

            bool alreadyLoaded = false;

#   ifdef _UNICODE
            wchar_t fileNameW[MAX_PATH];
            size_t numCharsConverted;
            errno_t wcharResult = mbstowcs_s(&numCharsConverted, fileNameW, m_fileName.c_str(), AZ_ARRAY_SIZE(fileNameW) - 1);
            if (wcharResult == 0)
            {
                // If module already open, return false, it was not loaded.
                alreadyLoaded = NULL != GetModuleHandleW(fileNameW);
                m_handle = LoadLibraryW(fileNameW);
            }
            else
            {
                AZ_Assert(false, "Failed to convert %s to wchar with error %d!", m_fileName.c_str(), wcharResult);
                m_handle = NULL;
                return LoadStatus::LoadFailure;
            }
#   else //!_UNICODE
            alreadyLoaded = NULL != GetModuleHandleA(m_fileName.c_str());
            m_handle = LoadLibraryA(m_fileName.c_str());
#   endif // !_UNICODE

            if (m_handle)
            {
                if (alreadyLoaded)
                {
                    return LoadStatus::AlreadyLoaded;
                }
                else
                {
                    return LoadStatus::LoadSuccess;
                }
            }
            else
            {
                return LoadStatus::LoadFailure;
            }
        }

        bool UnloadModule() override
        {
            bool result = false;
            if (m_handle)
            {
                result = FreeLibrary(m_handle) ? true : false;
                m_handle = 0;
            }
            return result;
        }

        bool IsLoaded() const override
        {
            return m_handle ? true : false;
        }

        void* GetFunctionAddress(const char* functionName) const override
        {
            return reinterpret_cast<void*>(::GetProcAddress(m_handle, functionName));
        }

        HMODULE m_handle;
    };

    // Implement the module creation function
    AZStd::unique_ptr<DynamicModuleHandle> DynamicModuleHandle::Create(const char* fullFileName)
    {
        return AZStd::unique_ptr<DynamicModuleHandle>(aznew DynamicModuleHandleWindows(fullFileName));
    }
} // namespace AZ
