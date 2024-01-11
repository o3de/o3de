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
#include <Atom/RHI/SingleDeviceRayTracingPipelineState.h>
#include <Atom/RHI/SingleDeviceShaderResourceGroup.h>

namespace AZ::RHI
{
    class SingleDeviceRayTracingBufferPools;

    //! Specifies the shader and any local root signature parameters that make up a record in the shader table
    struct SingleDeviceRayTracingShaderTableRecord
    {
        AZ::Name m_shaderExportName;                                    // name of the shader as described in the pipeline state
        const RHI::SingleDeviceShaderResourceGroup* m_shaderResourceGroup;          // shader resource group for this shader record
        static const uint32_t InvalidKey = static_cast<uint32_t>(-1);
        uint32_t m_key = InvalidKey;                                    // key that can be used to identify this record
    };
    using RayTracingShaderTableRecordList = AZStd::list<SingleDeviceRayTracingShaderTableRecord>;

    //! SingleDeviceRayTracingShaderTableDescriptor
    //!
    //! The Build() operation in the descriptor allows the shader table to be initialized
    //! using the following pattern:
    //!
    //! RHI::SingleDeviceRayTracingShaderTableDescriptor descriptor;
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
    class SingleDeviceRayTracingShaderTableDescriptor
    {
    public:
        SingleDeviceRayTracingShaderTableDescriptor() = default;
        ~SingleDeviceRayTracingShaderTableDescriptor() = default;

        // accessors
        const RHI::Ptr<SingleDeviceRayTracingPipelineState>& GetPipelineState() const { return m_rayTracingPipelineState; }

        const RayTracingShaderTableRecordList& GetRayGenerationRecord() const { return m_rayGenerationRecord; }
        RayTracingShaderTableRecordList& GetRayGenerationRecord() { return m_rayGenerationRecord; }

        const RayTracingShaderTableRecordList& GetMissRecords() const { return m_missRecords; }
        RayTracingShaderTableRecordList& GetMissRecords() { return m_missRecords; }

        const RayTracingShaderTableRecordList& GetCallableRecords() const { return m_callableRecords; }
        RayTracingShaderTableRecordList& GetCallableRecords() { return m_callableRecords; }

        const RayTracingShaderTableRecordList& GetHitGroupRecords() const { return m_hitGroupRecords; }
        RayTracingShaderTableRecordList& GetHitGroupRecords() { return m_hitGroupRecords; }

        void RemoveHitGroupRecords(uint32_t key);

        // build operations
        SingleDeviceRayTracingShaderTableDescriptor* Build(const AZ::Name& name, const RHI::Ptr<SingleDeviceRayTracingPipelineState>& rayTracingPipelineState);
        SingleDeviceRayTracingShaderTableDescriptor* RayGenerationRecord(const AZ::Name& name);
        SingleDeviceRayTracingShaderTableDescriptor* MissRecord(const AZ::Name& name);
        SingleDeviceRayTracingShaderTableDescriptor* CallableRecord(const AZ::Name& name);
        SingleDeviceRayTracingShaderTableDescriptor* HitGroupRecord(const AZ::Name& name, uint32_t key = SingleDeviceRayTracingShaderTableRecord::InvalidKey);
        SingleDeviceRayTracingShaderTableDescriptor* ShaderResourceGroup(const RHI::SingleDeviceShaderResourceGroup* shaderResourceGroup);

    private:
        AZ::Name m_name;
        RHI::Ptr<SingleDeviceRayTracingPipelineState> m_rayTracingPipelineState;
        RayTracingShaderTableRecordList m_rayGenerationRecord;  // limited to one record, but stored as a list to simplify processing
        RayTracingShaderTableRecordList m_missRecords;
        RayTracingShaderTableRecordList m_callableRecords;
        RayTracingShaderTableRecordList m_hitGroupRecords;

        SingleDeviceRayTracingShaderTableRecord* m_buildContext = nullptr;
    };

    //! Shader Table
    //! Specifies the ray generation, miss, and hit shaders used during the ray tracing process
    class SingleDeviceRayTracingShaderTable
        : public DeviceObject
    {
    public:
        SingleDeviceRayTracingShaderTable() = default;
        virtual ~SingleDeviceRayTracingShaderTable() = default;

        static RHI::Ptr<RHI::SingleDeviceRayTracingShaderTable> CreateRHIRayTracingShaderTable();
        void Init(Device& device, const SingleDeviceRayTracingBufferPools& rayTracingBufferPools);

        //! Queues this SingleDeviceRayTracingShaderTable to be built by the FrameScheduler.
        //! Note that the descriptor must be heap allocated, preferably using make_shared.
        void Build(const AZStd::shared_ptr<SingleDeviceRayTracingShaderTableDescriptor> descriptor);

    protected:

        AZStd::shared_ptr<SingleDeviceRayTracingShaderTableDescriptor> m_descriptor;
        const SingleDeviceRayTracingBufferPools* m_bufferPools = nullptr;

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
