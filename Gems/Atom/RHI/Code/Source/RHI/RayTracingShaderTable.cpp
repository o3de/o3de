/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/RayTracingBufferPools.h>
#include <Atom/RHI/RayTracingShaderTable.h>
#include <Atom/RHI/ShaderResourceGroup.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <AzCore/std/smart_ptr/make_shared.h>

namespace AZ::RHI
{
    RayTracingShaderTableRecord::RayTracingShaderTableRecord(const Name& shaderExportName)
        : m_shaderExportName(shaderExportName)
    {
    }

    AZStd::shared_ptr<DeviceRayTracingShaderTableDescriptor> RayTracingShaderTableDescriptor::GetDeviceRayTracingShaderTableDescriptor(
        int deviceIndex)
    {
        AZ_Assert(m_rayGenerationRecord.size() <= 1, "Only one ray generation record is allowed");

        AZStd::shared_ptr<DeviceRayTracingShaderTableDescriptor> descriptor = AZStd::make_shared<DeviceRayTracingShaderTableDescriptor>();

        if (m_rayTracingPipelineState)
        {
            descriptor->m_name = m_name;
            descriptor->m_rayTracingPipelineState = m_rayTracingPipelineState->GetDeviceRayTracingPipelineState(deviceIndex);
        }

        for (const auto& rayGenerationRecord : m_rayGenerationRecord)
        {
            auto& deviceRayGenerationRecord = descriptor->m_rayGenerationRecord.emplace_back();
            deviceRayGenerationRecord.m_shaderExportName = rayGenerationRecord.m_shaderExportName;
            if (rayGenerationRecord.m_shaderResourceGroup)
            {
                deviceRayGenerationRecord.m_shaderResourceGroup =
                    rayGenerationRecord.m_shaderResourceGroup->GetDeviceShaderResourceGroup(deviceIndex).get();
            }
        }

        for (const auto& missRecord : m_missRecords)
        {
            auto& deviceMissRecord = descriptor->m_missRecords.emplace_back();
            deviceMissRecord.m_shaderExportName = missRecord.m_shaderExportName;
            if (missRecord.m_shaderResourceGroup)
            {
                deviceMissRecord.m_shaderResourceGroup = missRecord.m_shaderResourceGroup->GetDeviceShaderResourceGroup(deviceIndex).get();
            }
        }

        for (const auto& hitGroupRecord : m_hitGroupRecords)
        {
            auto& deviceHitGroupRecord = descriptor->m_hitGroupRecords.emplace_back();
            deviceHitGroupRecord.m_shaderExportName = hitGroupRecord.m_shaderExportName;
            deviceHitGroupRecord.m_key = hitGroupRecord.m_key;
            if (hitGroupRecord.m_shaderResourceGroup)
            {
                deviceHitGroupRecord.m_shaderResourceGroup =
                    hitGroupRecord.m_shaderResourceGroup->GetDeviceShaderResourceGroup(deviceIndex).get();
            }
        }

        return descriptor;
    }

    void RayTracingShaderTableDescriptor::RemoveHitGroupRecords(uint32_t key)
    {
        for (RayTracingShaderTableRecordList::iterator itHitGroup = m_hitGroupRecords.begin();
             itHitGroup != m_hitGroupRecords.end();
             ++itHitGroup)
        {
            RayTracingShaderTableRecord& record = *itHitGroup;
            if (record.m_key == key)
            {
                m_hitGroupRecords.erase(itHitGroup);
            }
        }
    }

    void RayTracingShaderTable::Init(MultiDevice::DeviceMask deviceMask, const RayTracingBufferPools& bufferPools)
    {
        MultiDeviceObject::Init(deviceMask);

        IterateDevices(
            [this, &bufferPools](auto deviceIndex)
            {
                auto device = RHISystemInterface::Get()->GetDevice(deviceIndex);
                m_deviceObjects[deviceIndex] = Factory::Get().CreateRayTracingShaderTable();

                auto deviceBufferPool{ bufferPools.GetDeviceRayTracingBufferPools(deviceIndex).get() };

                GetDeviceRayTracingShaderTable(deviceIndex)->Init(*device, *deviceBufferPool);

                return true;
            });

        if (const auto& name = GetName(); !name.IsEmpty())
        {
            SetName(name);
        }
    }

    void RayTracingShaderTable::Build(const AZStd::shared_ptr<RayTracingShaderTableDescriptor> descriptor)
    {
        IterateObjects<DeviceRayTracingShaderTable>(
            [&descriptor](auto deviceIndex, auto deviceRayTracingShaderTable)
            {
                deviceRayTracingShaderTable->Build(descriptor->GetDeviceRayTracingShaderTableDescriptor(deviceIndex));
            });
    }
} // namespace AZ::RHI
