/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Module/DynamicModuleHandle.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/Utils/Utils.h>

#include <dlfcn.h>
#include <libgen.h>

namespace AZ
{
    namespace Platform
    {
        AZ::IO::FixedMaxPath GetModulePath();
        void* OpenModule(const AZ::OSString& fileName, bool& alreadyOpen);
        void ConstructModuleFullFileName(AZ::IO::FixedMaxPath& fullPath);
    }

    class DynamicModuleHandleUnixLike
        : public DynamicModuleHandle
    {
    public:
        AZ_CLASS_ALLOCATOR(DynamicModuleHandleUnixLike, OSAllocator, 0)

        DynamicModuleHandleUnixLike(const char* fullFileName)
            : DynamicModuleHandle(fullFileName)
            , m_handle(nullptr)
        {
            AZ::IO::FixedMaxPath fullFilePath(AZStd::string_view{m_fileName});
            if (fullFilePath.HasFilename())
            {
                AZ::IO::FixedMaxPathString fileNamePath{fullFilePath.Filename().Native()};
                if (!fileNamePath.starts_with(AZ_TRAIT_OS_DYNAMIC_LIBRARY_PREFIX))
                {
                    fileNamePath = AZ_TRAIT_OS_DYNAMIC_LIBRARY_PREFIX + fileNamePath;
                }

                if (!fileNamePath.ends_with(AZ_TRAIT_OS_DYNAMIC_LIBRARY_EXTENSION))
                {
                    fileNamePath += AZ_TRAIT_OS_DYNAMIC_LIBRARY_EXTENSION;
                }

                fullFilePath.ReplaceFilename(AZStd::string_view(fileNamePath));
            }

            Platform::ConstructModuleFullFileName(fullFilePath);

            // Check if the module exist at the given path within the current working directory
            // If it doesn't attempt to append the path to the executable path
            if (!AZ::IO::SystemFile::Exists(fullFilePath.c_str()))
            {
                AZ::IO::FixedMaxPath candidatePath = Platform::GetModulePath() / fullFilePath;
                if (AZ::IO::SystemFile::Exists(candidatePath.c_str()))
                {
                    m_fileName.assign(candidatePath.Native().c_str(), candidatePath.Native().size());
                    return;
                }
            }

            // If the path still doesn't exist at this point, check the SettingsRegistryMergeUtils
            // FilePathKey_ProjectBuildPath key to see if a project-build-path argument has been supplied
            if (!AZ::IO::SystemFile::Exists(fullFilePath.c_str()))
            {
                if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
                {
                    bool fileFound = false;
                    if (AZ::IO::FixedMaxPath projectModulePath;
                        settingsRegistry->Get(projectModulePath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectConfigurationBinPath))
                    {
                        projectModulePath /= fullFilePath;
                        if (AZ::IO::SystemFile::Exists(projectModulePath.c_str()))
                        {
                            m_fileName.assign(projectModulePath.c_str(), projectModulePath.Native().size());
                            fileFound = true;
                        }
                    }
                    if (!fileFound)
                    {
                        if (AZ::IO::FixedMaxPath installedBinariesPath;
                            settingsRegistry->Get(installedBinariesPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_InstalledBinaryFolder))
                        {
                            if (AZ::IO::FixedMaxPath engineRootFolder;
                                settingsRegistry->Get(engineRootFolder.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder))
                            {
                                installedBinariesPath = engineRootFolder / installedBinariesPath / fullFilePath;
                                if (AZ::IO::SystemFile::Exists(installedBinariesPath.c_str()))
                                {
                                    m_fileName.assign(installedBinariesPath.c_str(), installedBinariesPath.Native().size());
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                // The module does exist (in 'cwd'), but still needs to be an absolute path for the module to be loaded.
                AZStd::optional<AZ::IO::FixedMaxPathString> absPathOptional = AZ::Utils::ConvertToAbsolutePath(fullFilePath.c_str());
                if (absPathOptional.has_value())
                {
                    m_fileName.assign(absPathOptional->c_str(), absPathOptional->size());
                }
            }
        }

        ~DynamicModuleHandleUnixLike() override
        {
            Unload();
        }

        LoadStatus LoadModule() override
        {
            AZ::Debug::Trace::Printf("Module", "Attempting to load module:%s\n", m_fileName.c_str());
            bool alreadyOpen = false;

            m_handle = Platform::OpenModule(m_fileName, alreadyOpen);

            if(m_handle)
            {
                if (alreadyOpen)
                {
                    AZ::Debug::Trace::Printf("Module", "Success! System already had it opened.\n");
                    // We want to return LoadSuccess and not AlreadyLoaded
                    // because the system may have already loaded the DLL (because
                    // it is a dependency of another library we have loaded) but
                    // we have not run our initialization routine on it. Returning
                    // AlreadyLoaded stops us from running the initialization
                    // routine that results in a cascade of failures and potential
                    // crash.
                    return LoadStatus::LoadSuccess;
                }
                else
                {
                    AZ::Debug::Trace::Printf("Module", "Success!\n");
                    return LoadStatus::LoadSuccess;
                }
            }
            else
            {
                AZ::Debug::Trace::Printf("Module", "Failed with error:\n%s\n", dlerror());
                return LoadStatus::LoadFailure;
            }
        }

        bool UnloadModule() override
        {
            bool result = false;
            if (m_handle)
            {
                result = dlclose(m_handle) == 0 ? true : false;
                m_handle = nullptr;
            }
            return result;
        }

        bool IsLoaded() const override
        {
            return m_handle ? true : false;
        }

        void* GetFunctionAddress(const char* functionName) const override
        {
            return dlsym(m_handle, functionName);
        }

        void* m_handle;
    };

    // Implement the module creation function
    AZStd::unique_ptr<DynamicModuleHandle> DynamicModuleHandle::Create(const char* fullFileName)
    {
        return AZStd::unique_ptr<DynamicModuleHandle>(aznew DynamicModuleHandleUnixLike(fullFileName));
    }
} // namespace AZ
