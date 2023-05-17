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
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
// #include <Atom/RHI/MultiDeviceShaderResourceGroup.h>
#include <Atom/RHI/RayTracingShaderTable.h>

namespace AZ
{
    namespace RHI
    {
        class MultiDeviceRayTracingBufferPools;
        class MultiDeviceShaderResourceGroup;

        //! Specifies the shader and any local root signature parameters that make up a record in the shader table
        struct MultiDeviceRayTracingShaderTableRecord
        {
            AZ::Name m_shaderExportName; // name of the shader as described in the pipeline state
            const RHI::MultiDeviceShaderResourceGroup* m_shaderResourceGroup; // shader resource group for this shader record
            static const uint32_t InvalidKey = static_cast<uint32_t>(-1);
            uint32_t m_key = InvalidKey; // key that can be used to identify this record
        };
        using MultiDeviceRayTracingShaderTableRecordList = AZStd::list<MultiDeviceRayTracingShaderTableRecord>;

        //! MultiDeviceRayTracingShaderTableDescriptor
        //!
        //! The Build() operation in the descriptor allows the shader table to be initialized
        //! using the following pattern:
        //!
        //! RHI::MultiDeviceRayTracingShaderTableDescriptor descriptor;
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
        class MultiDeviceRayTracingShaderTableDescriptor
        {
        public:
            MultiDeviceRayTracingShaderTableDescriptor() = default;
            ~MultiDeviceRayTracingShaderTableDescriptor() = default;

            AZStd::shared_ptr<RayTracingShaderTableDescriptor> GetDeviceRayTracingShaderTableDescriptor(int deviceIndex);

            // accessors
            const RHI::Ptr<MultiDeviceRayTracingPipelineState>& GetPipelineState() const
            {
                return m_rayTracingPipelineState;
            }

            const MultiDeviceRayTracingShaderTableRecordList& GetRayGenerationRecord() const
            {
                return m_rayGenerationRecord;
            }
            MultiDeviceRayTracingShaderTableRecordList& GetRayGenerationRecord()
            {
                return m_rayGenerationRecord;
            }

            const MultiDeviceRayTracingShaderTableRecordList& GetMissRecords() const
            {
                return m_missRecords;
            }
            MultiDeviceRayTracingShaderTableRecordList& GetMissRecords()
            {
                return m_missRecords;
            }

            const MultiDeviceRayTracingShaderTableRecordList& GetHitGroupRecords() const
            {
                return m_hitGroupRecords;
            }
            MultiDeviceRayTracingShaderTableRecordList& GetHitGroupRecords()
            {
                return m_hitGroupRecords;
            }

            void RemoveHitGroupRecords(uint32_t key);

            // build operations
            MultiDeviceRayTracingShaderTableDescriptor* Build(
                const AZ::Name& name, RHI::Ptr<MultiDeviceRayTracingPipelineState>& rayTracingPipelineState);
            MultiDeviceRayTracingShaderTableDescriptor* RayGenerationRecord(const AZ::Name& name);
            MultiDeviceRayTracingShaderTableDescriptor* MissRecord(const AZ::Name& name);
            MultiDeviceRayTracingShaderTableDescriptor* HitGroupRecord(
                const AZ::Name& name, uint32_t key = MultiDeviceRayTracingShaderTableRecord::InvalidKey);
            MultiDeviceRayTracingShaderTableDescriptor* ShaderResourceGroup(const RHI::MultiDeviceShaderResourceGroup* shaderResourceGroup);

        private:
            AZ::Name m_name;
            RHI::Ptr<MultiDeviceRayTracingPipelineState> m_rayTracingPipelineState;
            MultiDeviceRayTracingShaderTableRecordList
                m_rayGenerationRecord; // limited to one record, but stored as a list to simplify processing
            MultiDeviceRayTracingShaderTableRecordList m_missRecords;
            MultiDeviceRayTracingShaderTableRecordList m_hitGroupRecords;

            MultiDeviceRayTracingShaderTableRecord* m_buildContext = nullptr;
        };

        //! Shader Table
        //! Specifies the ray generation, miss, and hit shaders used during the ray tracing process
        class MultiDeviceRayTracingShaderTable : public MultiDeviceObject
        {
        public:
            AZ_CLASS_ALLOCATOR(MultiDeviceRayTracingShaderTable, AZ::SystemAllocator, 0);
            AZ_RTTI(MultiDeviceRayTracingShaderTable, "{B448997B-A8E6-446E-A333-EFD92B486D9B}", MultiDeviceObject)
            MultiDeviceRayTracingShaderTable() = default;
            virtual ~MultiDeviceRayTracingShaderTable() = default;

            const RHI::Ptr<RayTracingShaderTable>& GetDeviceRayTracingShaderTable(int deviceIndex) const
            {
                AZ_Error(
                    "MultiDeviceRayTracingShaderTable",
                    m_deviceRayTracingShaderTables.find(deviceIndex) != m_deviceRayTracingShaderTables.end(),
                    "No RayTracingShaderTable found for device index %d\n",
                    deviceIndex);

                return m_deviceRayTracingShaderTables.at(deviceIndex);
            }

            void Init(MultiDevice::DeviceMask deviceMask, const MultiDeviceRayTracingBufferPools& rayTracingBufferPools);

            //! Queues this MultiDeviceRayTracingShaderTable to be built by the FrameScheduler.
            //! Note that the descriptor must be heap allocated, preferably using make_shared.
            void Build(const AZStd::shared_ptr<MultiDeviceRayTracingShaderTableDescriptor> descriptor);

        private:
            AZStd::unordered_map<int, Ptr<RayTracingShaderTable>> m_deviceRayTracingShaderTables;
        };
    } // namespace RHI
} // namespace AZ
