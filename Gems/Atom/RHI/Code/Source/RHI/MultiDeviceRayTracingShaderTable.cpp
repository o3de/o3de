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

namespace AZ
{
    namespace RHI
    {
        AZStd::shared_ptr<RayTracingShaderTableDescriptor> MultiDeviceRayTracingShaderTableDescriptor::
            GetDeviceRayTracingShaderTableDescriptor(int deviceIndex)
        {
            AZ_Assert(m_rayTracingPipelineState, "No MultiDeviceRayTracingPipelineState available\n");

            AZStd::shared_ptr<RayTracingShaderTableDescriptor> descriptor =
                AZStd::make_shared<RayTracingShaderTableDescriptor>();

            if (m_rayTracingPipelineState)
                descriptor->Build(m_name, m_rayTracingPipelineState->GetDevicePipelineState(deviceIndex));

            for (const auto& rayGenerationRecord : m_rayGenerationRecord)
            {
                descriptor->RayGenerationRecord(rayGenerationRecord.m_shaderExportName);
                if (rayGenerationRecord.m_shaderResourceGroup)
                    descriptor->ShaderResourceGroup(
                        rayGenerationRecord.m_shaderResourceGroup->GetDeviceShaderResourceGroup(deviceIndex).get());
            }

            for (const auto& missRecord : m_missRecords)
            {
                descriptor->MissRecord(missRecord.m_shaderExportName);
                if (missRecord.m_shaderResourceGroup)
                    descriptor->ShaderResourceGroup(missRecord.m_shaderResourceGroup->GetDeviceShaderResourceGroup(deviceIndex).get());
            }

            for (const auto& hitGroupRecord : m_hitGroupRecords)
            {
                descriptor->HitGroupRecord(hitGroupRecord.m_shaderExportName, hitGroupRecord.m_key);
                if (hitGroupRecord.m_shaderResourceGroup)
                    descriptor->ShaderResourceGroup(hitGroupRecord.m_shaderResourceGroup->GetDeviceShaderResourceGroup(deviceIndex).get());
            }

            return descriptor;
        }

        void MultiDeviceRayTracingShaderTableDescriptor::RemoveHitGroupRecords(uint32_t key)
        {
            for (MultiDeviceRayTracingShaderTableRecordList::iterator itHitGroup = m_hitGroupRecords.begin();
                 itHitGroup != m_hitGroupRecords.end();
                 ++itHitGroup)
            {
                MultiDeviceRayTracingShaderTableRecord& record = *itHitGroup;
                if (record.m_key == key)
                {
                    m_hitGroupRecords.erase(itHitGroup);
                }
            }
        }

        MultiDeviceRayTracingShaderTableDescriptor* MultiDeviceRayTracingShaderTableDescriptor::Build(
            const AZ::Name& name, RHI::Ptr<MultiDeviceRayTracingPipelineState>& rayTracingPipelineState)
        {
            m_name = name;
            m_rayTracingPipelineState = rayTracingPipelineState;
            return this;
        }

        MultiDeviceRayTracingShaderTableDescriptor* MultiDeviceRayTracingShaderTableDescriptor::RayGenerationRecord(const AZ::Name& name)
        {
            AZ_Assert(m_rayGenerationRecord.empty(), "Ray generation record already added");
            m_rayGenerationRecord.emplace_back(MultiDeviceRayTracingShaderTableRecord{ name });
            m_buildContext = &m_rayGenerationRecord.back();
            return this;
        }

        MultiDeviceRayTracingShaderTableDescriptor* MultiDeviceRayTracingShaderTableDescriptor::MissRecord(const AZ::Name& name)
        {
            m_missRecords.emplace_back(MultiDeviceRayTracingShaderTableRecord{ name });
            m_buildContext = &m_missRecords.back();
            return this;
        }

        MultiDeviceRayTracingShaderTableDescriptor* MultiDeviceRayTracingShaderTableDescriptor::HitGroupRecord(
            const AZ::Name& name, uint32_t key /* = MultiDeviceRayTracingShaderTableRecord::InvalidKey */)
        {
            m_hitGroupRecords.emplace_back(MultiDeviceRayTracingShaderTableRecord{ name, nullptr, key });
            m_buildContext = &m_hitGroupRecords.back();
            return this;
        }

        MultiDeviceRayTracingShaderTableDescriptor* MultiDeviceRayTracingShaderTableDescriptor::ShaderResourceGroup(
            const RHI::MultiDeviceShaderResourceGroup* shaderResourceGroup)
        {
            AZ_Assert(m_buildContext, "MultiDeviceShaderResourceGroup can only be added to a shader table record");
            AZ_Assert(m_buildContext->m_shaderResourceGroup == nullptr, "Records can only have one MultiDeviceShaderResourceGroup");
            m_buildContext->m_shaderResourceGroup = shaderResourceGroup;
            return this;
        }

        void MultiDeviceRayTracingShaderTable::Init(MultiDevice::DeviceMask deviceMask, const MultiDeviceRayTracingBufferPools& bufferPools)
        {
            MultiDeviceObject::Init(deviceMask);

            IterateDevices(
                [this, &bufferPools](int deviceIndex)
                {
                    auto device = RHISystemInterface::Get()->GetDevice(deviceIndex);
                    m_deviceRayTracingShaderTables[deviceIndex] = Factory::Get().CreateRayTracingShaderTable();

                    auto deviceBufferPool{ bufferPools.GetDeviceRayTracingBufferPool(deviceIndex).get() };

                    m_deviceRayTracingShaderTables[deviceIndex]->Init(*device, *deviceBufferPool);

                    return true;
                });
        }

        void MultiDeviceRayTracingShaderTable::Build(const AZStd::shared_ptr<MultiDeviceRayTracingShaderTableDescriptor> descriptor)
        {
            for (auto& [deviceIndex, shaderTable] : m_deviceRayTracingShaderTables)
            {
                shaderTable->Build(descriptor->GetDeviceRayTracingShaderTableDescriptor(deviceIndex));
            }
        }
    } // namespace RHI
} // namespace AZ
