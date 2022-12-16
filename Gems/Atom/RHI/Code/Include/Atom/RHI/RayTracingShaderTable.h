/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Base.h>
#include <Atom/RHI/DeviceRayTracingShaderTable.h>
#include <Atom/RHI/MultiDeviceObject.h>
#include <Atom/RHI/RayTracingPipelineState.h>
#include <Atom/RHI/ShaderResourceGroup.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    namespace RHI
    {
        class RayTracingBufferPools;

        //! Specifies the shader and any local root signature parameters that make up a record in the shader table
        struct RayTracingShaderTableRecord
        {
            AZ::Name m_shaderExportName; // name of the shader as described in the pipeline state
            const RHI::ShaderResourceGroup* m_shaderResourceGroup; // shader resource group for this shader record
            static const uint32_t InvalidKey = static_cast<uint32_t>(-1);
            uint32_t m_key = InvalidKey; // key that can be used to identify this record
        };
        using RayTracingShaderTableRecordList = AZStd::list<RayTracingShaderTableRecord>;

        //! RayTracingShaderTableDescriptor
        //!
        //! The Build() operation in the descriptor allows the shader table to be initialized
        //! using the following pattern:
        //!
        //! RHI::RayTracingShaderTableDescriptor descriptor;
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
        class RayTracingShaderTableDescriptor
        {
        public:
            RayTracingShaderTableDescriptor() = default;
            ~RayTracingShaderTableDescriptor() = default;

            AZStd::shared_ptr<DeviceRayTracingShaderTableDescriptor> GetDeviceRayTracingShaderTableDescriptor(int deviceIndex);

            // accessors
            const RHI::Ptr<RayTracingPipelineState>& GetPipelineState() const
            {
                return m_rayTracingPipelineState;
            }

            const RayTracingShaderTableRecordList& GetRayGenerationRecord() const
            {
                return m_rayGenerationRecord;
            }
            RayTracingShaderTableRecordList& GetRayGenerationRecord()
            {
                return m_rayGenerationRecord;
            }

            const RayTracingShaderTableRecordList& GetMissRecords() const
            {
                return m_missRecords;
            }
            RayTracingShaderTableRecordList& GetMissRecords()
            {
                return m_missRecords;
            }

            const RayTracingShaderTableRecordList& GetHitGroupRecords() const
            {
                return m_hitGroupRecords;
            }
            RayTracingShaderTableRecordList& GetHitGroupRecords()
            {
                return m_hitGroupRecords;
            }

            void RemoveHitGroupRecords(uint32_t key);

            // build operations
            RayTracingShaderTableDescriptor* Build(const AZ::Name& name, RHI::Ptr<RayTracingPipelineState>& rayTracingPipelineState);
            RayTracingShaderTableDescriptor* RayGenerationRecord(const AZ::Name& name);
            RayTracingShaderTableDescriptor* MissRecord(const AZ::Name& name);
            RayTracingShaderTableDescriptor* HitGroupRecord(const AZ::Name& name, uint32_t key = RayTracingShaderTableRecord::InvalidKey);
            RayTracingShaderTableDescriptor* ShaderResourceGroup(const RHI::ShaderResourceGroup* shaderResourceGroup);

        private:
            AZ::Name m_name;
            RHI::Ptr<RayTracingPipelineState> m_rayTracingPipelineState;
            RayTracingShaderTableRecordList m_rayGenerationRecord; // limited to one record, but stored as a list to simplify processing
            RayTracingShaderTableRecordList m_missRecords;
            RayTracingShaderTableRecordList m_hitGroupRecords;

            RayTracingShaderTableRecord* m_buildContext = nullptr;
        };

        //! Shader Table
        //! Specifies the ray generation, miss, and hit shaders used during the ray tracing process
        class RayTracingShaderTable : public MultiDeviceObject
        {
        public:
            AZ_CLASS_ALLOCATOR(RayTracingShaderTable, AZ::SystemAllocator, 0);
            AZ_RTTI(RayTracingShaderTable, "{C3750390-748D-4AC2-AD83-54AE037C85CE}", MultiDeviceObject)
            RayTracingShaderTable() = default;

            static RHI::Ptr<RHI::RayTracingShaderTable> Create()
            {
                return aznew RayTracingShaderTable;
            }

            const RHI::Ptr<DeviceRayTracingShaderTable>& GetDeviceRayTracingShaderTable(int deviceIndex) const
            {
                AZ_Assert(
                    m_deviceRayTracingShaderTables.find(deviceIndex) != m_deviceRayTracingShaderTables.end(),
                    "No DeviceRayTracingShaderTable found for device index %d\n",
                    deviceIndex);
                return m_deviceRayTracingShaderTables.at(deviceIndex);
            }

            void Init(DeviceMask deviceMask, const RayTracingBufferPools& rayTracingBufferPools);

            //! Queues this RayTracingShaderTable to be built by the FrameScheduler.
            //! Note that the descriptor must be heap allocated, preferably using make_shared.
            void Build(const AZStd::shared_ptr<RayTracingShaderTableDescriptor> descriptor);

        private:
            AZStd::unordered_map<int, Ptr<DeviceRayTracingShaderTable>> m_deviceRayTracingShaderTables;
        };
    } // namespace RHI
} // namespace AZ
