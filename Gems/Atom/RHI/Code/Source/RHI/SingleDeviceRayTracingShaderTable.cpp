/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/SingleDeviceRayTracingShaderTable.h>
#include <Atom/RHI/RHISystemInterface.h>

namespace AZ::RHI
{
    void SingleDeviceRayTracingShaderTableDescriptor::RemoveHitGroupRecords(uint32_t key)
    {
        for (RayTracingShaderTableRecordList::iterator itHitGroup = m_hitGroupRecords.begin(); itHitGroup != m_hitGroupRecords.end(); ++itHitGroup)
        {
            SingleDeviceRayTracingShaderTableRecord& record = *itHitGroup;
            if (record.m_key == key)
            {
                m_hitGroupRecords.erase(itHitGroup);
            }
        }
    }

    SingleDeviceRayTracingShaderTableDescriptor* SingleDeviceRayTracingShaderTableDescriptor::Build(const AZ::Name& name, const RHI::Ptr<SingleDeviceRayTracingPipelineState>& rayTracingPipelineState)
    {
        m_name = name;
        m_rayTracingPipelineState = rayTracingPipelineState;
        return this;
    }

    SingleDeviceRayTracingShaderTableDescriptor* SingleDeviceRayTracingShaderTableDescriptor::RayGenerationRecord(const AZ::Name& name)
    {
        AZ_Assert(m_rayGenerationRecord.empty(), "Ray generation record already added");
        m_rayGenerationRecord.emplace_back();
        m_buildContext = &m_rayGenerationRecord.back();
        m_buildContext->m_shaderExportName = name;
        return this;
    }

    SingleDeviceRayTracingShaderTableDescriptor* SingleDeviceRayTracingShaderTableDescriptor::MissRecord(const AZ::Name& name)
    {
        m_missRecords.emplace_back();
        m_buildContext = &m_missRecords.back();
        m_buildContext->m_shaderExportName = name;
        return this;
    }

    SingleDeviceRayTracingShaderTableDescriptor* SingleDeviceRayTracingShaderTableDescriptor::CallableRecord(const AZ::Name& name)
    {
        m_callableRecords.emplace_back();
        m_buildContext = &m_callableRecords.back();
        m_buildContext->m_shaderExportName = name;
        return this;
    }

    SingleDeviceRayTracingShaderTableDescriptor* SingleDeviceRayTracingShaderTableDescriptor::HitGroupRecord(const AZ::Name& name, uint32_t key /* = SingleDeviceRayTracingShaderTableRecord::InvalidKey */)
    {
        m_hitGroupRecords.emplace_back();
        m_buildContext = &m_hitGroupRecords.back();
        m_buildContext->m_shaderExportName = name;
        m_buildContext->m_key = key;
        return this;
    }

    SingleDeviceRayTracingShaderTableDescriptor* SingleDeviceRayTracingShaderTableDescriptor::ShaderResourceGroup(const RHI::SingleDeviceShaderResourceGroup* shaderResourceGroup)
    {
        AZ_Assert(m_buildContext, "SingleDeviceShaderResourceGroup can only be added to a shader table record");
        AZ_Assert(m_buildContext->m_shaderResourceGroup == nullptr, "Records can only have one SingleDeviceShaderResourceGroup");
        m_buildContext->m_shaderResourceGroup = shaderResourceGroup;
        return this;
    }

    RHI::Ptr<RHI::SingleDeviceRayTracingShaderTable> SingleDeviceRayTracingShaderTable::CreateRHIRayTracingShaderTable()
    {
        RHI::Ptr<RHI::SingleDeviceRayTracingShaderTable> rayTracingShaderTable = RHI::Factory::Get().CreateRayTracingShaderTable();
        AZ_Error("SingleDeviceRayTracingShaderTable", rayTracingShaderTable, "Failed to create RHI::SingleDeviceRayTracingShaderTable");
        return rayTracingShaderTable;
    }

    void SingleDeviceRayTracingShaderTable::Init(Device& device, const SingleDeviceRayTracingBufferPools& bufferPools)
    {
#if defined (AZ_RHI_ENABLE_VALIDATION)
        // [GFX TODO][ATOM-5217] Validate shaders in the ray tracing shader table are present in the pipeline state
#endif
        DeviceObject::Init(device);
        m_bufferPools = &bufferPools;
    }

    void SingleDeviceRayTracingShaderTable::Build(const AZStd::shared_ptr<SingleDeviceRayTracingShaderTableDescriptor> descriptor)
    {
        AZ_Assert(!m_isQueuedForBuild, "Attempting to build a SingleDeviceRayTracingShaderTable that's already been queued. Only build once per frame.")
        m_descriptor = descriptor;

        RHI::RHISystemInterface::Get()->QueueRayTracingShaderTableForBuild(this);
        m_isQueuedForBuild = true;
    }

    void SingleDeviceRayTracingShaderTable::Validate()
    {
        AZ_Assert(m_isQueuedForBuild, "Attempting to build a SingleDeviceRayTracingShaderTable that is not queued.");
        AZ_Assert(m_bufferPools, "SingleDeviceRayTracingBufferPools pointer is null.");
    }
}
