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

#include <CoreLights/PointLightFeatureProcessor.h>

#include <AzCore/Debug/EventTrace.h>

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Color.h>

#include <Atom/Feature/CoreLights/CoreLightsConstants.h>

#include <Atom/RHI/Factory.h>
#include <Atom/RHI/CpuProfiler.h>

#include <Atom/RPI.Public/ColorManagement/TransformColor.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>

namespace AZ
{
    namespace Render
    {
        void PointLightFeatureProcessor::Reflect(ReflectContext* context)
        {
            if (auto * serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext
                    ->Class<PointLightFeatureProcessor, FeatureProcessor>()
                    ->Version(0);
            }
        }

        PointLightFeatureProcessor::PointLightFeatureProcessor()
            : PointLightFeatureProcessorInterface()
        {
        }

        void PointLightFeatureProcessor::Activate()
        {
            GpuBufferHandler::Descriptor desc;
            desc.m_bufferName = "PointLightBuffer";
            desc.m_bufferSrgName = "m_pointLights";
            desc.m_elementCountSrgName = "m_pointLightCount";
            desc.m_elementSize = sizeof(PointLightData);
            desc.m_srgLayout = RPI::RPISystemInterface::Get()->GetViewSrgAsset()->GetLayout();

            m_lightBufferHandler = GpuBufferHandler(desc);
        }

        void PointLightFeatureProcessor::Deactivate()
        {
            m_pointLightData.Clear();
            m_lightBufferHandler.Release();
        }

        PointLightFeatureProcessor::LightHandle PointLightFeatureProcessor::AcquireLight()
        {
            uint16_t id = m_pointLightData.GetFreeSlotIndex();

            if (id == IndexedDataVector<PointLightData>::NoFreeSlot)
            {
                return LightHandle::Null;
            }
            else
            {
                m_deviceBufferNeedsUpdate = true;
                return LightHandle(id);
            }
        }

        bool PointLightFeatureProcessor::ReleaseLight(LightHandle& handle)
        {
            if (handle.IsValid())
            {
                m_pointLightData.RemoveIndex(handle.GetIndex());
                m_deviceBufferNeedsUpdate = true;
                handle.Reset();
                return true;
            }
            return false;
        }

        PointLightFeatureProcessor::LightHandle PointLightFeatureProcessor::CloneLight(LightHandle sourceLightHandle)
        {
            AZ_Assert(sourceLightHandle.IsValid(), "Invalid LightHandle passed to PointLightFeatureProcessor::CloneLight().");

            LightHandle handle = AcquireLight();
            if (handle.IsValid())
            {
                m_pointLightData.GetData(handle.GetIndex()) = m_pointLightData.GetData(sourceLightHandle.GetIndex());
                m_deviceBufferNeedsUpdate = true;
            }
            return handle;
        }

        void PointLightFeatureProcessor::Simulate(const FeatureProcessor::SimulatePacket& packet)
        {
            AZ_ATOM_PROFILE_FUNCTION("RPI", "PointLightFeatureProcessor: Simulate");
            AZ_UNUSED(packet);

            if (m_deviceBufferNeedsUpdate)
            {
                m_lightBufferHandler.UpdateBuffer(m_pointLightData.GetDataVector());
                m_deviceBufferNeedsUpdate = false;
            }
        }

        void PointLightFeatureProcessor::Render(const PointLightFeatureProcessor::RenderPacket& packet)
        {
            AZ_ATOM_PROFILE_FUNCTION("RPI", "PointLightFeatureProcessor: Render");

            for (const RPI::ViewPtr& view : packet.m_views)
            {
                m_lightBufferHandler.UpdateSrg(view->GetShaderResourceGroup().get());
            }
        }

        void PointLightFeatureProcessor::SetRgbIntensity(LightHandle handle, const PhotometricColor<PhotometricUnitType>& lightRgbIntensity)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to PointLightFeatureProcessor::SetRgbIntensity().");

            auto transformedColor = AZ::RPI::TransformColor(lightRgbIntensity, AZ::RPI::ColorSpaceId::LinearSRGB, AZ::RPI::ColorSpaceId::ACEScg);

            AZStd::array<float, 3>& rgbIntensity = m_pointLightData.GetData(handle.GetIndex()).m_rgbIntensity;
            rgbIntensity[0] = transformedColor.GetR();
            rgbIntensity[1] = transformedColor.GetG();
            rgbIntensity[2] = transformedColor.GetB();

            m_deviceBufferNeedsUpdate = true;
        }

        void PointLightFeatureProcessor::SetPosition(LightHandle handle, const AZ::Vector3& lightPosition)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to PointLightFeatureProcessor::SetPosition().");

            AZStd::array<float, 3>& position = m_pointLightData.GetData(handle.GetIndex()).m_position;
            lightPosition.StoreToFloat3(position.data());

            m_deviceBufferNeedsUpdate = true;
        }

        void PointLightFeatureProcessor::SetAttenuationRadius(LightHandle handle, float attenuationRadius)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to PointLightFeatureProcessor::SetAttenuationRadius().");

            attenuationRadius = AZStd::max<float>(attenuationRadius, 0.001f); // prevent divide by zero.
            m_pointLightData.GetData(handle.GetIndex()).m_invAttenuationRadiusSquared = 1.0f / (attenuationRadius * attenuationRadius);
            m_deviceBufferNeedsUpdate = true;
        }

        void PointLightFeatureProcessor::SetBulbRadius(LightHandle handle, float bulbRadius)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to PointLightFeatureProcessor::SetBulbRadius().");

            m_pointLightData.GetData(handle.GetIndex()).m_bulbRadius = bulbRadius;
            m_deviceBufferNeedsUpdate = true;
        }

        const Data::Instance<RPI::Buffer> PointLightFeatureProcessor::GetLightBuffer() const
        {
            return m_lightBufferHandler.GetBuffer();
        }

        uint32_t PointLightFeatureProcessor::GetLightCount() const
        {
            return m_lightBufferHandler.GetElementCount();
        }

    } // namespace Render
} // namespace AZ
