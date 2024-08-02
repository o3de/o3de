/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Base.h>
#include <Atom/RHI/DeviceObject.h>
#include <Atom/RHI/PipelineState.h>
#include <Atom/RHI/DeviceRayTracingPipelineState.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace AZ::RHI
{
    //! RayTracingPipelineStateDescriptor
    //!
    //! The Build() operation in the descriptor allows the pipeline state to be initialized
    //! using the following pattern:
    //!
    //! RHI::RayTracingPipelineStateDescriptor descriptor;
    //! descriptor.Build()
    //!     ->ShaderLibrary(shaderDescriptor)
    //!         ->RayGenerationShaderName(AZ::Name("RayGenerationShader"))
    //!     ->ShaderLibrary(missShaderDescriptor)
    //!         ->MissShaderName(AZ::Name("MissShader"))
    //!     ->ShaderLibrary(closestHitShader1Descriptor)
    //!         ->ClosestHitShaderName(AZ::Name("ClosestHitShader1"))
    //!     ->ShaderLibrary(closestHitShader2Descriptor)
    //!         ->ClosestHitShaderName(AZ::Name("ClosestHitShader2"))
    //!     ->HitGroup(AZ::Name("HitGroup1"))
    //!         ->ClosestHitShaderName(AZ::Name("ClosestHitShader1"))
    //!     ->HitGroup(AZ::Name("HitGroup2"))
    //!         ->ClosestHitShaderName(AZ::Name("ClosestHitShader2"))
    //!     ;
    //!
    class RayTracingPipelineStateDescriptor final
    {
    public:
        RayTracingPipelineStateDescriptor() = default;
        ~RayTracingPipelineStateDescriptor() = default;

        //! Returns the device-specific DeviceRayTracingPipelineStateDescriptor for the given index
        DeviceRayTracingPipelineStateDescriptor GetDeviceRayTracingPipelineStateDescriptor(int deviceIndex) const;

        //! Accessors
        const RayTracingConfiguration& GetConfiguration() const
        {
            return m_descriptor.GetConfiguration();
        }
        RayTracingConfiguration& GetConfiguration()
        {
            return m_descriptor.GetConfiguration();
        }

        const RHI::PipelineState* GetPipelineState() const
        {
            return m_pipelineState;
        }

        const RayTracingShaderLibraryVector& GetShaderLibraries() const
        {
            return m_descriptor.GetShaderLibraries();
        }
        RayTracingShaderLibraryVector& GetShaderLibraries()
        {
            return m_descriptor.GetShaderLibraries();
        }

        const RayTracingHitGroupVector& GetHitGroups() const
        {
            return m_descriptor.GetHitGroups();
        }
        RayTracingHitGroupVector& GetHitGroups()
        {
            return m_descriptor.GetHitGroups();
        }

        //! Build operations
        RayTracingPipelineStateDescriptor* Build();
        RayTracingPipelineStateDescriptor* MaxPayloadSize(uint32_t maxPayloadSize);
        RayTracingPipelineStateDescriptor* MaxAttributeSize(uint32_t maxAttributeSize);
        RayTracingPipelineStateDescriptor* MaxRecursionDepth(uint32_t maxRecursionDepth);
        RayTracingPipelineStateDescriptor* PipelineState(const RHI::PipelineState* pipelineState);
        RayTracingPipelineStateDescriptor* ShaderLibrary(RHI::PipelineStateDescriptorForRayTracing& descriptor);

        RayTracingPipelineStateDescriptor* RayGenerationShaderName(const AZ::Name& name);
        RayTracingPipelineStateDescriptor* MissShaderName(const AZ::Name& name);
        RayTracingPipelineStateDescriptor* ClosestHitShaderName(const AZ::Name& closestHitShaderName);
        RayTracingPipelineStateDescriptor* AnyHitShaderName(const AZ::Name& anyHitShaderName);
        RayTracingPipelineStateDescriptor* IntersectionShaderName(const AZ::Name& intersectionShaderName);

        RayTracingPipelineStateDescriptor* HitGroup(const AZ::Name& name);

    private:
        const RHI::PipelineState* m_pipelineState = nullptr;
        DeviceRayTracingPipelineStateDescriptor m_descriptor;
    };

    //! Defines the shaders, hit groups, and other parameters required for ray tracing operations across multiple devices.
    class RayTracingPipelineState : public MultiDeviceObject
    {
    public:
        AZ_CLASS_ALLOCATOR(RayTracingPipelineState, AZ::SystemAllocator, 0);
        AZ_RTTI(RayTracingPipelineState, "{22F609DF-C889-4278-9580-3D2A99E78857}", MultiDeviceObject);
        AZ_RHI_MULTI_DEVICE_OBJECT_GETTER(RayTracingPipelineState);
        RayTracingPipelineState() = default;

        virtual ~RayTracingPipelineState() = default;

        const RayTracingPipelineStateDescriptor& GetDescriptor() const
        {
            return m_descriptor;
        }

        //! Initialize all device-specific RayTracingPipelineStates
        ResultCode Init(MultiDevice::DeviceMask deviceMask, const RayTracingPipelineStateDescriptor& descriptor);

    private:
        //! explicit shutdown is not allowed for this type
        void Shutdown() override final;

        RayTracingPipelineStateDescriptor m_descriptor;
    };
} // namespace AZ::RHI
