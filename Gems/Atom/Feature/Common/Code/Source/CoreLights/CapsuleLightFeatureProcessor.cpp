/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CoreLights/CapsuleLightFeatureProcessor.h>

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
        void CapsuleLightFeatureProcessor::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext
                    ->Class<CapsuleLightFeatureProcessor, FeatureProcessor>()
                    ->Version(0);
            }
        }

        CapsuleLightFeatureProcessor::CapsuleLightFeatureProcessor()
            : CapsuleLightFeatureProcessorInterface()
        {
        }

        void CapsuleLightFeatureProcessor::Activate()
        {
            GpuBufferHandler::Descriptor desc;
            desc.m_bufferName = "CapsuleLightBuffer";
            desc.m_bufferSrgName = "m_capsuleLights";
            desc.m_elementCountSrgName = "m_capsuleLightCount";
            desc.m_elementSize = sizeof(CapsuleLightData);
            desc.m_srgLayout = RPI::RPISystemInterface::Get()->GetViewSrgLayout().get();

            m_lightBufferHandler = GpuBufferHandler(desc);
        }

        void CapsuleLightFeatureProcessor::Deactivate()
        {
            m_capsuleLightData.Clear();
            m_lightBufferHandler.Release();
        }

        CapsuleLightFeatureProcessor::LightHandle CapsuleLightFeatureProcessor::AcquireLight()
        {
            uint16_t id = m_capsuleLightData.GetFreeSlotIndex();

            if (id == IndexedDataVector<CapsuleLightData>::NoFreeSlot)
            {
                return LightHandle(LightHandle::NullIndex);
            }
            else
            {
                m_deviceBufferNeedsUpdate = true;
                return LightHandle(id);
            }
        }

        bool CapsuleLightFeatureProcessor::ReleaseLight(LightHandle& handle)
        {
            if (handle.IsValid())
            {
                m_capsuleLightData.RemoveIndex(handle.GetIndex());
                m_deviceBufferNeedsUpdate = true;
                handle.Reset();
                return true;
            }
            return false;
        }

        CapsuleLightFeatureProcessor::LightHandle CapsuleLightFeatureProcessor::CloneLight(LightHandle sourceLightHandle)
        {
            AZ_Assert(sourceLightHandle.IsValid(), "Invalid LightHandle passed to CapsuleLightFeatureProcessor::CloneLight().");

            LightHandle handle = AcquireLight();
            if (handle.IsValid())
            {
                m_capsuleLightData.GetData(handle.GetIndex()) = m_capsuleLightData.GetData(sourceLightHandle.GetIndex());
                m_deviceBufferNeedsUpdate = true;
            }
            return handle;
        }

        void CapsuleLightFeatureProcessor::Simulate(const FeatureProcessor::SimulatePacket& packet)
        {
            AZ_PROFILE_SCOPE(RPI, "CapsuleLightFeatureProcessor: Simulate");
            AZ_UNUSED(packet);

            if (m_deviceBufferNeedsUpdate)
            {
                m_lightBufferHandler.UpdateBuffer(m_capsuleLightData.GetDataVector());
                m_deviceBufferNeedsUpdate = false;
            }
        }

        void CapsuleLightFeatureProcessor::Render(const CapsuleLightFeatureProcessor::RenderPacket& packet)
        {
            AZ_PROFILE_SCOPE(RPI, "CapsuleLightFeatureProcessor: Render");

            for (const RPI::ViewPtr& view : packet.m_views)
            {
                m_lightBufferHandler.UpdateSrg(view->GetShaderResourceGroup().get());
            }
        }

        void CapsuleLightFeatureProcessor::SetRgbIntensity(LightHandle handle, const PhotometricColor<PhotometricUnitType>& lightRgbIntensity)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to CapsuleLightFeatureProcessor::SetRgbIntensity().");

            auto transformedColor = AZ::RPI::TransformColor(lightRgbIntensity, AZ::RPI::ColorSpaceId::LinearSRGB, AZ::RPI::ColorSpaceId::ACEScg);

            auto& rgbIntensity = m_capsuleLightData.GetData(handle.GetIndex()).m_rgbIntensity;
            rgbIntensity[0] = transformedColor.GetR();
            rgbIntensity[1] = transformedColor.GetG();
            rgbIntensity[2] = transformedColor.GetB();

            m_deviceBufferNeedsUpdate = true;
        }

        void CapsuleLightFeatureProcessor::SetCapsuleLineSegment(LightHandle handle, const Vector3& startPoint, const Vector3& endPoint)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to CapsuleLightFeatureProcessor::SetCapsuleLineSegment().");

            CapsuleLightData& capsuleData = m_capsuleLightData.GetData(handle.GetIndex());
            startPoint.StoreToFloat3(capsuleData.m_startPoint.data());

            if (startPoint.IsClose(endPoint))
            {
                capsuleData.m_length = 0;
                Vector3::CreateAxisX().StoreToFloat3(capsuleData.m_direction.data());
            }
            else
            {
                Vector3 direction = endPoint - startPoint;
                capsuleData.m_length = direction.GetLength();
                direction.Normalize();
                direction.StoreToFloat3(capsuleData.m_direction.data());
            }

            m_deviceBufferNeedsUpdate = true;
        }

        void CapsuleLightFeatureProcessor::SetAttenuationRadius(LightHandle handle, float attenuationRadius)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to CapsuleLightFeatureProcessor::SetAttenuationRadius().");

            attenuationRadius = AZStd::max<float>(attenuationRadius, 0.001f); // prevent divide by zero.
            m_capsuleLightData.GetData(handle.GetIndex()).m_invAttenuationRadiusSquared = 1.0f / (attenuationRadius * attenuationRadius);
            m_deviceBufferNeedsUpdate = true;
        }

        void CapsuleLightFeatureProcessor::SetCapsuleRadius(LightHandle handle, float radius)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to CapsuleLightFeatureProcessor::SetCapsuleRadius().");

            m_capsuleLightData.GetData(handle.GetIndex()).m_radius = radius;
            m_deviceBufferNeedsUpdate = true;
        }

        void CapsuleLightFeatureProcessor::SetCapsuleData(LightHandle handle, const CapsuleLightData& data)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to CapsuleLightFeatureProcessor::SetCapsuleData().");

            m_capsuleLightData.GetData(handle.GetIndex()) = data;
            m_deviceBufferNeedsUpdate = true;
        }

        const Data::Instance<RPI::Buffer> CapsuleLightFeatureProcessor::GetLightBuffer()const
        {
            return m_lightBufferHandler.GetBuffer();
        }

        uint32_t CapsuleLightFeatureProcessor::GetLightCount() const
        {
            return m_lightBufferHandler.GetElementCount();
        }

    } // namespace Render
} // namespace AZ
