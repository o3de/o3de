/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CoreLights/QuadLightFeatureProcessor.h>
#include <CoreLights/LtcCommon.h>

#include <AzCore/Debug/EventTrace.h>

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Color.h>

#include <Atom/Feature/CoreLights/CoreLightsConstants.h>

#include <Atom/RHI/Factory.h>

#include <Atom/RPI.Public/ColorManagement/TransformColor.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>

namespace AZ
{
    namespace Render
    {

        void QuadLightFeatureProcessor::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext
                    ->Class<QuadLightFeatureProcessor, FeatureProcessor>()
                    ->Version(0);
            }
        }

        QuadLightFeatureProcessor::QuadLightFeatureProcessor()
            : QuadLightFeatureProcessorInterface()
        {
        }

        void QuadLightFeatureProcessor::Activate()
        {
            GpuBufferHandler::Descriptor desc;
            desc.m_bufferName = "QuadLightBuffer";
            desc.m_bufferSrgName = "m_quadLights";
            desc.m_elementCountSrgName = "m_quadLightCount";
            desc.m_elementSize = sizeof(QuadLightData);
            desc.m_srgLayout = RPI::RPISystemInterface::Get()->GetViewSrgLayout().get();

            m_lightBufferHandler = GpuBufferHandler(desc);

            Interface<ILtcCommon>::Get()->LoadMatricesForSrg(GetParentScene()->GetShaderResourceGroup());
        }

        void QuadLightFeatureProcessor::Deactivate()
        {
            m_quadLightData.Clear();
            m_lightBufferHandler.Release();
        }

        QuadLightFeatureProcessor::LightHandle QuadLightFeatureProcessor::AcquireLight()
        {
            uint16_t id = m_quadLightData.GetFreeSlotIndex();

            if (id == m_quadLightData.NoFreeSlot)
            {
                return LightHandle::Null;
            }
            else
            {
                m_deviceBufferNeedsUpdate = true;
                return LightHandle(id);
            }
        }

        bool QuadLightFeatureProcessor::ReleaseLight(LightHandle& handle)
        {
            if (handle.IsValid())
            {
                m_quadLightData.RemoveIndex(handle.GetIndex());
                m_deviceBufferNeedsUpdate = true;
                handle.Reset();
                return true;
            }
            return false;
        }

        QuadLightFeatureProcessor::LightHandle QuadLightFeatureProcessor::CloneLight(LightHandle sourceLightHandle)
        {
            AZ_Assert(sourceLightHandle.IsValid(), "Invalid LightHandle passed to QuadLightFeatureProcessor::CloneLight().");

            LightHandle handle = AcquireLight();
            if (handle.IsValid())
            {
                m_quadLightData.GetData(handle.GetIndex()) = m_quadLightData.GetData(sourceLightHandle.GetIndex());
                m_deviceBufferNeedsUpdate = true;
            }
            return handle;
        }

        void QuadLightFeatureProcessor::Simulate(const FeatureProcessor::SimulatePacket& packet)
        {
            AZ_PROFILE_SCOPE(RPI, "QuadLightFeatureProcessor: Simulate");
            AZ_UNUSED(packet);

            if (m_deviceBufferNeedsUpdate)
            {
                m_lightBufferHandler.UpdateBuffer(m_quadLightData.GetDataVector());
                m_deviceBufferNeedsUpdate = false;
            }
        }

        void QuadLightFeatureProcessor::Render(const QuadLightFeatureProcessor::RenderPacket& packet)
        {
            AZ_PROFILE_SCOPE(RPI, "QuadLightFeatureProcessor: Render");

            for (const RPI::ViewPtr& view : packet.m_views)
            {
                m_lightBufferHandler.UpdateSrg(view->GetShaderResourceGroup().get());
            }
        }

        void QuadLightFeatureProcessor::SetRgbIntensity(LightHandle handle, const PhotometricColor<PhotometricUnitType>& lightRgbIntensity)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to QuadLightFeatureProcessor::SetRgbIntensity().");

            auto transformedColor = AZ::RPI::TransformColor(lightRgbIntensity, AZ::RPI::ColorSpaceId::LinearSRGB, AZ::RPI::ColorSpaceId::ACEScg);

            AZStd::array<float, 3>& rgbIntensity = m_quadLightData.GetData(handle.GetIndex()).m_rgbIntensityNits;
            rgbIntensity[0] = transformedColor.GetR();
            rgbIntensity[1] = transformedColor.GetG();
            rgbIntensity[2] = transformedColor.GetB();

            m_deviceBufferNeedsUpdate = true;
        }

        void QuadLightFeatureProcessor::SetPosition(LightHandle handle, const AZ::Vector3& lightPosition)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to QuadLightFeatureProcessor::SetPosition().");

            AZStd::array<float, 3>& position = m_quadLightData.GetData(handle.GetIndex()).m_position;
            lightPosition.StoreToFloat3(position.data());

            m_deviceBufferNeedsUpdate = true;
        }

        void QuadLightFeatureProcessor::SetOrientation(LightHandle handle, const AZ::Quaternion& orientation)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to QuadLightFeatureProcessor::SetOrientation().");

            QuadLightData& data = m_quadLightData.GetData(handle.GetIndex());
            orientation.TransformVector(Vector3::CreateAxisX()).StoreToFloat3(data.m_leftDir.data());
            orientation.TransformVector(Vector3::CreateAxisY()).StoreToFloat3(data.m_upDir.data());
            m_deviceBufferNeedsUpdate = true;
        }

        void QuadLightFeatureProcessor::SetLightEmitsBothDirections(LightHandle handle, bool lightEmitsBothDirections)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to QuadLightFeatureProcessor::SetLightEmitsBothDirections().");

            m_quadLightData.GetData(handle.GetIndex()).SetFlag(QuadLightFlag::EmitBothDirections, lightEmitsBothDirections);
            m_deviceBufferNeedsUpdate = true;
        }

        void QuadLightFeatureProcessor::SetUseFastApproximation(LightHandle handle, bool useFastApproximation)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to QuadLightFeatureProcessor::SetLightEmitsBothDirections().");

            m_quadLightData.GetData(handle.GetIndex()).SetFlag(QuadLightFlag::UseFastApproximation, useFastApproximation);
            m_deviceBufferNeedsUpdate = true;
        }

        void QuadLightFeatureProcessor::SetAttenuationRadius(LightHandle handle, float attenuationRadius)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to QuadLightFeatureProcessor::SetAttenuationRadius().");

            attenuationRadius = AZStd::max<float>(attenuationRadius, 0.001f); // prevent divide by zero.
            m_quadLightData.GetData(handle.GetIndex()).m_invAttenuationRadiusSquared = 1.0f / (attenuationRadius * attenuationRadius);
            m_deviceBufferNeedsUpdate = true;
        }

        void QuadLightFeatureProcessor::SetQuadDimensions(LightHandle handle, float width, float height)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to QuadLightFeatureProcessor::SetQuadDimensions().");

            QuadLightData& data = m_quadLightData.GetData(handle.GetIndex());
            data.m_halfWidth = width * 0.5f;
            data.m_halfHeight = height * 0.5f;
            m_deviceBufferNeedsUpdate = true;
        }

        void QuadLightFeatureProcessor::SetQuadData(LightHandle handle, const QuadLightData& data)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to QuadLightFeatureProcessor::SetQuadData().");

            m_quadLightData.GetData(handle.GetIndex()) = data;
            m_deviceBufferNeedsUpdate = true;
        }

        const Data::Instance<RPI::Buffer> QuadLightFeatureProcessor::GetLightBuffer()const
        {
            return m_lightBufferHandler.GetBuffer();
        }

        uint32_t QuadLightFeatureProcessor::GetLightCount() const
        {
            return m_lightBufferHandler.GetElementCount();
        }

    } // namespace Render
} // namespace AZ
