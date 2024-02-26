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
#include <Atom/RHI/MultiDeviceRayTracingPipelineState.h>
#include <Atom/RHI/SingleDeviceRayTracingShaderTable.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace AZ::RHI
{
    class MultiDeviceRayTracingBufferPools;
    class MultiDeviceShaderResourceGroup;

    //! Specifies the shader and any local root signature parameters that make up a record in the shader table
    struct MultiDeviceRayTracingShaderTableRecord
    {
        //! name of the shader as described in the pipeline state
        AZ::Name m_shaderExportName;

        //! shader resource group for this shader record
        const RHI::MultiDeviceShaderResourceGroup* m_mdShaderResourceGroup;

        static const uint32_t InvalidKey = static_cast<uint32_t>(-1);

        //! key that can be used to identify this record
        uint32_t m_key = InvalidKey;
    };
    using MultiDeviceRayTracingShaderTableRecordList = AZStd::list<MultiDeviceRayTracingShaderTableRecord>;

    //! MultiDeviceRayTracingShaderTableDescriptor
    //!
    //! The Build() operation in the descriptor allows the shader table to be initialized
    //! using the following pattern:
    //!
    //! RHI::MultiDeviceRayTracingShaderTableDescriptor descriptor;
    //! descriptor.Build(AZ::Name("RayTracingExampleShaderTable"), m_mdRayTracingPipelineState)
    //!     ->RayGenerationRecord(AZ::Name("RayGenerationShader"))
    //!     ->MissRecord(AZ::Name("MissShader"))
    //!         ->ShaderResourceGroup(missSrg)
    //!     ->HitGroupRecord(AZ::Name("HitGroup1"))
    //!         ->ShaderResourceGroup(hitGroupSrg1)
    //!     ->HitGroupRecord(AZ::Name("HitGroup2"))
    //!         ->ShaderResourceGroup(hitGroupSrg2)
    //!     ;
    //!
    class MultiDeviceRayTracingShaderTableDescriptor
    {
    public:
        MultiDeviceRayTracingShaderTableDescriptor() = default;
        ~MultiDeviceRayTracingShaderTableDescriptor() = default;

        //! Returns the device-specific SingleDeviceRayTracingShaderTableDescriptor for the given index
        AZStd::shared_ptr<SingleDeviceRayTracingShaderTableDescriptor> GetDeviceRayTracingShaderTableDescriptor(int deviceIndex);

        //! Accessors
        const RHI::Ptr<MultiDeviceRayTracingPipelineState>& GetPipelineState() const
        {
            return m_mdRayTracingPipelineState;
        }

        const MultiDeviceRayTracingShaderTableRecordList& GetRayGenerationRecord() const
        {
            return m_mdRayGenerationRecord;
        }
        MultiDeviceRayTracingShaderTableRecordList& GetRayGenerationRecord()
        {
            return m_mdRayGenerationRecord;
        }

        const MultiDeviceRayTracingShaderTableRecordList& GetMissRecords() const
        {
            return m_mdMissRecords;
        }
        MultiDeviceRayTracingShaderTableRecordList& GetMissRecords()
        {
            return m_mdMissRecords;
        }

        const MultiDeviceRayTracingShaderTableRecordList& GetHitGroupRecords() const
        {
            return m_mdHitGroupRecords;
        }
        MultiDeviceRayTracingShaderTableRecordList& GetHitGroupRecords()
        {
            return m_mdHitGroupRecords;
        }

        void RemoveHitGroupRecords(uint32_t key);

        //! build operations
        MultiDeviceRayTracingShaderTableDescriptor* Build(
            const AZ::Name& name, RHI::Ptr<MultiDeviceRayTracingPipelineState>& rayTracingPipelineState);
        MultiDeviceRayTracingShaderTableDescriptor* RayGenerationRecord(const AZ::Name& name);
        MultiDeviceRayTracingShaderTableDescriptor* MissRecord(const AZ::Name& name);
        MultiDeviceRayTracingShaderTableDescriptor* HitGroupRecord(
            const AZ::Name& name, uint32_t key = MultiDeviceRayTracingShaderTableRecord::InvalidKey);
        MultiDeviceRayTracingShaderTableDescriptor* ShaderResourceGroup(const RHI::MultiDeviceShaderResourceGroup* shaderResourceGroup);

    private:
        AZ::Name m_name;
        RHI::Ptr<MultiDeviceRayTracingPipelineState> m_mdRayTracingPipelineState;
        //! limited to one record, but stored as a list to simplify processing
        MultiDeviceRayTracingShaderTableRecordList m_mdRayGenerationRecord;
        MultiDeviceRayTracingShaderTableRecordList m_mdMissRecords;
        MultiDeviceRayTracingShaderTableRecordList m_mdHitGroupRecords;

        MultiDeviceRayTracingShaderTableRecord* m_mdBuildContext = nullptr;
    };

    //! Shader Table
    //! Specifies the ray generation, miss, and hit shaders used during the ray tracing process
    class MultiDeviceRayTracingShaderTable : public MultiDeviceObject
    {
    public:
        AZ_CLASS_ALLOCATOR(MultiDeviceRayTracingShaderTable, AZ::SystemAllocator, 0);
        AZ_RTTI(MultiDeviceRayTracingShaderTable, "{B448997B-A8E6-446E-A333-EFD92B486D9B}", MultiDeviceObject);
        AZ_RHI_MULTI_DEVICE_OBJECT_GETTER(RayTracingShaderTable);
        MultiDeviceRayTracingShaderTable() = default;
        virtual ~MultiDeviceRayTracingShaderTable() = default;

        //! Initialize all device-specific RayTracingShaderTables
        void Init(MultiDevice::DeviceMask deviceMask, const MultiDeviceRayTracingBufferPools& rayTracingBufferPools);

        //! Queues this MultiDeviceRayTracingShaderTable to be built by the FrameScheduler.
        //! Note that the descriptor must be heap allocated, preferably using make_shared.
        void Build(const AZStd::shared_ptr<MultiDeviceRayTracingShaderTableDescriptor> descriptor);
    };
} // namespace AZ::RHI
