/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <CoreLights/DiskLightFeatureProcessor.h>

#include <AzCore/Debug/EventTrace.h>

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Color.h>

#include <Atom/Feature/CoreLights/CoreLightsConstants.h>

#include <Atom/RHI/CpuProfiler.h>
#include <Atom/RHI/Factory.h>

#include <Atom/RPI.Public/ColorManagement/TransformColor.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>

namespace AZ
{
    namespace Render
    {
        void DiskLightFeatureProcessor::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext
                    ->Class<DiskLightFeatureProcessor, FeatureProcessor>()
                    ->Version(0);
            }
        }

        DiskLightFeatureProcessor::DiskLightFeatureProcessor()
            : DiskLightFeatureProcessorInterface()
        {
        }

        void DiskLightFeatureProcessor::Activate()
        {
            GpuBufferHandler::Descriptor desc;
            desc.m_bufferName = "DiskLightBuffer";
            desc.m_bufferSrgName = "m_diskLights";
            desc.m_elementCountSrgName = "m_diskLightCount";
            desc.m_elementSize = sizeof(DiskLightData);
            desc.m_srgLayout = RPI::RPISystemInterface::Get()->GetViewSrgAsset()->GetLayout();

            m_lightBufferHandler = GpuBufferHandler(desc);
        }

        void DiskLightFeatureProcessor::Deactivate()
        {
            m_diskLightData.Clear();
            m_lightBufferHandler.Release();
        }

        DiskLightFeatureProcessor::LightHandle DiskLightFeatureProcessor::AcquireLight()
        {
            uint16_t id = m_diskLightData.GetFreeSlotIndex();

            if (id == IndexedDataVector<DiskLightData>::NoFreeSlot)
            {
                return LightHandle::Null;
            }
            else
           {
                m_deviceBufferNeedsUpdate = true;
                return LightHandle(id);
            }
        }

        bool DiskLightFeatureProcessor::ReleaseLight(LightHandle& handle)
        {
            if (handle.IsValid())
            {
                m_diskLightData.RemoveIndex(handle.GetIndex());
                m_deviceBufferNeedsUpdate = true;
                handle.Reset();
                return true;
            }
            return false;
        }

        DiskLightFeatureProcessor::LightHandle DiskLightFeatureProcessor::CloneLight(LightHandle sourceLightHandle)
        {
            AZ_Assert(sourceLightHandle.IsValid(), "Invalid LightHandle passed to DiskLightFeatureProcessor::CloneLight().");

            LightHandle handle = AcquireLight();
            if (handle.IsValid())
            {
                m_diskLightData.GetData(handle.GetIndex()) = m_diskLightData.GetData(sourceLightHandle.GetIndex());
                m_deviceBufferNeedsUpdate = true;
            }
            return handle;
        }

        void DiskLightFeatureProcessor::Simulate(const FeatureProcessor::SimulatePacket& packet)
        {
            AZ_ATOM_PROFILE_FUNCTION("RPI", "DiskLightFeatureProcessor: Simulate");
            AZ_UNUSED(packet);

            if (m_deviceBufferNeedsUpdate)
            {
                m_lightBufferHandler.UpdateBuffer(m_diskLightData.GetDataVector());
                m_deviceBufferNeedsUpdate = false;
            }
        }

        void DiskLightFeatureProcessor::Render(const DiskLightFeatureProcessor::RenderPacket& packet)
        {
            AZ_ATOM_PROFILE_FUNCTION("RPI", "DiskLightFeatureProcessor: Simulate");

            for (const RPI::ViewPtr& view : packet.m_views)
            {
                m_lightBufferHandler.UpdateSrg(view->GetShaderResourceGroup().get());
            }
        }

        void DiskLightFeatureProcessor::SetRgbIntensity(LightHandle handle, const PhotometricColor<PhotometricUnitType>& lightRgbIntensity)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to DiskLightFeatureProcessor::SetRgbIntensity().");

            auto transformedColor = AZ::RPI::TransformColor(lightRgbIntensity, AZ::RPI::ColorSpaceId::LinearSRGB, AZ::RPI::ColorSpaceId::ACEScg);

            AZStd::array<float, 3>& rgbIntensity = m_diskLightData.GetData(handle.GetIndex()).m_rgbIntensity;
            rgbIntensity[0] = transformedColor.GetR();
            rgbIntensity[1] = transformedColor.GetG();
            rgbIntensity[2] = transformedColor.GetB();

            m_deviceBufferNeedsUpdate = true;
        }

        void DiskLightFeatureProcessor::SetPosition(LightHandle handle, const AZ::Vector3& lightPosition)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to DiskLightFeatureProcessor::SetPosition().");

            AZStd::array<float, 3>& position = m_diskLightData.GetData(handle.GetIndex()).m_position;
            lightPosition.StoreToFloat3(position.data());

            m_deviceBufferNeedsUpdate = true;
        }

        void DiskLightFeatureProcessor::SetDirection(LightHandle handle, const AZ::Vector3& lightDirection)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to DiskLightFeatureProcessor::SetDirection().");

            AZStd::array<float, 3>& direction = m_diskLightData.GetData(handle.GetIndex()).m_direction;
            lightDirection.StoreToFloat3(direction.data());

            m_deviceBufferNeedsUpdate = true;
        }

        void DiskLightFeatureProcessor::SetLightEmitsBothDirections(LightHandle handle, bool lightEmitsBothDirections)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to DiskLightFeatureProcessor::SetLightEmitsBothDirections().");

            m_diskLightData.GetData(handle.GetIndex()).SetLightEmitsBothDirections(lightEmitsBothDirections);
            m_deviceBufferNeedsUpdate = true;
        }

        void DiskLightFeatureProcessor::SetAttenuationRadius(LightHandle handle, float attenuationRadius)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to DiskLightFeatureProcessor::SetAttenuationRadius().");

            attenuationRadius = AZStd::max<float>(attenuationRadius, 0.001f); // prevent divide by zero.
            m_diskLightData.GetData(handle.GetIndex()).m_invAttenuationRadiusSquared = 1.0f / (attenuationRadius * attenuationRadius);
            m_deviceBufferNeedsUpdate = true;
        }

        void DiskLightFeatureProcessor::SetDiskRadius(LightHandle handle, float radius)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to DiskLightFeatureProcessor::SetDiskRadius().");

            m_diskLightData.GetData(handle.GetIndex()).m_diskRadius = radius;
            m_deviceBufferNeedsUpdate = true;
        }

        void DiskLightFeatureProcessor::SetDiskData(LightHandle handle, const DiskLightData& data)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to DiskLightFeatureProcessor::SetDiskData().");

            m_diskLightData.GetData(handle.GetIndex()) = data;
            m_deviceBufferNeedsUpdate = true;
        }

        const Data::Instance<RPI::Buffer> DiskLightFeatureProcessor::GetLightBuffer()const
        {
            return m_lightBufferHandler.GetBuffer();
        }

        uint32_t DiskLightFeatureProcessor::GetLightCount() const
        {
            return m_lightBufferHandler.GetElementCount();
        }

    } // namespace Render
} // namespace AZ
