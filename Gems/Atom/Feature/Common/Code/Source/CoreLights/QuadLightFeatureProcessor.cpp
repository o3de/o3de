/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CoreLights/QuadLightFeatureProcessor.h>
#include <CoreLights/LightCommon.h>
#include <CoreLights/LtcCommon.h>
#include <Mesh/MeshFeatureProcessor.h>

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
                    ->Version(1);
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

            MeshFeatureProcessor* meshFeatureProcessor = GetParentScene()->GetFeatureProcessor<MeshFeatureProcessor>();
            if (meshFeatureProcessor)
            {
                m_lightLtcMeshFlag = meshFeatureProcessor->GetShaderOptionFlagRegistry()->AcquireTag(AZ::Name("o_enableQuadLightLTC"));
                m_lightApproxMeshFlag = meshFeatureProcessor->GetShaderOptionFlagRegistry()->AcquireTag(AZ::Name("o_enableQuadLightApprox"));
            }
        }

        void QuadLightFeatureProcessor::Deactivate()
        {
            m_lightData.Clear();
            m_lightBufferHandler.Release();
        }

        QuadLightFeatureProcessor::LightHandle QuadLightFeatureProcessor::AcquireLight()
        {
            uint16_t id = m_lightData.GetFreeSlotIndex();

            if (id == m_lightData.NoFreeSlot)
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
                m_lightData.RemoveIndex(handle.GetIndex());
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
                m_lightData.GetData<0>(handle.GetIndex()) = m_lightData.GetData<0>(sourceLightHandle.GetIndex());
                m_lightData.GetData<1>(handle.GetIndex()) = m_lightData.GetData<1>(sourceLightHandle.GetIndex());
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
                m_lightBufferHandler.UpdateBuffer(m_lightData.GetDataVector<0>());
                m_deviceBufferNeedsUpdate = false;
            }

            if (r_enablePerMeshShaderOptionFlags)
            {
                auto usesLtc = [&](const MeshCommon::BoundsVariant& bounds) -> bool
                {
                    LightHandle::IndexType index = m_lightData.GetIndexForData<1>(&bounds);
                    return (m_lightData.GetData<0>(index).m_flags & QuadLightFlag::UseFastApproximation) == 0;
                };
                auto usesFastApproximation = [&](const MeshCommon::BoundsVariant& bounds) -> bool
                {
                    LightHandle::IndexType index = m_lightData.GetIndexForData<1>(&bounds);
                    return (m_lightData.GetData<0>(index).m_flags & QuadLightFlag::UseFastApproximation) > 0;
                };

                MeshCommon::MarkMeshesWithFlag(GetParentScene(), AZStd::span(m_lightData.GetDataVector<1>()), m_lightLtcMeshFlag.GetIndex(), usesLtc);
                MeshCommon::MarkMeshesWithFlag(GetParentScene(), AZStd::span(m_lightData.GetDataVector<1>()), m_lightApproxMeshFlag.GetIndex(), usesFastApproximation);
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

            AZStd::array<float, 3>& rgbIntensity = m_lightData.GetData<0>(handle.GetIndex()).m_rgbIntensityNits;
            rgbIntensity[0] = transformedColor.GetR();
            rgbIntensity[1] = transformedColor.GetG();
            rgbIntensity[2] = transformedColor.GetB();

            m_deviceBufferNeedsUpdate = true;
        }

        void QuadLightFeatureProcessor::SetPosition(LightHandle handle, const AZ::Vector3& lightPosition)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to QuadLightFeatureProcessor::SetPosition().");

            AZStd::array<float, 3>& position = m_lightData.GetData<0>(handle.GetIndex()).m_position;
            lightPosition.StoreToFloat3(position.data());

            UpdateBounds(handle);

            m_deviceBufferNeedsUpdate = true;
        }

        void QuadLightFeatureProcessor::SetOrientation(LightHandle handle, const AZ::Quaternion& orientation)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to QuadLightFeatureProcessor::SetOrientation().");

            QuadLightData& data = m_lightData.GetData<0>(handle.GetIndex());
            orientation.TransformVector(Vector3::CreateAxisX()).StoreToFloat3(data.m_leftDir.data());
            orientation.TransformVector(Vector3::CreateAxisY()).StoreToFloat3(data.m_upDir.data());

            UpdateBounds(handle);

            m_deviceBufferNeedsUpdate = true;
        }

        void QuadLightFeatureProcessor::SetLightEmitsBothDirections(LightHandle handle, bool lightEmitsBothDirections)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to QuadLightFeatureProcessor::SetLightEmitsBothDirections().");

            m_lightData.GetData<0>(handle.GetIndex()).SetFlag(QuadLightFlag::EmitBothDirections, lightEmitsBothDirections);

            UpdateBounds(handle);

            m_deviceBufferNeedsUpdate = true;
        }

        void QuadLightFeatureProcessor::SetUseFastApproximation(LightHandle handle, bool useFastApproximation)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to QuadLightFeatureProcessor::SetLightEmitsBothDirections().");

            m_lightData.GetData<0>(handle.GetIndex()).SetFlag(QuadLightFlag::UseFastApproximation, useFastApproximation);
            m_deviceBufferNeedsUpdate = true;
        }

        void QuadLightFeatureProcessor::SetAttenuationRadius(LightHandle handle, float attenuationRadius)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to QuadLightFeatureProcessor::SetAttenuationRadius().");

            attenuationRadius = AZStd::max<float>(attenuationRadius, 0.001f); // prevent divide by zero.
            m_lightData.GetData<0>(handle.GetIndex()).m_invAttenuationRadiusSquared = 1.0f / (attenuationRadius * attenuationRadius);

            UpdateBounds(handle);

            m_deviceBufferNeedsUpdate = true;
        }

        void QuadLightFeatureProcessor::SetQuadDimensions(LightHandle handle, float width, float height)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to QuadLightFeatureProcessor::SetQuadDimensions().");

            QuadLightData& data = m_lightData.GetData<0>(handle.GetIndex());
            data.m_halfWidth = width * 0.5f;
            data.m_halfHeight = height * 0.5f;

            UpdateBounds(handle);

            m_deviceBufferNeedsUpdate = true;
        }

        void QuadLightFeatureProcessor::SetAffectsGI(LightHandle handle, bool affectsGI)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to QuadLightFeatureProcessor::SetAffectsGI().");

            m_lightData.GetData<0>(handle.GetIndex()).m_affectsGI = affectsGI;
            m_deviceBufferNeedsUpdate = true;
        }

        void QuadLightFeatureProcessor::SetAffectsGIFactor(LightHandle handle, float affectsGIFactor)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to QuadLightFeatureProcessor::SetAffectsGIFactor().");

            m_lightData.GetData<0>(handle.GetIndex()).m_affectsGIFactor = affectsGIFactor;
            m_deviceBufferNeedsUpdate = true;
        }

        void QuadLightFeatureProcessor::SetLightingChannelMask(LightHandle handle, uint32_t lightingChannelMask)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to QuadLightFeatureProcessor::SetLightingChannelMask().");

            m_lightData.GetData<0>(handle.GetIndex()).m_lightingChannelMask = lightingChannelMask;
            m_deviceBufferNeedsUpdate = true;
        }

        void QuadLightFeatureProcessor::SetQuadData(LightHandle handle, const QuadLightData& data)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to QuadLightFeatureProcessor::SetQuadData().");

            m_lightData.GetData<0>(handle.GetIndex()) = data;

            UpdateBounds(handle);

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

        void QuadLightFeatureProcessor::UpdateBounds(LightHandle handle)
        {
            const QuadLightData& data = m_lightData.GetData<0>(handle.GetIndex());
            MeshCommon::BoundsVariant& bounds = m_lightData.GetData<1>(handle.GetIndex());

            AZ::Vector3 position = AZ::Vector3::CreateFromFloat3(data.m_position.data());
            float radius = LightCommon::GetRadiusFromInvRadiusSquared(data.m_invAttenuationRadiusSquared);

            if ((data.m_flags & QuadLightFlag::EmitBothDirections) > 0)
            {
                bounds.emplace<Sphere>(AZ::Sphere(position, radius));
            }
            else
            {
                AZ::Vector3 normal = AZ::Vector3::CreateFromFloat3(data.m_upDir.data());
                bounds.emplace<Hemisphere>(AZ::Hemisphere(position, radius, normal));
            }
        }

    } // namespace Render
} // namespace AZ
