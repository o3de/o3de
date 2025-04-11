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
        AZStd::shared_ptr<DeviceRayTracingShaderTableDescriptor> descriptor = AZStd::make_shared<DeviceRayTracingShaderTableDescriptor>();

        if (m_rayTracingPipelineState)
        {
            descriptor->Build(m_name, m_rayTracingPipelineState->GetDeviceRayTracingPipelineState(deviceIndex));
        }

        for (const auto& rayGenerationRecord : m_rayGenerationRecord)
        {
            descriptor->RayGenerationRecord(rayGenerationRecord.m_shaderExportName);
            if (rayGenerationRecord.m_shaderResourceGroup)
            {
                descriptor->ShaderResourceGroup(
                    rayGenerationRecord.m_shaderResourceGroup->GetDeviceShaderResourceGroup(deviceIndex).get());
            }
        }

        for (const auto& missRecord : m_missRecords)
        {
            descriptor->MissRecord(missRecord.m_shaderExportName);
            if (missRecord.m_shaderResourceGroup)
            {
                descriptor->ShaderResourceGroup(missRecord.m_shaderResourceGroup->GetDeviceShaderResourceGroup(deviceIndex).get());
            }
        }

        for (const auto& hitGroupRecord : m_hitGroupRecords)
        {
            descriptor->HitGroupRecord(hitGroupRecord.m_shaderExportName, hitGroupRecord.m_key);
            if (hitGroupRecord.m_shaderResourceGroup)
            {
                descriptor->ShaderResourceGroup(hitGroupRecord.m_shaderResourceGroup->GetDeviceShaderResourceGroup(deviceIndex).get());
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

    RayTracingShaderTableDescriptor* RayTracingShaderTableDescriptor::Build(
        const AZ::Name& name, RHI::Ptr<RayTracingPipelineState>& rayTracingPipelineState)
    {
        m_name = name;
        m_rayTracingPipelineState = rayTracingPipelineState;
        return this;
    }

    RayTracingShaderTableDescriptor* RayTracingShaderTableDescriptor::RayGenerationRecord(const AZ::Name& name)
    {
        AZ_Assert(m_rayGenerationRecord.empty(), "Ray generation record already added");
        m_rayGenerationRecord.emplace_back(RayTracingShaderTableRecord{ name });
        m_buildContext = &m_rayGenerationRecord.back();
        return this;
    }

    RayTracingShaderTableDescriptor* RayTracingShaderTableDescriptor::MissRecord(const AZ::Name& name)
    {
        m_missRecords.emplace_back(RayTracingShaderTableRecord{ name });
        m_buildContext = &m_missRecords.back();
        return this;
    }

    RayTracingShaderTableDescriptor* RayTracingShaderTableDescriptor::HitGroupRecord(
        const AZ::Name& name, uint32_t key /* = RayTracingShaderTableRecord::InvalidKey */)
    {
        m_hitGroupRecords.emplace_back(RayTracingShaderTableRecord{ name, nullptr, key });
        m_buildContext = &m_hitGroupRecords.back();
        return this;
    }

    RayTracingShaderTableDescriptor* RayTracingShaderTableDescriptor::ShaderResourceGroup(
        const RHI::ShaderResourceGroup* shaderResourceGroup)
    {
        AZ_Assert(m_buildContext, "ShaderResourceGroup can only be added to a shader table record");
        AZ_Assert(m_buildContext->m_shaderResourceGroup == nullptr, "Records can only have one ShaderResourceGroup");
        m_buildContext->m_shaderResourceGroup = shaderResourceGroup;
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
