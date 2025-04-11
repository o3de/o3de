/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <RHI/PhysicalDevice.h>
#include <Atom/RHI/ValidationLayer.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI.Loader/LoaderContext.h>

#if defined(USE_NSIGHT_AFTERMATH)
#include <RHI/NsightAftermathGpuCrashTracker_Windows.h>
#endif

namespace AZ
{
    namespace Vulkan
    {
        class Instance final
        {
        public:
            AZ_CLASS_ALLOCATOR(Instance, AZ::SystemAllocator);

            struct Descriptor
            {
                RawStringList m_requiredLayers;
                RawStringList m_optionalLayers;
                RawStringList m_requiredExtensions;
                RawStringList m_optionalExtensions;
                AZ::RHI::ValidationMode m_validationMode = RHI::ValidationMode::Disabled;
            };

            static Instance& GetInstance();
            static void Reset();
            ~Instance();
            bool Init(const Descriptor& descriptor);
            void Shutdown();

            VkInstance GetNativeInstance()
            {
                return m_instance;
            }
            GladVulkanContext& GetContext()
            {
                return m_loaderContext->GetContext();
            }
            const Descriptor& GetDescriptor() const;
            RHI::PhysicalDeviceList GetSupportedDevices() const;
            AZ::RHI::ValidationMode GetValidationMode() const;

            //! Returns the list of layers loaded by the Vulkan instance.
            const RawStringList& GetLoadedLayers() const;
            //! Retuns the list of instance extensions loaded by the Vulkan instance.
            const RawStringList& GetLoadedExtensions() const;

            //! Returns the App Info used for creting the instance.
            const VkApplicationInfo& GetVkAppInfo() const;

        private:
            RHI::PhysicalDeviceList EnumerateSupportedDevices(uint32_t minVersion) const;
            void ShutdownNativeInstance();
            void CreateDebugMessenger();

            Descriptor m_descriptor;
            VkInstance m_instance = VK_NULL_HANDLE;
            AZStd::unique_ptr<LoaderContext> m_loaderContext;
            RHI::PhysicalDeviceList m_supportedDevices;
            VkInstanceCreateInfo m_instanceCreateInfo = {};
            VkApplicationInfo m_appInfo = {};
            uint32_t m_instanceVersion = 0;

#if defined(USE_NSIGHT_AFTERMATH)
            GpuCrashTracker m_gpuCrashHandler;
#endif
        };
    } // namespace Vulkan
} // namespace AZ
