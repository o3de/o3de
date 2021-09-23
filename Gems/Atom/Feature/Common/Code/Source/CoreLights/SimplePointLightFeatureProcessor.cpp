/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CoreLights/SimplePointLightFeatureProcessor.h>

#include <AzCore/Debug/EventTrace.h>

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Color.h>

#include <Atom/Feature/CoreLights/CoreLightsConstants.h>

#include <Atom/RHI/Factory.h>

#include <Atom/RPI.Public/ColorManagement/TransformColor.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>

namespace AZ
{
    namespace Render
    {
        void SimplePointLightFeatureProcessor::Reflect(ReflectContext* context)
        {
            if (auto * serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext
                    ->Class<SimplePointLightFeatureProcessor, FeatureProcessor>()
                    ->Version(0);
            }
        }

        SimplePointLightFeatureProcessor::SimplePointLightFeatureProcessor()
            : SimplePointLightFeatureProcessorInterface()
        {
        }

        void SimplePointLightFeatureProcessor::Activate()
        {
            GpuBufferHandler::Descriptor desc;
            desc.m_bufferName = "SimplePointLightBuffer";
            desc.m_bufferSrgName = "m_simplePointLights";
            desc.m_elementCountSrgName = "m_simplePointLightCount";
            desc.m_elementSize = sizeof(SimplePointLightData);
            desc.m_srgLayout = RPI::RPISystemInterface::Get()->GetViewSrgLayout().get();

            m_lightBufferHandler = GpuBufferHandler(desc);
        }

        void SimplePointLightFeatureProcessor::Deactivate()
        {
            m_pointLightData.Clear();
            m_lightBufferHandler.Release();
        }

        SimplePointLightFeatureProcessor::LightHandle SimplePointLightFeatureProcessor::AcquireLight()
        {
            uint16_t id = m_pointLightData.GetFreeSlotIndex();

            if (id == IndexedDataVector<SimplePointLightData>::NoFreeSlot)
            {
                return LightHandle::Null;
            }
            else
            {
                m_deviceBufferNeedsUpdate = true;
                return LightHandle(id);
            }
        }

        bool SimplePointLightFeatureProcessor::ReleaseLight(LightHandle& handle)
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

        SimplePointLightFeatureProcessor::LightHandle SimplePointLightFeatureProcessor::CloneLight(LightHandle sourceLightHandle)
        {
            AZ_Assert(sourceLightHandle.IsValid(), "Invalid LightHandle passed to SimplePointLightFeatureProcessor::CloneLight().");

            LightHandle handle = AcquireLight();
            if (handle.IsValid())
            {
                m_pointLightData.GetData(handle.GetIndex()) = m_pointLightData.GetData(sourceLightHandle.GetIndex());
                m_deviceBufferNeedsUpdate = true;
            }
            return handle;
        }

        void SimplePointLightFeatureProcessor::Simulate(const FeatureProcessor::SimulatePacket& packet)
        {
            AZ_PROFILE_SCOPE(RPI, "SimplePointLightFeatureProcessor: Simulate");
            AZ_UNUSED(packet);

            if (m_deviceBufferNeedsUpdate)
            {
                m_lightBufferHandler.UpdateBuffer(m_pointLightData.GetDataVector());
                m_deviceBufferNeedsUpdate = false;
            }
        }

        void SimplePointLightFeatureProcessor::Render(const SimplePointLightFeatureProcessor::RenderPacket& packet)
        {
            AZ_PROFILE_SCOPE(RPI, "SimplePointLightFeatureProcessor: Render");

            for (const RPI::ViewPtr& view : packet.m_views)
            {
                m_lightBufferHandler.UpdateSrg(view->GetShaderResourceGroup().get());
            }
        }

        void SimplePointLightFeatureProcessor::SetRgbIntensity(LightHandle handle, const PhotometricColor<PhotometricUnitType>& lightRgbIntensity)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to SimplePointLightFeatureProcessor::SetRgbIntensity().");

            auto transformedColor = AZ::RPI::TransformColor(lightRgbIntensity, AZ::RPI::ColorSpaceId::LinearSRGB, AZ::RPI::ColorSpaceId::ACEScg);

            AZStd::array<float, 3>& rgbIntensity = m_pointLightData.GetData(handle.GetIndex()).m_rgbIntensity;
            rgbIntensity[0] = transformedColor.GetR();
            rgbIntensity[1] = transformedColor.GetG();
            rgbIntensity[2] = transformedColor.GetB();

            m_deviceBufferNeedsUpdate = true;
        }

        void SimplePointLightFeatureProcessor::SetPosition(LightHandle handle, const AZ::Vector3& lightPosition)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to SimplePointLightFeatureProcessor::SetPosition().");

            AZStd::array<float, 3>& position = m_pointLightData.GetData(handle.GetIndex()).m_position;
            lightPosition.StoreToFloat3(position.data());

            m_deviceBufferNeedsUpdate = true;
        }

        void SimplePointLightFeatureProcessor::SetAttenuationRadius(LightHandle handle, float attenuationRadius)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to SimplePointLightFeatureProcessor::SetAttenuationRadius().");

            attenuationRadius = AZStd::max<float>(attenuationRadius, 0.001f); // prevent divide by zero.
            m_pointLightData.GetData(handle.GetIndex()).m_invAttenuationRadiusSquared = 1.0f / (attenuationRadius * attenuationRadius);
            m_deviceBufferNeedsUpdate = true;
        }

        const Data::Instance<RPI::Buffer> SimplePointLightFeatureProcessor::GetLightBuffer() const
        {
            return m_lightBufferHandler.GetBuffer();
        }

        uint32_t SimplePointLightFeatureProcessor::GetLightCount() const
        {
            return m_lightBufferHandler.GetElementCount();
        }

    } // namespace Render
} // namespace AZ
