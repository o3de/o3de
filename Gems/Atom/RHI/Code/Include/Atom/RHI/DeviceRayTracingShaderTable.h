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
    //! The Build() operation in the descriptor allows the shader table to be initialized
    //! using the following pattern:
    //!
    //! RHI::DeviceRayTracingShaderTableDescriptor descriptor;
    //! descriptor.Build(AZ::Name("RayTracingExampleShaderTable"), m_rayTracingPipelineState)
    //!     ->RayGenerationRecord(AZ::Name("RayGenerationShader"))
    //!     ->MissRecord(AZ::Name("MissShader"))
    //!         ->ShaderResourceGroup(missSrg)
    //!     ->HitGroupRecord(AZ::Name("HitGroup1"))
    //!         ->ShaderResourceGroup(hitGroupSrg1)
    //!     ->HitGroupRecord(AZ::Name("HitGroup2"))
    //!         ->ShaderResourceGroup(hitGroupSrg2)
    //!     ;
    //!
    class DeviceRayTracingShaderTableDescriptor
    {
    public:
        DeviceRayTracingShaderTableDescriptor() = default;
        ~DeviceRayTracingShaderTableDescriptor() = default;

        // accessors
        const RHI::Ptr<DeviceRayTracingPipelineState>& GetPipelineState() const { return m_rayTracingPipelineState; }

        const DeviceRayTracingShaderTableRecordList& GetRayGenerationRecord() const { return m_rayGenerationRecord; }
        DeviceRayTracingShaderTableRecordList& GetRayGenerationRecord() { return m_rayGenerationRecord; }

        const DeviceRayTracingShaderTableRecordList& GetMissRecords() const { return m_missRecords; }
        DeviceRayTracingShaderTableRecordList& GetMissRecords() { return m_missRecords; }

        const DeviceRayTracingShaderTableRecordList& GetCallableRecords() const { return m_callableRecords; }
        DeviceRayTracingShaderTableRecordList& GetCallableRecords() { return m_callableRecords; }

        const DeviceRayTracingShaderTableRecordList& GetHitGroupRecords() const { return m_hitGroupRecords; }
        DeviceRayTracingShaderTableRecordList& GetHitGroupRecords() { return m_hitGroupRecords; }

        void RemoveHitGroupRecords(uint32_t key);

        // build operations
        DeviceRayTracingShaderTableDescriptor* Build(const AZ::Name& name, const RHI::Ptr<DeviceRayTracingPipelineState>& rayTracingPipelineState);
        DeviceRayTracingShaderTableDescriptor* RayGenerationRecord(const AZ::Name& name);
        DeviceRayTracingShaderTableDescriptor* MissRecord(const AZ::Name& name);
        DeviceRayTracingShaderTableDescriptor* CallableRecord(const AZ::Name& name);
        DeviceRayTracingShaderTableDescriptor* HitGroupRecord(const AZ::Name& name, uint32_t key = DeviceRayTracingShaderTableRecord::InvalidKey);
        DeviceRayTracingShaderTableDescriptor* ShaderResourceGroup(const RHI::DeviceShaderResourceGroup* shaderResourceGroup);

    private:
        AZ::Name m_name;
        RHI::Ptr<DeviceRayTracingPipelineState> m_rayTracingPipelineState;
        DeviceRayTracingShaderTableRecordList m_rayGenerationRecord;  // limited to one record, but stored as a list to simplify processing
        DeviceRayTracingShaderTableRecordList m_missRecords;
        DeviceRayTracingShaderTableRecordList m_callableRecords;
        DeviceRayTracingShaderTableRecordList m_hitGroupRecords;

        DeviceRayTracingShaderTableRecord* m_buildContext = nullptr;
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
