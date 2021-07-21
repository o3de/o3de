/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <vulkan/vulkan.h>
#include <Atom/RHI.Loader/FunctionLoader.h>
#include <AzCore/Module/DynamicModuleHandle.h>
#include <AzCore/std/containers/vector.h>
#include <Vulkan_Traits_Platform.h>

namespace AZ
{
    namespace Vulkan
    {
        FunctionLoader::~FunctionLoader()
        {
            AZ_Assert(!m_moduleHandle, "Shutdown was not called before destroying this FunctionLoader");
        }

        bool FunctionLoader::Init()
        {
            const AZStd::vector<const char*> libs = {
                AZ_TRAIT_ATOM_VULKAN_DLL,
                AZ_TRAIT_ATOM_VULKAN_DLL_1,
            };

            for (const char* libName : libs)
            {
                m_moduleHandle = AZ::DynamicModuleHandle::Create(libName);
                if (m_moduleHandle->Load(false))
                {
                    break;
                }
                else
                {
                    m_moduleHandle = nullptr;
                }
            }

            if (!m_moduleHandle)
            {
                AZ_Warning("Vulkan", false, "Could not find Vulkan library.");
                return false;
            }
            
            return InitInternal();
        }

        void FunctionLoader::Shutdown()
        {
            ShutdownInternal();
            if (m_moduleHandle)
            {
                m_moduleHandle->Unload();
            }
            m_moduleHandle = nullptr;
        }
    } // namespace Vulkan
} // namespace AZ
