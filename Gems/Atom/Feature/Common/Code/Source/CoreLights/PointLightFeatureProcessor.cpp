/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CoreLights/PointLightFeatureProcessor.h>

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Color.h>

#include <Atom/Feature/CoreLights/CoreLightsConstants.h>
#include <Atom/Feature/CoreLights/LightCommon.h>
#include <Atom/Feature/Mesh/MeshCommon.h>
#include <Atom/Feature/Mesh/MeshFeatureProcessor.h>

#include <Atom/RHI/Factory.h>

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
            // Note must match PointShadowDirections in PointLight.azsli
            m_pointShadowTransforms[0] = AZ::Transform::CreateLookAt(AZ::Vector3::CreateZero(), -AZ::Vector3::CreateAxisX());
            m_pointShadowTransforms[1] = AZ::Transform::CreateLookAt(AZ::Vector3::CreateZero(), AZ::Vector3::CreateAxisX());
            m_pointShadowTransforms[2] = AZ::Transform::CreateLookAt(AZ::Vector3::CreateZero(), -AZ::Vector3::CreateAxisY());
            m_pointShadowTransforms[3] = AZ::Transform::CreateLookAt(AZ::Vector3::CreateZero(), AZ::Vector3::CreateAxisY());
            m_pointShadowTransforms[4] = AZ::Transform::CreateLookAt(AZ::Vector3::CreateZero(), -AZ::Vector3::CreateAxisZ());
            m_pointShadowTransforms[5] = AZ::Transform::CreateLookAt(AZ::Vector3::CreateZero(), AZ::Vector3::CreateAxisZ());
        }

        void PointLightFeatureProcessor::Activate()
        {
            GpuBufferHandler::Descriptor desc;
            desc.m_bufferName = "PointLightBuffer";
            desc.m_bufferSrgName = "m_pointLights";
            desc.m_elementCountSrgName = "m_pointLightCount";
            desc.m_elementSize = sizeof(PointLightData);
            desc.m_srgLayout = RPI::RPISystemInterface::Get()->GetViewSrgLayout().get();
            m_shadowFeatureProcessor = GetParentScene()->GetFeatureProcessor<ProjectedShadowFeatureProcessor>();

            m_lightBufferHandler = GpuBufferHandler(desc);

            MeshFeatureProcessor* meshFeatureProcessor = GetParentScene()->GetFeatureProcessor<MeshFeatureProcessor>();
            if (meshFeatureProcessor)
            {
                m_lightMeshFlag = meshFeatureProcessor->GetFlagRegistry()->AcquireTag(AZ::Name("o_enableSphereLights"));
                m_shadowMeshFlag = meshFeatureProcessor->GetFlagRegistry()->AcquireTag(AZ::Name("o_enableSphereLightShadows"));
            }
        }

        void PointLightFeatureProcessor::Deactivate()
        {
            m_lightData.Clear();
            m_lightBufferHandler.Release();
        }

        PointLightFeatureProcessor::LightHandle PointLightFeatureProcessor::AcquireLight()
        {
            uint16_t id = m_lightData.GetFreeSlotIndex();

            if (id == MultiIndexedDataVector<PointLightData>::NoFreeSlot)
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
                for (int i = 0; i < PointLightData::NumShadowFaces; ++i)
                {
                    ShadowId shadowId = ShadowId(m_lightData.GetData<0>(handle.GetIndex()).m_shadowIndices[i]);
                    if (shadowId.IsValid())
                    {
                        m_shadowFeatureProcessor->ReleaseShadow(shadowId);
                    }
                }

                m_lightData.RemoveIndex(handle.GetIndex());
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
                m_lightData.GetData<0>(handle.GetIndex()) = m_lightData.GetData<0>(sourceLightHandle.GetIndex());
                m_lightData.GetData<1>(handle.GetIndex()) = m_lightData.GetData<1>(sourceLightHandle.GetIndex());
                m_deviceBufferNeedsUpdate = true;
            }
            return handle;
        }

        void PointLightFeatureProcessor::Simulate(const FeatureProcessor::SimulatePacket& packet)
        {
            AZ_PROFILE_SCOPE(RPI, "PointLightFeatureProcessor: Simulate");
            AZ_UNUSED(packet);

            if (m_deviceBufferNeedsUpdate)
            {
                m_lightBufferHandler.UpdateBuffer(m_lightData.GetDataVector<0>());
                m_deviceBufferNeedsUpdate = false;
            }

            if (r_enablePerMeshShaderOptionFlags)
            {
                auto hasShadow = [&](const AZ::Sphere& sphere) -> bool
                {
                    LightHandle::IndexType index = m_lightData.GetIndexForData<1>(&sphere);
                    ShadowId shadowId = ShadowId(m_lightData.GetData<0>(index).m_shadowIndices[0]);
                    return shadowId.IsValid();
                };
                auto noShadow = [&](const AZ::Sphere& sphere) -> bool
                {
                    return !hasShadow(sphere);
                };

                // Mark meshes that have point lights without shadow using only the light flag.
                MeshCommon::MarkMeshesWithFlag(GetParentScene(), AZStd::span(m_lightData.GetDataVector<1>()), m_lightMeshFlag.GetIndex(), noShadow);

                // Mark meshes that have point lights with shadow using a combination of light and shadow flags.
                uint32_t lightAndShadow = m_lightMeshFlag.GetIndex() | m_shadowMeshFlag.GetIndex();
                MeshCommon::MarkMeshesWithFlag(GetParentScene(), AZStd::span(m_lightData.GetDataVector<1>()), lightAndShadow, hasShadow);
            }
        }

        void PointLightFeatureProcessor::Render(const PointLightFeatureProcessor::RenderPacket& packet)
        {
            AZ_PROFILE_SCOPE(RPI, "PointLightFeatureProcessor: Render");

            for (const RPI::ViewPtr& view : packet.m_views)
            {
                m_lightBufferHandler.UpdateSrg(view->GetShaderResourceGroup().get());
            }
        }

        void PointLightFeatureProcessor::SetRgbIntensity(LightHandle handle, const PhotometricColor<PhotometricUnitType>& lightRgbIntensity)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to PointLightFeatureProcessor::SetRgbIntensity().");

            auto transformedColor = AZ::RPI::TransformColor(lightRgbIntensity, AZ::RPI::ColorSpaceId::LinearSRGB, AZ::RPI::ColorSpaceId::ACEScg);

            AZStd::array<float, 3>& rgbIntensity = m_lightData.GetData<0>(handle.GetIndex()).m_rgbIntensity;
            rgbIntensity[0] = transformedColor.GetR();
            rgbIntensity[1] = transformedColor.GetG();
            rgbIntensity[2] = transformedColor.GetB();

            m_deviceBufferNeedsUpdate = true;
        }

        void PointLightFeatureProcessor::SetPosition(LightHandle handle, const AZ::Vector3& lightPosition)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to PointLightFeatureProcessor::SetPosition().");

            AZStd::array<float, 3>& position = m_lightData.GetData<0>(handle.GetIndex()).m_position;
            lightPosition.StoreToFloat3(position.data());

            m_lightData.GetData<1>(handle.GetIndex()).SetCenter(lightPosition);

            m_deviceBufferNeedsUpdate = true;
            UpdateShadow(handle);
        }

        void PointLightFeatureProcessor::SetAttenuationRadius(LightHandle handle, float attenuationRadius)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to PointLightFeatureProcessor::SetAttenuationRadius().");

            attenuationRadius = AZStd::max<float>(attenuationRadius, 0.001f); // prevent divide by zero.
            m_lightData.GetData<0>(handle.GetIndex()).m_invAttenuationRadiusSquared = 1.0f / (attenuationRadius * attenuationRadius);
            m_lightData.GetData<1>(handle.GetIndex()).SetRadius(attenuationRadius);

            m_deviceBufferNeedsUpdate = true;
        }

        void PointLightFeatureProcessor::SetBulbRadius(LightHandle handle, float bulbRadius)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to PointLightFeatureProcessor::SetBulbRadius().");

            m_lightData.GetData<0>(handle.GetIndex()).m_bulbRadius = bulbRadius;
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

        void PointLightFeatureProcessor::SetShadowsEnabled(LightHandle handle, bool enabled)
        {
            auto& light = m_lightData.GetData<0>(handle.GetIndex());
            for (int i = 0; i < PointLightData::NumShadowFaces; ++i)
            {
                ShadowId shadowId = ShadowId(light.m_shadowIndices[i]);
                if (shadowId.IsValid() && !enabled)
                {
                    // Disable shadows
                    m_shadowFeatureProcessor->ReleaseShadow(shadowId);
                    shadowId.Reset();
                    light.m_shadowIndices[i] = shadowId.GetIndex();
                    m_deviceBufferNeedsUpdate = true;
                }
                else if (shadowId.IsNull() && enabled)
                {
                    // Enable shadows
                    light.m_shadowIndices[i] = m_shadowFeatureProcessor->AcquireShadow().GetIndex();

                    UpdateShadow(handle);
                    m_deviceBufferNeedsUpdate = true;
                }
            }
        }

        void PointLightFeatureProcessor::SetPointData(LightHandle handle, const PointLightData& data)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to PointLightFeatureProcessor::SetPointData().");

            m_lightData.GetData<0>(handle.GetIndex()) = data;

            AZ::Vector3 position = AZ::Vector3::CreateFromFloat3(data.m_position.data());
            float radius = LightCommon::GetRadiusFromInvRadiusSquared(data.m_invAttenuationRadiusSquared);
            m_lightData.GetData<1>(handle.GetIndex()).Set(AZ::Sphere(position, radius));
            m_deviceBufferNeedsUpdate = true;
            UpdateShadow(handle);
        }

        void PointLightFeatureProcessor::UpdateShadow(LightHandle handle)
        {
            constexpr float SqrtHalf = 0.707106781187f; // sqrt(0.5);

            const auto& pointLight = m_lightData.GetData<0>(handle.GetIndex());
            for (int i = 0; i < PointLightData::NumShadowFaces; ++i)
            {
                ShadowId shadowId = ShadowId(pointLight.m_shadowIndices[i]);
                if (shadowId.IsNull())
                {
                    // Early out if shadows are disabled.
                    return;
                }

                ProjectedShadowFeatureProcessorInterface::ProjectedShadowDescriptor desc = m_shadowFeatureProcessor->GetShadowProperties(shadowId);
                // Make it slightly larger than 90 degrees to avoid artifacts on the boundary between 2 cubemap faces
                desc.m_fieldOfViewYRadians = DegToRad(91.0f); 
                desc.m_transform = m_pointShadowTransforms[i];
                desc.m_transform.SetTranslation(pointLight.m_position[0], pointLight.m_position[1], pointLight.m_position[2]);
                desc.m_aspectRatio = 1.0f;
                desc.m_nearPlaneDistance = SqrtHalf * pointLight.m_bulbRadius;

                const float invRadiusSquared = pointLight.m_invAttenuationRadiusSquared;
                if (invRadiusSquared <= 0.f)
                {
                    AZ_Assert(false, "Attenuation radius must be set before using the light.");
                    return;
                }
                const float attenuationRadius = sqrtf(1.f / invRadiusSquared);
                desc.m_farPlaneDistance = attenuationRadius + pointLight.m_bulbRadius;

                m_shadowFeatureProcessor->SetShadowProperties(shadowId, desc);
            }
        }

        template<typename Functor, typename ParamType>
        void PointLightFeatureProcessor::SetShadowSetting(LightHandle handle, Functor&& functor, ParamType&& param)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to PointLightFeatureProcessor::SetShadowSetting().");

            auto& light = m_lightData.GetData<0>(handle.GetIndex());
            for (int lightIndex = 0; lightIndex < PointLightData::NumShadowFaces; ++lightIndex)
            {
                ShadowId shadowId = ShadowId(light.m_shadowIndices[lightIndex]);

                AZ_Assert(shadowId.IsValid(), "Attempting to set a shadow property when shadows are not enabled.");
                if (shadowId.IsValid())
                {
                    AZStd::invoke(AZStd::forward<Functor>(functor), m_shadowFeatureProcessor, shadowId, AZStd::forward<ParamType>(param));
                }
            }
        }
        
        void PointLightFeatureProcessor::SetShadowBias(LightHandle handle, float bias)
        {
            SetShadowSetting(handle, &ProjectedShadowFeatureProcessor::SetShadowBias, bias);
        }

        void PointLightFeatureProcessor::SetAffectsGI(LightHandle handle, bool affectsGI)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to PointLightFeatureProcessor::SetAffectsGI().");

            m_lightData.GetData<0>(handle.GetIndex()).m_affectsGI = affectsGI;
            m_deviceBufferNeedsUpdate = true;
        }

        void PointLightFeatureProcessor::SetAffectsGIFactor(LightHandle handle, float affectsGIFactor)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to PointLightFeatureProcessor::SetAffectsGIFactor().");

            m_lightData.GetData<0>(handle.GetIndex()).m_affectsGIFactor = affectsGIFactor;
            m_deviceBufferNeedsUpdate = true;
        }

        void PointLightFeatureProcessor::SetShadowmapMaxResolution(LightHandle handle, ShadowmapSize shadowmapSize)
        {
            SetShadowSetting(handle, &ProjectedShadowFeatureProcessor::SetShadowmapMaxResolution, shadowmapSize);
        }

        void PointLightFeatureProcessor::SetShadowFilterMethod(LightHandle handle, ShadowFilterMethod method)
        {
            SetShadowSetting(handle, &ProjectedShadowFeatureProcessor::SetShadowFilterMethod, method);
        }

        void PointLightFeatureProcessor::SetFilteringSampleCount(LightHandle handle, uint16_t count)
        {
            SetShadowSetting(handle, &ProjectedShadowFeatureProcessor::SetFilteringSampleCount, count);
        }

        void PointLightFeatureProcessor::SetEsmExponent(LightHandle handle, float esmExponent)
        {
            SetShadowSetting(handle, &ProjectedShadowFeatureProcessor::SetEsmExponent, esmExponent);
        }

        void PointLightFeatureProcessor::SetNormalShadowBias(LightHandle handle, float bias)
        {
            SetShadowSetting(handle, &ProjectedShadowFeatureProcessor::SetNormalShadowBias, bias);
        }

    } // namespace Render
} // namespace AZ
