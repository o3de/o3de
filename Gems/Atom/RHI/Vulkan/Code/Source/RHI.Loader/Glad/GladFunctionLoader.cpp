/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#define GLAD_VULKAN_IMPLEMENTATION
#include <vulkan/vulkan.h>

#include <RHI.Loader/Glad/GladFunctionLoader.h>
#include <AzCore/Module/DynamicModuleHandle.h>

namespace
{

    GLADapiproc LoadFunctionFromLibrary(void* userPtr, const char *name)
    {
        AZ::DynamicModuleHandle* moduleHandle = reinterpret_cast<AZ::DynamicModuleHandle*>(userPtr);
        AZ_Assert(moduleHandle, "Invalid module handle");
        return moduleHandle->GetFunction<GLADapiproc>(name);
    }
}

namespace AZ
{
    namespace Vulkan
    {
        AZStd::unique_ptr<FunctionLoader> FunctionLoader::Create()
        {
            return AZStd::make_unique<GladFunctionLoader>();
        }

        bool GladFunctionLoader::InitInternal()
        {
            // Since que don't have the vulkan instance or device yet, we just load the function pointers from the loader
            // using dlsym or similar.
            return gladLoadVulkanUserPtr(VK_NULL_HANDLE, &LoadFunctionFromLibrary, m_moduleHandle.get()) != 0;
        }

        void GladFunctionLoader::ShutdownInternal()
        {
            gladLoaderUnloadVulkan();
        }

        bool GladFunctionLoader::LoadProcAddresses(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device)
        {
            int gladResult = 0;
            if (m_device == VK_NULL_HANDLE || m_device == device)
            {
                // Now that we have the instance and device we can get the function pointer directly from the ICD without going trough the loader's trampoline.
                // Basically we use vkGetInstanceProcAddr and vkGetDeviceProcAddr to get the function address.
                gladResult = gladLoaderLoadVulkan(instance, physicalDevice, device);
                m_device = device;
            }
            else
            {
                // [ATOM-338] Find a better way to handle loading function pointers for multiple devices.
                // Currently we just fallback to using the loader, and let it handle the multi device situation.
                // Unfortunately this add the overhead of the trampoline/terminator when calling any vulkan function.
                gladResult = gladLoadVulkanUserPtr(physicalDevice, &LoadFunctionFromLibrary, m_moduleHandle.get());
            }
            return gladResult != 0;
        }
    } // namespace Vulkan
} // namespace AZ
