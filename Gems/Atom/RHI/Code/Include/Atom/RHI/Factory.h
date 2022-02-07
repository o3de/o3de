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

#if defined(USE_RENDERDOC)
#include <renderdoc_app.h>
#endif

AZ_DECLARE_BUDGET(RHI);

namespace AZ
{
    namespace RHI
    {
        class Buffer;
        class BufferPool;
        class BufferView;
        class Device;
        class Image;
        class ImagePool;
        class ImageView;
        class IndirectBufferSignature;
        class IndirectBufferWriter;
        class Fence;
        class FrameGraphCompiler;
        class FrameGraphExecuter;
        class PipelineState;
        class PipelineLibrary;
        class Query;
        class QueryPool;
        class Scope;
        class ShaderResourceGroup;
        class ShaderResourceGroupPool;
        class StreamingImagePool;
        class SwapChain;
        class TransientAttachmentPool;
        class RayTracingBufferPools;
        class RayTracingBlas;
        class RayTracingTlas;
        class RayTracingPipelineState;
        class RayTracingShaderTable;

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

#if defined(USE_RENDERDOC)
            //! Access the RenderDoc API pointer if available.
            //! The availability of the render doc API at runtime depends on the following:
            //! - You must not be building a packaged game/product (LY_MONOLITHIC_GAME not enabled in CMake)
            //! - A valid renderdoc installation was found, either by auto-discovery, or by supplying ATOM_RENDERDOC_PATH as an environment variable
            //! - The module loaded successfully at runtime, and the API function pointer was retrieved successfully
            static RENDERDOC_API_1_1_2* GetRenderDocAPI();
#endif
            
            //! Returns true if RenderDoc dll is loaded
            static bool IsRenderDocModuleLoaded();

            //! Returns true if Pix dll is loaded
            static bool IsPixModuleLoaded();

            //! Returns true if Warp is enabled
            static bool UsingWarpDevice();

            //! Returns the name of the Factory.
            virtual Name GetName() = 0;

            //! Returns the APIType of the factory.
            virtual APIType GetType() = 0;

            //! Returns the default priority of the factory in case there's no priorities set in the FactoryManager.
            virtual APIPriority GetDefaultPriority() = 0;

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

            virtual Ptr<Buffer> CreateBuffer() = 0;

            virtual Ptr<BufferPool> CreateBufferPool() = 0;

            virtual Ptr<BufferView> CreateBufferView() = 0;

            virtual Ptr<Device> CreateDevice() = 0;

            virtual Ptr<Fence> CreateFence() = 0;

            virtual Ptr<FrameGraphCompiler> CreateFrameGraphCompiler() = 0;

            virtual Ptr<FrameGraphExecuter> CreateFrameGraphExecuter() = 0;

            virtual Ptr<Image> CreateImage() = 0;

            virtual Ptr<ImagePool> CreateImagePool() = 0;

            virtual Ptr<ImageView> CreateImageView() = 0;

            virtual Ptr<StreamingImagePool> CreateStreamingImagePool() = 0;

            virtual Ptr<PipelineState> CreatePipelineState() = 0;

            virtual Ptr<PipelineLibrary> CreatePipelineLibrary() = 0;

            virtual Ptr<Scope> CreateScope() = 0;

            virtual Ptr<ShaderResourceGroup> CreateShaderResourceGroup() = 0;

            virtual Ptr<ShaderResourceGroupPool> CreateShaderResourceGroupPool() = 0;

            virtual Ptr<SwapChain> CreateSwapChain() = 0;

            virtual Ptr<TransientAttachmentPool> CreateTransientAttachmentPool() = 0;

            virtual Ptr<QueryPool> CreateQueryPool() = 0;

            virtual Ptr<Query> CreateQuery() = 0;

            virtual Ptr<IndirectBufferSignature> CreateIndirectBufferSignature() = 0;

            virtual Ptr<IndirectBufferWriter> CreateIndirectBufferWriter() = 0;

            virtual Ptr<RayTracingBufferPools> CreateRayTracingBufferPools() = 0;

            virtual Ptr<RayTracingBlas> CreateRayTracingBlas() = 0;

            virtual Ptr<RayTracingTlas> CreateRayTracingTlas() = 0;

            virtual Ptr<RayTracingPipelineState> CreateRayTracingPipelineState() = 0;

            virtual Ptr<RayTracingShaderTable> CreateRayTracingShaderTable() = 0;
        };
    }
}
