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
#include <Atom/RHI/MultiDevicePipelineState.h>
#include <Atom/RHI/RayTracingPipelineState.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    namespace RHI
    {
        //! MultiDeviceRayTracingPipelineStateDescriptor
        //!
        //! The Build() operation in the descriptor allows the pipeline state to be initialized
        //! using the following pattern:
        //!
        //! RHI::MultiDeviceRayTracingPipelineStateDescriptor descriptor;
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
        class MultiDeviceRayTracingPipelineStateDescriptor final
        {
        public:
            MultiDeviceRayTracingPipelineStateDescriptor() = default;
            ~MultiDeviceRayTracingPipelineStateDescriptor() = default;

            RayTracingPipelineStateDescriptor GetDeviceRayTracingPipelineStateDescriptor(int deviceIndex) const;

            // accessors
            const RayTracingConfiguration& GetConfiguration() const
            {
                return m_descriptor.GetConfiguration();
            }
            RayTracingConfiguration& GetConfiguration()
            {
                return m_descriptor.GetConfiguration();
            }

            const RHI::MultiDevicePipelineState* GetPipelineState() const
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
                ;
            }

            // build operations
            MultiDeviceRayTracingPipelineStateDescriptor* Build();
            MultiDeviceRayTracingPipelineStateDescriptor* MaxPayloadSize(uint32_t maxPayloadSize);
            MultiDeviceRayTracingPipelineStateDescriptor* MaxAttributeSize(uint32_t maxAttributeSize);
            MultiDeviceRayTracingPipelineStateDescriptor* MaxRecursionDepth(uint32_t maxRecursionDepth);
            MultiDeviceRayTracingPipelineStateDescriptor* PipelineState(const RHI::MultiDevicePipelineState* pipelineState);
            MultiDeviceRayTracingPipelineStateDescriptor* ShaderLibrary(RHI::PipelineStateDescriptorForRayTracing& descriptor);

            MultiDeviceRayTracingPipelineStateDescriptor* RayGenerationShaderName(const AZ::Name& name);
            MultiDeviceRayTracingPipelineStateDescriptor* MissShaderName(const AZ::Name& name);
            MultiDeviceRayTracingPipelineStateDescriptor* ClosestHitShaderName(const AZ::Name& closestHitShaderName);
            MultiDeviceRayTracingPipelineStateDescriptor* AnyHitShaderName(const AZ::Name& anyHitShaderName);

            MultiDeviceRayTracingPipelineStateDescriptor* HitGroup(const AZ::Name& name);

        private:
            const RHI::MultiDevicePipelineState* m_pipelineState = nullptr;
            RayTracingPipelineStateDescriptor m_descriptor;
        };

        //! Defines the shaders, hit groups, and other parameters required for ray tracing operations.
        class MultiDeviceRayTracingPipelineState : public MultiDeviceObject
        {
        public:
            AZ_CLASS_ALLOCATOR(MultiDeviceRayTracingPipelineState, AZ::SystemAllocator, 0);
            AZ_RTTI(MultiDeviceRayTracingPipelineState, "{22F609DF-C889-4278-9580-3D2A99E78857}", MultiDeviceObject)
            MultiDeviceRayTracingPipelineState() = default;

            virtual ~MultiDeviceRayTracingPipelineState() = default;

            const RHI::Ptr<RayTracingPipelineState>& GetDevicePipelineState(int deviceIndex) const
            {
                AZ_Error(
                    "MultiDeviceRayTracingPipelineState",
                    m_deviceRayTracingPipelineStates.find(deviceIndex) != m_deviceRayTracingPipelineStates.end(),
                    "No RayTracingPipelineState found for device index %d\n",
                    deviceIndex);

                return m_deviceRayTracingPipelineStates.at(deviceIndex);
            }

            const MultiDeviceRayTracingPipelineStateDescriptor& GetDescriptor() const
            {
                return m_descriptor;
            }
            ResultCode Init(MultiDevice::DeviceMask deviceMask, const MultiDeviceRayTracingPipelineStateDescriptor& descriptor);

        private:
            // explicit shutdown is not allowed for this type
            void Shutdown() override final;

            MultiDeviceRayTracingPipelineStateDescriptor m_descriptor;
            AZStd::unordered_map<int, Ptr<RayTracingPipelineState>> m_deviceRayTracingPipelineStates;
        };
    } // namespace RHI
} // namespace AZ
