/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/DeviceRayTracingShaderTable.h>
#include <Atom/RHI/RHISystemInterface.h>

namespace AZ::RHI
{
    void DeviceRayTracingShaderTableDescriptor::RemoveHitGroupRecords(uint32_t key)
    {
        for (DeviceRayTracingShaderTableRecordList::iterator itHitGroup = m_hitGroupRecords.begin(); itHitGroup != m_hitGroupRecords.end(); ++itHitGroup)
        {
            DeviceRayTracingShaderTableRecord& record = *itHitGroup;
            if (record.m_key == key)
            {
                m_hitGroupRecords.erase(itHitGroup);
            }
        }
    }

    DeviceRayTracingShaderTableDescriptor* DeviceRayTracingShaderTableDescriptor::Build(const AZ::Name& name, const RHI::Ptr<DeviceRayTracingPipelineState>& rayTracingPipelineState)
    {
        m_name = name;
        m_rayTracingPipelineState = rayTracingPipelineState;
        return this;
    }

    DeviceRayTracingShaderTableDescriptor* DeviceRayTracingShaderTableDescriptor::RayGenerationRecord(const AZ::Name& name)
    {
        AZ_Assert(m_rayGenerationRecord.empty(), "Ray generation record already added");
        m_rayGenerationRecord.emplace_back();
        m_buildContext = &m_rayGenerationRecord.back();
        m_buildContext->m_shaderExportName = name;
        return this;
    }

    DeviceRayTracingShaderTableDescriptor* DeviceRayTracingShaderTableDescriptor::MissRecord(const AZ::Name& name)
    {
        m_missRecords.emplace_back();
        m_buildContext = &m_missRecords.back();
        m_buildContext->m_shaderExportName = name;
        return this;
    }

    DeviceRayTracingShaderTableDescriptor* DeviceRayTracingShaderTableDescriptor::CallableRecord(const AZ::Name& name)
    {
        m_callableRecords.emplace_back();
        m_buildContext = &m_callableRecords.back();
        m_buildContext->m_shaderExportName = name;
        return this;
    }

    DeviceRayTracingShaderTableDescriptor* DeviceRayTracingShaderTableDescriptor::HitGroupRecord(const AZ::Name& name, uint32_t key /* = DeviceRayTracingShaderTableRecord::InvalidKey */)
    {
        m_hitGroupRecords.emplace_back();
        m_buildContext = &m_hitGroupRecords.back();
        m_buildContext->m_shaderExportName = name;
        m_buildContext->m_key = key;
        return this;
    }

    DeviceRayTracingShaderTableDescriptor* DeviceRayTracingShaderTableDescriptor::ShaderResourceGroup(const RHI::DeviceShaderResourceGroup* shaderResourceGroup)
    {
        AZ_Assert(m_buildContext, "DeviceShaderResourceGroup can only be added to a shader table record");
        AZ_Assert(m_buildContext->m_shaderResourceGroup == nullptr, "Records can only have one DeviceShaderResourceGroup");
        m_buildContext->m_shaderResourceGroup = shaderResourceGroup;
        return this;
    }

    RHI::Ptr<RHI::DeviceRayTracingShaderTable> DeviceRayTracingShaderTable::CreateRHIRayTracingShaderTable()
    {
        RHI::Ptr<RHI::DeviceRayTracingShaderTable> rayTracingShaderTable = RHI::Factory::Get().CreateRayTracingShaderTable();
        AZ_Error("DeviceRayTracingShaderTable", rayTracingShaderTable, "Failed to create RHI::DeviceRayTracingShaderTable");
        return rayTracingShaderTable;
    }

    void DeviceRayTracingShaderTable::Init(Device& device, const DeviceRayTracingBufferPools& bufferPools)
    {
#if defined (AZ_RHI_ENABLE_VALIDATION)
        // [GFX TODO][ATOM-5217] Validate shaders in the ray tracing shader table are present in the pipeline state
#endif
        DeviceObject::Init(device);
        m_bufferPools = &bufferPools;
    }

    void DeviceRayTracingShaderTable::Build(const AZStd::shared_ptr<DeviceRayTracingShaderTableDescriptor> descriptor)
    {
        AZ_Assert(!m_isQueuedForBuild, "Attempting to build a DeviceRayTracingShaderTable that's already been queued. Only build once per frame.")
        m_descriptor = descriptor;

        RHI::RHISystemInterface::Get()->QueueRayTracingShaderTableForBuild(this);
        m_isQueuedForBuild = true;
    }

    void DeviceRayTracingShaderTable::Validate()
    {
        AZ_Assert(m_isQueuedForBuild, "Attempting to build a DeviceRayTracingShaderTable that is not queued.");
        AZ_Assert(m_bufferPools, "DeviceRayTracingBufferPools pointer is null.");
    }
}
