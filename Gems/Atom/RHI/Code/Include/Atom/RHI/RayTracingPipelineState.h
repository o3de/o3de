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
    //! Describes a ray tracing pipeline state.
    class RayTracingPipelineStateDescriptor final
    {
    public:
        //! Returns the device-specific DeviceRayTracingPipelineStateDescriptor for the given index
        DeviceRayTracingPipelineStateDescriptor GetDeviceRayTracingPipelineStateDescriptor(int deviceIndex) const;

        //! Convenience functions for adding shader libraries
        void AddRayGenerationShaderLibrary(const PipelineStateDescriptorForRayTracing& descriptor, const Name& rayGenerationShaderName);
        void AddMissShaderLibrary(const PipelineStateDescriptorForRayTracing& descriptor, const Name& missShaderName);
        void AddCallableShaderLibrary(const PipelineStateDescriptorForRayTracing& descriptor, const Name& callableShaderName);
        void AddClosestHitShaderLibrary(const PipelineStateDescriptorForRayTracing& descriptor, const Name& closestHitShaderName);
        void AddAnyHitShaderLibrary(const PipelineStateDescriptorForRayTracing& descriptor, const Name& anyHitShaderName);
        void AddIntersectionShaderLibrary(const PipelineStateDescriptorForRayTracing& descriptor, const Name& intersectionShaderName);

        //! Convenience functions for adding hit groups
        void AddHitGroup(const Name& hitGroupName, const Name& closestHitShaderName);
        void AddHitGroup(const Name& hitGroupName, const Name& closestHitShaderName, const Name& intersectionShaderName);

        RayTracingConfiguration m_configuration;
        RayTracingShaderLibraryVector m_shaderLibraries;
        RayTracingHitGroupVector m_hitGroups;
        const RHI::PipelineState* m_pipelineState = nullptr;
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
