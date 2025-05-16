/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <Atom/RHI/DeviceObject.h>
#include <Atom/RHI.Reflect/Base.h>
#include <Atom/RHI/DeviceRayTracingPipelineState.h>
#include <Atom/RHI/DeviceShaderResourceGroup.h>

namespace AZ::RHI
{
    class DeviceRayTracingBufferPools;

    //! Specifies the shader and any local root signature parameters that make up a record in the shader table
    struct DeviceRayTracingShaderTableRecord
    {
        AZ::Name m_shaderExportName;                                    // name of the shader as described in the pipeline state
        const RHI::DeviceShaderResourceGroup* m_shaderResourceGroup;          // shader resource group for this shader record
        static const uint32_t InvalidKey = static_cast<uint32_t>(-1);
        uint32_t m_key = InvalidKey;                                    // key that can be used to identify this record
    };
    using DeviceRayTracingShaderTableRecordList = AZStd::list<DeviceRayTracingShaderTableRecord>;

    //! DeviceRayTracingShaderTableDescriptor
    //!
    //!  Describes a single-device ray tracing shader table.
    class DeviceRayTracingShaderTableDescriptor
    {
    public:
        void RemoveHitGroupRecords(uint32_t key);

        AZ::Name m_name;
        RHI::Ptr<DeviceRayTracingPipelineState> m_rayTracingPipelineState;
        DeviceRayTracingShaderTableRecordList m_rayGenerationRecord;  // limited to one record, but stored as a list to simplify processing
        DeviceRayTracingShaderTableRecordList m_missRecords;
        DeviceRayTracingShaderTableRecordList m_callableRecords;
        DeviceRayTracingShaderTableRecordList m_hitGroupRecords;
    };

    //! Shader Table
    //! Specifies the ray generation, miss, and hit shaders used during the ray tracing process
    class DeviceRayTracingShaderTable
        : public DeviceObject
    {
    public:
        DeviceRayTracingShaderTable() = default;
        virtual ~DeviceRayTracingShaderTable() = default;

        static RHI::Ptr<RHI::DeviceRayTracingShaderTable> CreateRHIRayTracingShaderTable();
        void Init(Device& device, const DeviceRayTracingBufferPools& rayTracingBufferPools);

        //! Queues this DeviceRayTracingShaderTable to be built by the FrameScheduler.
        //! Note that the descriptor must be heap allocated, preferably using make_shared.
        void Build(const AZStd::shared_ptr<DeviceRayTracingShaderTableDescriptor> descriptor);

    protected:

        AZStd::shared_ptr<DeviceRayTracingShaderTableDescriptor> m_descriptor;
        const DeviceRayTracingBufferPools* m_bufferPools = nullptr;

    private:

        friend class FrameScheduler;

        /// Called by the FrameScheduler to validate the state prior to building
        void Validate();

        //////////////////////////////////////////////////////////////////////////
        // Platform API
        virtual RHI::ResultCode BuildInternal() = 0;
        //////////////////////////////////////////////////////////////////////////

        bool m_isQueuedForBuild = false;
    };
}
