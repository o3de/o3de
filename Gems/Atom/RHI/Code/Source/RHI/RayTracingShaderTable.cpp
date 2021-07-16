/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/RayTracingShaderTable.h>
#include <Atom/RHI/RHISystemInterface.h>

namespace AZ
{
    namespace RHI
    {
        void RayTracingShaderTableDescriptor::RemoveHitGroupRecords(uint32_t key)
        {
            for (RayTracingShaderTableRecordList::iterator itHitGroup = m_hitGroupRecords.begin(); itHitGroup != m_hitGroupRecords.end(); ++itHitGroup)
            {
                RayTracingShaderTableRecord& record = *itHitGroup;
                if (record.m_key == key)
                {
                    m_hitGroupRecords.erase(itHitGroup);
                }
            }
        }

        RayTracingShaderTableDescriptor* RayTracingShaderTableDescriptor::Build(const AZ::Name& name, RHI::Ptr<RayTracingPipelineState>& rayTracingPipelineState)
        {
            m_name = name;
            m_rayTracingPipelineState = rayTracingPipelineState;
            return this;
        }

        RayTracingShaderTableDescriptor* RayTracingShaderTableDescriptor::RayGenerationRecord(const AZ::Name& name)
        {
            AZ_Assert(m_rayGenerationRecord.empty(), "Ray generation record already added");
            m_rayGenerationRecord.emplace_back();
            m_buildContext = &m_rayGenerationRecord.back();
            m_buildContext->m_shaderExportName = name;
            return this;
        }

        RayTracingShaderTableDescriptor* RayTracingShaderTableDescriptor::MissRecord(const AZ::Name& name)
        {
            m_missRecords.emplace_back();
            m_buildContext = &m_missRecords.back();
            m_buildContext->m_shaderExportName = name;
            return this;
        }

        RayTracingShaderTableDescriptor* RayTracingShaderTableDescriptor::HitGroupRecord(const AZ::Name& name, uint32_t key /* = RayTracingShaderTableRecord::InvalidKey */)
        {
            m_hitGroupRecords.emplace_back();
            m_buildContext = &m_hitGroupRecords.back();
            m_buildContext->m_shaderExportName = name;
            m_buildContext->m_key = key;
            return this;
        }

        RayTracingShaderTableDescriptor* RayTracingShaderTableDescriptor::ShaderResourceGroup(const RHI::ShaderResourceGroup* shaderResourceGroup)
        {
            AZ_Assert(m_buildContext, "ShaderResourceGroup can only be added to a shader table record");
            AZ_Assert(m_buildContext->m_shaderResourceGroup == nullptr, "Records can only have one ShaderResourceGroup");
            m_buildContext->m_shaderResourceGroup = shaderResourceGroup;
            return this;
        }

        RHI::Ptr<RHI::RayTracingShaderTable> RayTracingShaderTable::CreateRHIRayTracingShaderTable()
        {
            RHI::Ptr<RHI::RayTracingShaderTable> rayTracingShaderTable = RHI::Factory::Get().CreateRayTracingShaderTable();
            AZ_Error("RayTracingShaderTable", rayTracingShaderTable, "Failed to create RHI::RayTracingShaderTable");
            return rayTracingShaderTable;
        }

        void RayTracingShaderTable::Init(Device& device, const RayTracingBufferPools& bufferPools)
        {
#if defined (AZ_RHI_ENABLE_VALIDATION)
            // [GFX TODO][ATOM-5217] Validate shaders in the ray tracing shader table are present in the pipeline state
#endif
            DeviceObject::Init(device);
            m_bufferPools = &bufferPools;
        }

        void RayTracingShaderTable::Build(const AZStd::shared_ptr<RayTracingShaderTableDescriptor> descriptor)
        {
            AZ_Assert(!m_isQueuedForBuild, "Attempting to build a RayTracingShaderTable that's already been queued. Only build once per frame.")
            m_descriptor = descriptor;

            RHI::RHISystemInterface::Get()->QueueRayTracingShaderTableForBuild(this);
            m_isQueuedForBuild = true;
        }

        void RayTracingShaderTable::Validate()
        {
            AZ_Assert(m_isQueuedForBuild, "Attempting to build a RayTracingShaderTable that is not queued.");
            AZ_Assert(m_bufferPools, "RayTracingBufferPools pointer is null.");
        }
    }
}
