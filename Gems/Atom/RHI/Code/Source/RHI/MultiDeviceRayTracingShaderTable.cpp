/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/MultiDeviceRayTracingBufferPools.h>
#include <Atom/RHI/MultiDeviceRayTracingShaderTable.h>
#include <Atom/RHI/MultiDeviceShaderResourceGroup.h>
#include <Atom/RHI/RHISystemInterface.h>
#include <AzCore/std/smart_ptr/make_shared.h>

namespace AZ::RHI
{
    AZStd::shared_ptr<SingleDeviceRayTracingShaderTableDescriptor> MultiDeviceRayTracingShaderTableDescriptor::GetDeviceRayTracingShaderTableDescriptor(
        int deviceIndex)
    {
        AZ_Assert(m_mdRayTracingPipelineState, "No MultiDeviceRayTracingPipelineState available\n");

        AZStd::shared_ptr<SingleDeviceRayTracingShaderTableDescriptor> descriptor = AZStd::make_shared<SingleDeviceRayTracingShaderTableDescriptor>();

        if (m_mdRayTracingPipelineState)
        {
            descriptor->Build(m_name, m_mdRayTracingPipelineState->GetDeviceRayTracingPipelineState(deviceIndex));
        }

        for (const auto& rayGenerationRecord : m_mdRayGenerationRecord)
        {
            descriptor->RayGenerationRecord(rayGenerationRecord.m_shaderExportName);
            if (rayGenerationRecord.m_mdShaderResourceGroup)
            {
                descriptor->ShaderResourceGroup(
                    rayGenerationRecord.m_mdShaderResourceGroup->GetDeviceShaderResourceGroup(deviceIndex).get());
            }
        }

        for (const auto& missRecord : m_mdMissRecords)
        {
            descriptor->MissRecord(missRecord.m_shaderExportName);
            if (missRecord.m_mdShaderResourceGroup)
            {
                descriptor->ShaderResourceGroup(missRecord.m_mdShaderResourceGroup->GetDeviceShaderResourceGroup(deviceIndex).get());
            }
        }

        for (const auto& hitGroupRecord : m_mdHitGroupRecords)
        {
            descriptor->HitGroupRecord(hitGroupRecord.m_shaderExportName, hitGroupRecord.m_key);
            if (hitGroupRecord.m_mdShaderResourceGroup)
            {
                descriptor->ShaderResourceGroup(hitGroupRecord.m_mdShaderResourceGroup->GetDeviceShaderResourceGroup(deviceIndex).get());
            }
        }

        return descriptor;
    }

    void MultiDeviceRayTracingShaderTableDescriptor::RemoveHitGroupRecords(uint32_t key)
    {
        for (MultiDeviceRayTracingShaderTableRecordList::iterator itHitGroup = m_mdHitGroupRecords.begin();
             itHitGroup != m_mdHitGroupRecords.end();
             ++itHitGroup)
        {
            MultiDeviceRayTracingShaderTableRecord& record = *itHitGroup;
            if (record.m_key == key)
            {
                m_mdHitGroupRecords.erase(itHitGroup);
            }
        }
    }

    MultiDeviceRayTracingShaderTableDescriptor* MultiDeviceRayTracingShaderTableDescriptor::Build(
        const AZ::Name& name, RHI::Ptr<MultiDeviceRayTracingPipelineState>& rayTracingPipelineState)
    {
        m_name = name;
        m_mdRayTracingPipelineState = rayTracingPipelineState;
        return this;
    }

    MultiDeviceRayTracingShaderTableDescriptor* MultiDeviceRayTracingShaderTableDescriptor::RayGenerationRecord(const AZ::Name& name)
    {
        AZ_Assert(m_mdRayGenerationRecord.empty(), "Ray generation record already added");
        m_mdRayGenerationRecord.emplace_back(MultiDeviceRayTracingShaderTableRecord{ name });
        m_mdBuildContext = &m_mdRayGenerationRecord.back();
        return this;
    }

    MultiDeviceRayTracingShaderTableDescriptor* MultiDeviceRayTracingShaderTableDescriptor::MissRecord(const AZ::Name& name)
    {
        m_mdMissRecords.emplace_back(MultiDeviceRayTracingShaderTableRecord{ name });
        m_mdBuildContext = &m_mdMissRecords.back();
        return this;
    }

    MultiDeviceRayTracingShaderTableDescriptor* MultiDeviceRayTracingShaderTableDescriptor::HitGroupRecord(
        const AZ::Name& name, uint32_t key /* = MultiDeviceRayTracingShaderTableRecord::InvalidKey */)
    {
        m_mdHitGroupRecords.emplace_back(MultiDeviceRayTracingShaderTableRecord{ name, nullptr, key });
        m_mdBuildContext = &m_mdHitGroupRecords.back();
        return this;
    }

    MultiDeviceRayTracingShaderTableDescriptor* MultiDeviceRayTracingShaderTableDescriptor::ShaderResourceGroup(
        const RHI::MultiDeviceShaderResourceGroup* shaderResourceGroup)
    {
        AZ_Assert(m_mdBuildContext, "MultiDeviceShaderResourceGroup can only be added to a shader table record");
        AZ_Assert(m_mdBuildContext->m_mdShaderResourceGroup == nullptr, "Records can only have one MultiDeviceShaderResourceGroup");
        m_mdBuildContext->m_mdShaderResourceGroup = shaderResourceGroup;
        return this;
    }

    void MultiDeviceRayTracingShaderTable::Init(MultiDevice::DeviceMask deviceMask, const MultiDeviceRayTracingBufferPools& bufferPools)
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

    void MultiDeviceRayTracingShaderTable::Build(const AZStd::shared_ptr<MultiDeviceRayTracingShaderTableDescriptor> descriptor)
    {
        IterateObjects<SingleDeviceRayTracingShaderTable>(
            [&descriptor](auto deviceIndex, auto deviceRayTracingShaderTable)
            {
                deviceRayTracingShaderTable->Build(descriptor->GetDeviceRayTracingShaderTableDescriptor(deviceIndex));
            });
    }
} // namespace AZ::RHI
