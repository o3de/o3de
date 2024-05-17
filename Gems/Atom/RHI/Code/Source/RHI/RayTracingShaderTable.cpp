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
    AZStd::shared_ptr<DeviceRayTracingShaderTableDescriptor> RayTracingShaderTableDescriptor::GetDeviceRayTracingShaderTableDescriptor(
        int deviceIndex)
    {
        AZ_Assert(m_RayTracingPipelineState, "No RayTracingPipelineState available\n");

        AZStd::shared_ptr<DeviceRayTracingShaderTableDescriptor> descriptor = AZStd::make_shared<DeviceRayTracingShaderTableDescriptor>();

        if (m_RayTracingPipelineState)
        {
            descriptor->Build(m_name, m_RayTracingPipelineState->GetDeviceRayTracingPipelineState(deviceIndex));
        }

        for (const auto& rayGenerationRecord : m_RayGenerationRecord)
        {
            descriptor->RayGenerationRecord(rayGenerationRecord.m_shaderExportName);
            if (rayGenerationRecord.m_ShaderResourceGroup)
            {
                descriptor->ShaderResourceGroup(
                    rayGenerationRecord.m_ShaderResourceGroup->GetDeviceShaderResourceGroup(deviceIndex).get());
            }
        }

        for (const auto& missRecord : m_MissRecords)
        {
            descriptor->MissRecord(missRecord.m_shaderExportName);
            if (missRecord.m_ShaderResourceGroup)
            {
                descriptor->ShaderResourceGroup(missRecord.m_ShaderResourceGroup->GetDeviceShaderResourceGroup(deviceIndex).get());
            }
        }

        for (const auto& hitGroupRecord : m_HitGroupRecords)
        {
            descriptor->HitGroupRecord(hitGroupRecord.m_shaderExportName, hitGroupRecord.m_key);
            if (hitGroupRecord.m_ShaderResourceGroup)
            {
                descriptor->ShaderResourceGroup(hitGroupRecord.m_ShaderResourceGroup->GetDeviceShaderResourceGroup(deviceIndex).get());
            }
        }

        return descriptor;
    }

    void RayTracingShaderTableDescriptor::RemoveHitGroupRecords(uint32_t key)
    {
        for (RayTracingShaderTableRecordList::iterator itHitGroup = m_HitGroupRecords.begin();
             itHitGroup != m_HitGroupRecords.end();
             ++itHitGroup)
        {
            RayTracingShaderTableRecord& record = *itHitGroup;
            if (record.m_key == key)
            {
                m_HitGroupRecords.erase(itHitGroup);
            }
        }
    }

    RayTracingShaderTableDescriptor* RayTracingShaderTableDescriptor::Build(
        const AZ::Name& name, RHI::Ptr<RayTracingPipelineState>& rayTracingPipelineState)
    {
        m_name = name;
        m_RayTracingPipelineState = rayTracingPipelineState;
        return this;
    }

    RayTracingShaderTableDescriptor* RayTracingShaderTableDescriptor::RayGenerationRecord(const AZ::Name& name)
    {
        AZ_Assert(m_RayGenerationRecord.empty(), "Ray generation record already added");
        m_RayGenerationRecord.emplace_back(RayTracingShaderTableRecord{ name });
        m_BuildContext = &m_RayGenerationRecord.back();
        return this;
    }

    RayTracingShaderTableDescriptor* RayTracingShaderTableDescriptor::MissRecord(const AZ::Name& name)
    {
        m_MissRecords.emplace_back(RayTracingShaderTableRecord{ name });
        m_BuildContext = &m_MissRecords.back();
        return this;
    }

    RayTracingShaderTableDescriptor* RayTracingShaderTableDescriptor::HitGroupRecord(
        const AZ::Name& name, uint32_t key /* = RayTracingShaderTableRecord::InvalidKey */)
    {
        m_HitGroupRecords.emplace_back(RayTracingShaderTableRecord{ name, nullptr, key });
        m_BuildContext = &m_HitGroupRecords.back();
        return this;
    }

    RayTracingShaderTableDescriptor* RayTracingShaderTableDescriptor::ShaderResourceGroup(
        const RHI::ShaderResourceGroup* shaderResourceGroup)
    {
        AZ_Assert(m_BuildContext, "ShaderResourceGroup can only be added to a shader table record");
        AZ_Assert(m_BuildContext->m_ShaderResourceGroup == nullptr, "Records can only have one ShaderResourceGroup");
        m_BuildContext->m_ShaderResourceGroup = shaderResourceGroup;
        return this;
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
