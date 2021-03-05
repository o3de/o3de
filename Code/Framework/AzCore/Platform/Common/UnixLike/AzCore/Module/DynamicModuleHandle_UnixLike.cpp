/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzCore/Module/DynamicModuleHandle.h>
#include <AzCore/IO/SystemFile.h> // for AZ_MAX_PATH_LEN

#include <AzCore/Memory/OSAllocator.h>
#include <dlfcn.h>
#include <libgen.h>

namespace AZ
{
    namespace Platform
    {
        void GetModulePath(AZ::OSString& path);
        void* OpenModule(const AZ::OSString& fileName, bool& alreadyOpen);
        void ConstructModuleFullFileName(const AZ::OSString& path, const AZ::OSString& fileName, AZ::OSString& fullPath);
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
            AZ::OSString path;
            AZ::OSString fileName;
            AZ::OSString fullPath = "";
            AZ::OSString::size_type finalSlash = m_fileName.find_last_of("/");
            if (finalSlash != AZ::OSString::npos)
            {
                // Path up to and including final slash
                path = m_fileName.substr(0, finalSlash + 1);
                // Everything after the final slash
                // If m_fileName ends in /, the end result is path/lib.dylib, which just fails to load.
                fileName = m_fileName.substr(finalSlash + 1);
            }
            else
            {
                // If no slash found, assume empty path, only file name
                path = "";
                Platform::GetModulePath(path);
                fileName = m_fileName;
            }

            if (fileName.substr(0, 3) != AZ_TRAIT_OS_DYNAMIC_LIBRARY_PREFIX)
            {
                fileName = AZ_TRAIT_OS_DYNAMIC_LIBRARY_PREFIX + fileName;
            }

            size_t extensionLen = strlen(AZ_TRAIT_OS_DYNAMIC_LIBRARY_EXTENSION);
            if (fileName.substr(fileName.length() - extensionLen, extensionLen) != AZ_TRAIT_OS_DYNAMIC_LIBRARY_EXTENSION)
            {
                fileName = fileName + AZ_TRAIT_OS_DYNAMIC_LIBRARY_EXTENSION;
            }
            
            Platform::ConstructModuleFullFileName(path, fileName, fullPath);

            m_fileName = fullPath;
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
