/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <vulkan/vulkan.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <RHI/PhysicalDevice.h>
#include <Atom/RHI/ValidationLayer.h>
#include <Atom/RHI/RHISystemInterface.h>

#if defined(USE_NSIGHT_AFTERMATH)
#include <RHI/NsightAftermathGpuCrashTracker_Windows.h>
#endif

namespace AZ
{
    namespace Vulkan
    {
        class FunctionLoader;

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
                return m_context;
            }
            const Descriptor& GetDescriptor() const;
            FunctionLoader& GetFunctionLoader()
            {
                return *m_functionLoader;
            }
            StringList GetInstanceLayerNames() const;
            StringList GetInstanceExtensionNames(const char* layerName = nullptr) const;
            RHI::PhysicalDeviceList GetSupportedDevices() const;
            AZ::RHI::ValidationMode GetValidationMode() const;

            //! Since the native instance is created when the RHI::Vulkan module was initialized
            //! this method allows us to re-update it if XR support is requested by XR module.
            void UpdateNativeInstance(RHI::XRRenderingInterface* xrSystem);

        private:
            RHI::PhysicalDeviceList EnumerateSupportedDevices() const;
            void ShutdownNativeInstance();
            void CreateDebugMessenger();

            Descriptor m_descriptor;
            VkInstance m_instance = VK_NULL_HANDLE;
            GladVulkanContext m_context = {};
            AZStd::unique_ptr<FunctionLoader> m_functionLoader;
            RHI::PhysicalDeviceList m_supportedDevices;
            VkInstanceCreateInfo m_instanceCreateInfo = {};
            VkApplicationInfo m_appInfo = {};
            bool m_isXRInstanceCreated = false;

#if defined(USE_NSIGHT_AFTERMATH)
            GpuCrashTracker m_gpuCrashHandler;
#endif
        };
    } // namespace Vulkan
} // namespace AZ
