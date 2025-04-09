/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Base.h>
#include <Atom/RHI/PhysicalDevice.h>
#include <AzCore/Debug/Budget.h>
#include <AzCore/Name/Name.h>
#include <AzCore/Console/IConsole.h>

AZ_CVAR_EXTERNED(bool, r_gpuMarkersMergeGroups);
AZ_CVAR_EXTERNED(bool, r_enablePsoCaching);

namespace AZ::RHI
{
    class DeviceBuffer;
    class DeviceBufferPool;
    class DeviceBufferView;
    class Device;
    class DeviceDispatchRaysIndirectBuffer;
    class DeviceImage;
    class DeviceImagePool;
    class DeviceImageView;
    class DeviceIndirectBufferSignature;
    class DeviceIndirectBufferWriter;
    class DeviceFence;
    class FrameGraphCompiler;
    class FrameGraphExecuter;
    class DevicePipelineState;
    class DevicePipelineLibrary;
    class DeviceQuery;
    class DeviceQueryPool;
    class Scope;
    class DeviceShaderResourceGroup;
    class DeviceShaderResourceGroupPool;
    class DeviceStreamingImagePool;
    class DeviceSwapChain;
    class DeviceTransientAttachmentPool;
    class DeviceRayTracingBufferPools;
    class DeviceRayTracingBlas;
    class DeviceRayTracingTlas;
    class DeviceRayTracingPipelineState;
    class DeviceRayTracingShaderTable;
    class DeviceRayTracingCompactionQueryPool;
    class DeviceRayTracingCompactionQuery;

    //! Priority of a Factory. The lower the number the higher the priority.
    //! Used when there's multiple factories available and the user hasn't define
    //! a priority.
    using APIPriority = uint32_t;
    static const APIPriority APITopPriority = 1;
    static const APIPriority APILowPriority = 10;
    static const APIPriority APIMiddlePriority = (APILowPriority - APITopPriority) / 2;

        

    //! The factory is an interface for creating RHI data structures. The platform system should
    //! register itself with the factory by calling Register, and unregister on shutdown with
    //! Unregister.
    //!
    //! A call to Get will return the active instance. In the event that it's unclear whether
    //! a platform instance exists, you must call IsReady to determine whether it's safe to
    //! call Get. Calling Get without a registered platform will result in an assert.
    class Factory
    {
    public:
        AZ_TYPE_INFO(Factory, "{2C0231FD-DD11-4154-A4F5-177181E26D8E}");

        Factory();
        virtual ~Factory() = default;

        // Note that you have to delete these for safety reasons, you will trip a static_assert if you do not
        AZ_DISABLE_COPY_MOVE(Factory);

        //! Returns the component service name CRC used by the platform RHI system component.
        static uint32_t GetComponentService();

        //! Returns the component service name CRC used by the Factory manager component.
        static uint32_t GetManagerComponentService();

        //! Returns the component service name CRC used by the platform RHI system component.
        static uint32_t GetPlatformService();

        //! Registers the global factory instance.
        static void Register(Factory* instance);

        //! Unregisters the global factory instance.
        static void Unregister(Factory* instance);

        //! Returns whether the factory is initialized and active in this module.
        static bool IsReady();

        //! Access the global factory instance.
        static Factory& Get();

        //! Returns true if Warp is enabled
        static bool UsingWarpDevice();

        //! Returns the name of the Factory.
        virtual Name GetName() = 0;

        //! Returns the APIType of the factory.
        virtual APIType GetType() = 0;

        //! Returns the default priority of the factory in case there's no priorities set in the FactoryManager.
        virtual APIPriority GetDefaultPriority() = 0;

        //! Returns true if the factory supports XR api
        virtual bool SupportsXR() const = 0;

        //! Purpose: The API Unique Index will be encoded in the 2 Most Significant Bits of a ShaderVariantAsset ProductSubId (a 32bits integer). 
        //! Returns a number in the range [0..3].
        //! In theory any given AssetBuilderSdk::PlatformInfo can support several RHI::APITypes.
        //! In reality "pc" only supports DX12 & Vulkan.
        //! "ios" supports only Metal.
        //! "mac" supports only Metal.
        //! "android" supports only Vulkan.
        //! So, for all practical purposes, a single PlatformInfo won't support more than 2 ShaderPlatformInterfaces, but for the sake of
        //! hedging our bets into the future We assume no more than 4 ShaderPlatformInterfaces will ever be supported for any given PlatformInfo.
        //! REMARK: It is the responsibility of the Factory subclass to return a unique number between 0...3.
        //! For example DX12 can return 0, while Vulkan should return 1 (Satisfies "pc", "android" and "linux").
        //! Metal can return 0 because it is the only ShaderPlatformInterface for "ios", "mac" and "appletv".
        //! See AZ::RHI::Limits::APIType::PerPlatformApiUniqueIndexMax.
        virtual uint32_t GetAPIUniqueIndex() const = 0;

        //! Collects the set of physical devices on the system and returns a list of them. Physical
        //! devices represent the hardware attached to the system. Physical devices can be grouped
        //! into nodes for linked setups (e.g. SLI / Crossfire). They can also represent software
        //! reference implementations. Check the PhysicalDeviceType on the descriptor to inspect
        //! this information.
        virtual PhysicalDeviceList EnumeratePhysicalDevices() = 0;

        //! Factory Creation Methods:
        //!
        //! Returns the platform-specific derived variant of the RHI type. All instances are created
        //! in an uninitialized state; the operation simply allocates the memory for the appropriate
        //! platform type and returns the pointer.

        virtual Ptr<DeviceBuffer> CreateBuffer() = 0;

        virtual Ptr<DeviceBufferPool> CreateBufferPool() = 0;

        virtual Ptr<DeviceBufferView> CreateBufferView() = 0;

        virtual Ptr<Device> CreateDevice() = 0;

        virtual Ptr<DeviceFence> CreateFence() = 0;

        virtual Ptr<FrameGraphCompiler> CreateFrameGraphCompiler() = 0;

        virtual Ptr<FrameGraphExecuter> CreateFrameGraphExecuter() = 0;

        virtual Ptr<DeviceImage> CreateImage() = 0;

        virtual Ptr<DeviceImagePool> CreateImagePool() = 0;

        virtual Ptr<DeviceImageView> CreateImageView() = 0;

        virtual Ptr<DeviceStreamingImagePool> CreateStreamingImagePool() = 0;

        virtual Ptr<DevicePipelineState> CreatePipelineState() = 0;

        virtual Ptr<DevicePipelineLibrary> CreatePipelineLibrary() = 0;

        virtual Ptr<Scope> CreateScope() = 0;

        virtual Ptr<DeviceShaderResourceGroup> CreateShaderResourceGroup() = 0;

        virtual Ptr<DeviceShaderResourceGroupPool> CreateShaderResourceGroupPool() = 0;

        virtual Ptr<DeviceSwapChain> CreateSwapChain() = 0;

        virtual Ptr<DeviceTransientAttachmentPool> CreateTransientAttachmentPool() = 0;

        virtual Ptr<DeviceQueryPool> CreateQueryPool() = 0;

        virtual Ptr<DeviceQuery> CreateQuery() = 0;

        virtual Ptr<DeviceIndirectBufferSignature> CreateIndirectBufferSignature() = 0;

        virtual Ptr<DeviceIndirectBufferWriter> CreateIndirectBufferWriter() = 0;

        virtual Ptr<DeviceRayTracingBufferPools> CreateRayTracingBufferPools() = 0;

        virtual Ptr<DeviceRayTracingBlas> CreateRayTracingBlas() = 0;

        virtual Ptr<DeviceRayTracingTlas> CreateRayTracingTlas() = 0;

        virtual Ptr<DeviceRayTracingPipelineState> CreateRayTracingPipelineState() = 0;

        virtual Ptr<DeviceRayTracingShaderTable> CreateRayTracingShaderTable() = 0;

        virtual Ptr<DeviceDispatchRaysIndirectBuffer> CreateDispatchRaysIndirectBuffer() = 0;

        virtual Ptr<DeviceRayTracingCompactionQueryPool> CreateRayTracingCompactionQueryPool() = 0;

        virtual Ptr<DeviceRayTracingCompactionQuery> CreateRayTracingCompactionQuery() = 0;
    };
} // namespace AZ::RHI
