/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CoreLights/CapsuleLightFeatureProcessor.h>

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Color.h>

#include <Atom/Feature/CoreLights/CoreLightsConstants.h>
#include <Atom/Feature/Mesh/MeshCommon.h>
#include <CoreLights/LightCommon.h>
#include <Mesh/MeshFeatureProcessor.h>

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

            MeshFeatureProcessor* meshFeatureProcessor = GetParentScene()->GetFeatureProcessor<MeshFeatureProcessor>();
            if (meshFeatureProcessor)
            {
                m_lightMeshFlag = meshFeatureProcessor->GetShaderOptionFlagRegistry()->AcquireTag(AZ::Name("o_enableCapsuleLights"));
            }
        }

        void CapsuleLightFeatureProcessor::Deactivate()
        {
            m_lightData.Clear();
            m_lightBufferHandler.Release();
        }

        CapsuleLightFeatureProcessor::LightHandle CapsuleLightFeatureProcessor::AcquireLight()
        {
            uint16_t id = m_lightData.GetFreeSlotIndex();

            if (id == MultiIndexedDataVector<CapsuleLightData>::NoFreeSlot)
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
                m_lightData.RemoveIndex(handle.GetIndex());
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
                m_lightData.GetData<0>(handle.GetIndex()) = m_lightData.GetData<0>(sourceLightHandle.GetIndex());
                m_lightData.GetData<1>(handle.GetIndex()) = m_lightData.GetData<1>(sourceLightHandle.GetIndex());
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
                m_lightBufferHandler.UpdateBuffer(m_lightData.GetDataVector<0>());
                m_deviceBufferNeedsUpdate = false;
            }

            if (r_enablePerMeshShaderOptionFlags)
            {
                MeshCommon::MarkMeshesWithFlag(GetParentScene(), AZStd::span(m_lightData.GetDataVector<1>()), m_lightMeshFlag.GetIndex());
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

            auto& rgbIntensity = m_lightData.GetData<0>(handle.GetIndex()).m_rgbIntensity;
            rgbIntensity[0] = transformedColor.GetR();
            rgbIntensity[1] = transformedColor.GetG();
            rgbIntensity[2] = transformedColor.GetB();

            m_deviceBufferNeedsUpdate = true;
        }

        void CapsuleLightFeatureProcessor::SetCapsuleLineSegment(LightHandle handle, const Vector3& startPoint, const Vector3& endPoint)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to CapsuleLightFeatureProcessor::SetCapsuleLineSegment().");

            CapsuleLightData& capsuleData = m_lightData.GetData<0>(handle.GetIndex());
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

            UpdateBounds(handle);

            m_deviceBufferNeedsUpdate = true;
        }

        void CapsuleLightFeatureProcessor::SetAttenuationRadius(LightHandle handle, float attenuationRadius)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to CapsuleLightFeatureProcessor::SetAttenuationRadius().");

            CapsuleLightData& capsuleData = m_lightData.GetData<0>(handle.GetIndex());

            attenuationRadius = AZStd::max<float>(attenuationRadius, 0.001f); // prevent divide by zero.
            capsuleData.m_invAttenuationRadiusSquared = 1.0f / (attenuationRadius * attenuationRadius);

            UpdateBounds(handle);

            m_deviceBufferNeedsUpdate = true;
        }

        void CapsuleLightFeatureProcessor::SetCapsuleRadius(LightHandle handle, float radius)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to CapsuleLightFeatureProcessor::SetCapsuleRadius().");

            m_lightData.GetData<0>(handle.GetIndex()).m_radius = radius;
            UpdateBounds(handle);

            m_deviceBufferNeedsUpdate = true;
        }

        void CapsuleLightFeatureProcessor::SetAffectsGI(LightHandle handle, bool affectsGI)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to CapsuleLightFeatureProcessor::SetAffectsGI().");

            m_lightData.GetData<0>(handle.GetIndex()).m_affectsGI = affectsGI;
            m_deviceBufferNeedsUpdate = true;
        }

        void CapsuleLightFeatureProcessor::SetAffectsGIFactor(LightHandle handle, float affectsGIFactor)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to CapsuleLightFeatureProcessor::SetAffectsGIFactor().");

            m_lightData.GetData<0>(handle.GetIndex()).m_affectsGIFactor = affectsGIFactor;
            m_deviceBufferNeedsUpdate = true;
        }

        void CapsuleLightFeatureProcessor::SetLightingChannelMask(LightHandle handle, uint32_t lightingChannelMask)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to CapsuleLightFeatureProcessor::SetLightingChannelMask().");
 
            m_lightData.GetData<0>(handle.GetIndex()).m_lightingChannelMask = lightingChannelMask;
            m_deviceBufferNeedsUpdate = true;
        }

        void CapsuleLightFeatureProcessor::SetCapsuleData(LightHandle handle, const CapsuleLightData& data)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to CapsuleLightFeatureProcessor::SetCapsuleData().");

            m_lightData.GetData<0>(handle.GetIndex()) = data;
            UpdateBounds(handle);
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

        void CapsuleLightFeatureProcessor::UpdateBounds(LightHandle handle)
        {
            CapsuleLightData& capsuleData = m_lightData.GetData<0>(handle.GetIndex());

            AZ::Vector3 startPoint = AZ::Vector3::CreateFromFloat3(capsuleData.m_startPoint.data());
            AZ::Vector3 direction = AZ::Vector3::CreateFromFloat3(capsuleData.m_direction.data());
            AZ::Vector3 endPoint = startPoint + direction * capsuleData.m_length;
            float attenuationRadius = LightCommon::GetRadiusFromInvRadiusSquared(capsuleData.m_invAttenuationRadiusSquared);

            AZ::Capsule& bounds = m_lightData.GetData<1>(handle.GetIndex());
            bounds.SetFirstHemisphereCenter(startPoint);
            bounds.SetSecondHemisphereCenter(endPoint);
            bounds.SetRadius(attenuationRadius);
        }

    } // namespace Render
} // namespace AZ
