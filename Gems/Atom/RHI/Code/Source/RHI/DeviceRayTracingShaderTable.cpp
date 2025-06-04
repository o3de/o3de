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
