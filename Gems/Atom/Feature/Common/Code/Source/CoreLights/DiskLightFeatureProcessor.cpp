/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CoreLights/DiskLightFeatureProcessor.h>

#include <AzCore/Math/Color.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector3.h>

#include <Atom/Feature/CoreLights/CoreLightsConstants.h>
#include <Atom/Feature/Mesh/MeshFeatureProcessor.h>

#include <Atom/RHI/Factory.h>

#include <Atom/RPI.Public/ColorManagement/TransformColor.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>

#include <AzCore/std/containers/variant.h>

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
                    ->Version(1);
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
            desc.m_srgLayout = RPI::RPISystemInterface::Get()->GetViewSrgLayout().get();

            m_lightBufferHandler = GpuBufferHandler(desc);
            m_shadowFeatureProcessor = GetParentScene()->GetFeatureProcessor<ProjectedShadowFeatureProcessor>();

            MeshFeatureProcessor* meshFeatureProcessor = GetParentScene()->GetFeatureProcessor<MeshFeatureProcessor>();
            if (meshFeatureProcessor)
            {
                m_lightMeshFlag = meshFeatureProcessor->GetFlagRegistry()->AcquireTag(AZ::Name("o_enableDiskLights"));
                m_shadowMeshFlag = meshFeatureProcessor->GetFlagRegistry()->AcquireTag(AZ::Name("o_enableDiskLightShadows"));
            }
        }

        void DiskLightFeatureProcessor::Deactivate()
        {
            m_lightData.Clear();
            m_lightBufferHandler.Release();
        }

        DiskLightFeatureProcessor::LightHandle DiskLightFeatureProcessor::AcquireLight()
        {
            uint16_t id = m_lightData.GetFreeSlotIndex();

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
                ShadowId shadowId = ShadowId(m_lightData.GetData<0>(handle.GetIndex()).m_shadowIndex);
                if (shadowId.IsValid())
                {
                    m_shadowFeatureProcessor->ReleaseShadow(shadowId);
                }
                m_lightData.RemoveIndex(handle.GetIndex());
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
                // Get a reference to the new light
                DiskLightData& light = m_lightData.GetData<0>(handle.GetIndex());
                // Copy data from the source light on top of it.
                light = m_lightData.GetData<0>(sourceLightHandle.GetIndex());
                m_lightData.GetData<1>(handle.GetIndex()) = m_lightData.GetData<1>(sourceLightHandle.GetIndex());

                static_assert(AZStd::variant_detail::copy_assignable_traits<AZ::Frustum, AZ::Hemisphere, AZ::Sphere, AZ::Aabb> != AZStd::variant_detail::SpecialFunctionTraits::Unavailable);

                ShadowId shadowId = ShadowId(light.m_shadowIndex);
                if (shadowId.IsValid())
                {
                    // Since the source light has a valid shadow, a new shadow must be generated for the cloned light.
                    ProjectedShadowFeatureProcessorInterface::ProjectedShadowDescriptor originalDesc = m_shadowFeatureProcessor->GetShadowProperties(shadowId);
                    ShadowId cloneShadow = m_shadowFeatureProcessor->AcquireShadow();
                    light.m_shadowIndex = cloneShadow.GetIndex();
                    m_shadowFeatureProcessor->SetShadowProperties(cloneShadow, originalDesc);
                }

                m_deviceBufferNeedsUpdate = true;
            }
            return handle;
        }

        void DiskLightFeatureProcessor::Simulate(const FeatureProcessor::SimulatePacket& packet)
        {
            AZ_PROFILE_SCOPE(RPI, "DiskLightFeatureProcessor: Simulate");
            AZ_UNUSED(packet);

            if (m_deviceBufferNeedsUpdate)
            {
                m_lightBufferHandler.UpdateBuffer(m_lightData.GetDataVector<0>());
                m_deviceBufferNeedsUpdate = false;
            }

            if (r_enablePerMeshShaderOptionFlags)
            {
                // Helper lambdas
                auto indexHasShadow = [&](LightHandle::IndexType index) -> bool
                {
                    ShadowId shadowId = ShadowId(m_lightData.GetData<0>(index).m_shadowIndex);
                    return shadowId.IsValid();
                };

                // Filter lambdas
                auto hasShadow = [&](const MeshCommon::BoundsVariant& bounds) -> bool
                {
                    return indexHasShadow(m_lightData.GetIndexForData<1>(&bounds));
                };
                auto noShadow = [&](const MeshCommon::BoundsVariant& bounds) -> bool
                {
                    return !indexHasShadow(m_lightData.GetIndexForData<1>(&bounds));
                };

                // Mark meshes that have point lights without shadow using only the light flag.
                MeshCommon::MarkMeshesWithFlag(GetParentScene(), AZStd::span(m_lightData.GetDataVector<1>()), m_lightMeshFlag.GetIndex(), noShadow);

                // Mark meshes that have point lights with shadow using a combination of light and shadow flags.
                uint32_t lightAndShadow = m_lightMeshFlag.GetIndex() | m_shadowMeshFlag.GetIndex();
                MeshCommon::MarkMeshesWithFlag(GetParentScene(), AZStd::span(m_lightData.GetDataVector<1>()), lightAndShadow, hasShadow);
            }
        }

        void DiskLightFeatureProcessor::Render(const DiskLightFeatureProcessor::RenderPacket& packet)
        {
            AZ_PROFILE_SCOPE(RPI, "DiskLightFeatureProcessor: Simulate");

            for (const RPI::ViewPtr& view : packet.m_views)
            {
                m_lightBufferHandler.UpdateSrg(view->GetShaderResourceGroup().get());
            }
        }

        void DiskLightFeatureProcessor::SetRgbIntensity(LightHandle handle, const PhotometricColor<PhotometricUnitType>& lightRgbIntensity)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to DiskLightFeatureProcessor::SetRgbIntensity().");

            auto transformedColor = AZ::RPI::TransformColor(lightRgbIntensity, AZ::RPI::ColorSpaceId::LinearSRGB, AZ::RPI::ColorSpaceId::ACEScg);

            AZStd::array<float, 3>& rgbIntensity = m_lightData.GetData<0>(handle.GetIndex()).m_rgbIntensity;
            rgbIntensity[0] = transformedColor.GetR();
            rgbIntensity[1] = transformedColor.GetG();
            rgbIntensity[2] = transformedColor.GetB();

            m_deviceBufferNeedsUpdate = true;
        }

        void DiskLightFeatureProcessor::SetPosition(LightHandle handle, const AZ::Vector3& lightPosition)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to DiskLightFeatureProcessor::SetPosition().");

            AZStd::array<float, 3>& position = m_lightData.GetData<0>(handle.GetIndex()).m_position;
            lightPosition.StoreToFloat3(position.data());

            UpdateBounds(handle);
            UpdateShadow(handle);

            m_deviceBufferNeedsUpdate = true;
        }

        void DiskLightFeatureProcessor::SetDirection(LightHandle handle, const AZ::Vector3& lightDirection)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to DiskLightFeatureProcessor::SetDirection().");

            AZStd::array<float, 3>& direction = m_lightData.GetData<0>(handle.GetIndex()).m_direction;
            lightDirection.GetNormalized().StoreToFloat3(direction.data());

            UpdateBounds(handle);
            UpdateShadow(handle);

            m_deviceBufferNeedsUpdate = true;
        }

        void DiskLightFeatureProcessor::SetAttenuationRadius(LightHandle handle, float attenuationRadius)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to DiskLightFeatureProcessor::SetAttenuationRadius().");

            attenuationRadius = AZStd::max<float>(attenuationRadius, 0.001f); // prevent divide by zero.
            DiskLightData& light = m_lightData.GetData<0>(handle.GetIndex());
            light.m_invAttenuationRadiusSquared = 1.0f / (attenuationRadius * attenuationRadius);

            UpdateBounds(handle);

            // Update the shadow near far planes if necessary
            ShadowId shadowId = ShadowId(light.m_shadowIndex);
            if (shadowId.IsValid())
            {
                m_shadowFeatureProcessor->SetNearFarPlanes(ShadowId(light.m_shadowIndex),
                    light.m_bulbPositionOffset, attenuationRadius + light.m_bulbPositionOffset);
            }

            m_deviceBufferNeedsUpdate = true;

        }

        void DiskLightFeatureProcessor::SetDiskRadius(LightHandle handle, float radius)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to DiskLightFeatureProcessor::SetDiskRadius().");
            
            DiskLightData& light = m_lightData.GetData<0>(handle.GetIndex());
            light.m_diskRadius = radius;

            UpdateBulbPositionOffset(light);
            UpdateBounds(handle);
            UpdateShadow(handle);

            m_deviceBufferNeedsUpdate = true;
        }

        void DiskLightFeatureProcessor::SetConstrainToConeLight(LightHandle handle, bool useCone)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to DiskLightFeatureProcessor::SetDiskRadius().");

            uint32_t& flags = m_lightData.GetData<0>(handle.GetIndex()).m_flags;
            useCone ? flags |= DiskLightData::Flags::UseConeAngle : flags &= ~DiskLightData::Flags::UseConeAngle;
            UpdateShadow(handle);

            m_deviceBufferNeedsUpdate = true;
        }
        
        void DiskLightFeatureProcessor::SetConeAngles(LightHandle handle, float innerRadians, float outerRadians)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to DiskLightFeatureProcessor::SetConeAngles().");
            
            ValidateAndSetConeAngles(handle, innerRadians, outerRadians);
            UpdateShadow(handle);

            m_deviceBufferNeedsUpdate = true;
        }
        
        void DiskLightFeatureProcessor::ValidateAndSetConeAngles(LightHandle handle, float innerRadians, float outerRadians)
        {
            DiskLightData& light = m_lightData.GetData<0>(handle.GetIndex());

            // Assume if the cone angles are being set that the user wants to constrain to a cone angle
            SetConstrainToConeLight(handle, true);
            
            ShadowId shadowId = ShadowId(light.m_shadowIndex);
            float maxRadians = shadowId.IsNull() ? MaxConeRadians : MaxProjectedShadowRadians;
            float minRadians = 0.001f;

            outerRadians = AZStd::clamp(outerRadians, minRadians, maxRadians);
            innerRadians = AZStd::clamp(innerRadians, minRadians, outerRadians);

            light.m_cosInnerConeAngle = cosf(innerRadians);
            light.m_cosOuterConeAngle = cosf(outerRadians);
            
            UpdateBulbPositionOffset(light);
        }

        void DiskLightFeatureProcessor::SetDiskData(LightHandle handle, const DiskLightData& data)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to DiskLightFeatureProcessor::SetDiskData().");

            m_lightData.GetData<0>(handle.GetIndex()) = data;
            UpdateShadow(handle);
            UpdateBounds(handle);

            m_deviceBufferNeedsUpdate = true;
        }

        const DiskLightData&  DiskLightFeatureProcessor::GetDiskData(LightHandle handle) const
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to DiskLightFeatureProcessor::GetDiskData().");

            return m_lightData.GetData<0>(handle.GetIndex());
        }

        const Data::Instance<RPI::Buffer> DiskLightFeatureProcessor::GetLightBuffer()const
        {
            return m_lightBufferHandler.GetBuffer();
        }

        uint32_t DiskLightFeatureProcessor::GetLightCount() const
        {
            return m_lightBufferHandler.GetElementCount();
        }

        void DiskLightFeatureProcessor::SetShadowsEnabled(LightHandle handle, bool enabled)
        {
            DiskLightData& light = m_lightData.GetData<0>(handle.GetIndex());
            ShadowId shadowId = ShadowId(light.m_shadowIndex);
            if (shadowId.IsValid() && enabled == false)
            {
                // Disable shadows
                m_shadowFeatureProcessor->ReleaseShadow(shadowId);
                shadowId.Reset();
                light.m_shadowIndex = shadowId.GetIndex();
                m_deviceBufferNeedsUpdate = true;
            }
            else if(shadowId.IsNull() && enabled == true)
            {
                // Enable shadows
                light.m_shadowIndex = m_shadowFeatureProcessor->AcquireShadow().GetIndex();

                // It's possible the cone angles aren't set, or are too wide for casting shadows. This makes sure they're set to reasonable limits.
                // This function expects radians, so the cos stored in the actual data needs to be undone.
                ValidateAndSetConeAngles(handle, acosf(light.m_cosInnerConeAngle), acosf(light.m_cosOuterConeAngle));

                UpdateShadow(handle);
                m_deviceBufferNeedsUpdate = true;
            }
        }

        template <typename Functor, typename ParamType>
        void DiskLightFeatureProcessor::SetShadowSetting(LightHandle handle, Functor&& functor, ParamType&& param)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to DiskLightFeatureProcessor::SetShadowSetting().");
            
            DiskLightData& light = m_lightData.GetData<0>(handle.GetIndex());
            ShadowId shadowId = ShadowId(light.m_shadowIndex);

            AZ_Assert(shadowId.IsValid(), "Attempting to set a shadow property when shadows are not enabled.");
            if (shadowId.IsValid())
            {
                AZStd::invoke(AZStd::forward<Functor>(functor), m_shadowFeatureProcessor, shadowId, AZStd::forward<ParamType>(param));
            }
        }
        
        void DiskLightFeatureProcessor::SetShadowBias(LightHandle handle, float bias)
        {
            SetShadowSetting(handle, &ProjectedShadowFeatureProcessor::SetShadowBias, bias);
        }

        void DiskLightFeatureProcessor::SetNormalShadowBias(LightHandle handle, float bias)
        {
            SetShadowSetting(handle, &ProjectedShadowFeatureProcessor::SetNormalShadowBias, bias);
        }

        void DiskLightFeatureProcessor::SetShadowmapMaxResolution(LightHandle handle, ShadowmapSize shadowmapSize)
        {
            SetShadowSetting(handle, &ProjectedShadowFeatureProcessor::SetShadowmapMaxResolution, shadowmapSize);
        }
        
        void DiskLightFeatureProcessor::SetShadowFilterMethod(LightHandle handle, ShadowFilterMethod method)
        {
            SetShadowSetting(handle, &ProjectedShadowFeatureProcessor::SetShadowFilterMethod, method);
        }
        
        void DiskLightFeatureProcessor::SetFilteringSampleCount(LightHandle handle, uint16_t count)
        {
            SetShadowSetting(handle, &ProjectedShadowFeatureProcessor::SetFilteringSampleCount, count);
        }

        void DiskLightFeatureProcessor::SetEsmExponent(LightHandle handle, float exponent)
        {
            SetShadowSetting(handle, &ProjectedShadowFeatureProcessor::SetEsmExponent, exponent);
        }

        void DiskLightFeatureProcessor::SetAffectsGI(LightHandle handle, bool affectsGI)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to DiskLightFeatureProcessor::SetAffectsGI().");

            m_lightData.GetData<0>(handle.GetIndex()).m_affectsGI = affectsGI;
            m_deviceBufferNeedsUpdate = true;
        }

        void DiskLightFeatureProcessor::SetAffectsGIFactor(LightHandle handle, float affectsGIFactor)
        {
            AZ_Assert(handle.IsValid(), "Invalid LightHandle passed to DiskLightFeatureProcessor::SetAffectsGIFactor().");

            m_lightData.GetData<0>(handle.GetIndex()).m_affectsGIFactor = affectsGIFactor;
            m_deviceBufferNeedsUpdate = true;
        }

        void DiskLightFeatureProcessor::UpdateShadow(LightHandle handle)
        {
            const DiskLightData& diskLight = m_lightData.GetData<0>(handle.GetIndex());
            ShadowId shadowId = ShadowId(diskLight.m_shadowIndex);
            if (shadowId.IsNull())
            {
                // Early out if shadows are disabled.
                return;
            }

            ProjectedShadowFeatureProcessorInterface::ProjectedShadowDescriptor desc = m_shadowFeatureProcessor->GetShadowProperties(shadowId);

            Vector3 position = Vector3::CreateFromFloat3(diskLight.m_position.data());
            const Vector3 direction = Vector3::CreateFromFloat3(diskLight.m_direction.data());

            constexpr float SmallAngle = 0.01f;
            float halfFov = acosf(diskLight.m_cosOuterConeAngle);
            desc.m_fieldOfViewYRadians = GetMax(halfFov * 2.0f, SmallAngle);
            
            // To handle bulb radius, set the position of the shadow caster behind the actual light depending on the radius of the bulb
            //
            //   \         /
            //    \       /
            //     \_____/  <-- position of light itself (and forward plane of shadow casting view)
            //      .   .
            //       . .
            //        *     <-- position of shadow casting view
            //
            position += diskLight.m_bulbPositionOffset * -direction;
            desc.m_transform = Transform::CreateLookAt(position, position + direction);

            desc.m_aspectRatio = 1.0f;
            desc.m_nearPlaneDistance = diskLight.m_bulbPositionOffset;
            const float attenuationRadius = LightCommon::GetRadiusFromInvRadiusSquared(diskLight.m_invAttenuationRadiusSquared);
            desc.m_farPlaneDistance = attenuationRadius + diskLight.m_bulbPositionOffset;

            m_shadowFeatureProcessor->SetShadowProperties(shadowId, desc);
        }
        
        void DiskLightFeatureProcessor::UpdateBulbPositionOffset(DiskLightData& light)
        {
            // If we have the outer cone angle in radians, the offset is (radius * tan(pi/2 - coneRadians)). However
            // light stores the cosine of outerConeRadians, making the equation (radius * tan(pi/2 - acosf(cosConeRadians)).
            // This simplifies to the equation below.
            float cosConeRadians = light.m_cosOuterConeAngle;
            light.m_bulbPositionOffset = light.m_diskRadius * cosConeRadians / sqrt(1.0f - cosConeRadians * cosConeRadians);
        }

        void DiskLightFeatureProcessor::UpdateBounds(LightHandle handle)
        {
            DiskLightData data = m_lightData.GetData<0>(handle.GetIndex());

            float radius = LightCommon::GetRadiusFromInvRadiusSquared(data.m_invAttenuationRadiusSquared);
            AZ::Vector3 position = AZ::Vector3::CreateFromFloat3(data.m_position.data());
            AZ::Vector3 normal = AZ::Vector3::CreateFromFloat3(data.m_direction.data());

            // At greater than a 68 degree cone angle, a hemisphere will have a smaller volume than a frustum.
            constexpr float CosFrustumHemisphereVolumeCrossoverAngle = 0.37f;

            if (data.m_cosOuterConeAngle < CosFrustumHemisphereVolumeCrossoverAngle)
            {
                // Wide angle, use a hemisphere for bounds instead of frustum
                MeshCommon::BoundsVariant& bounds = m_lightData.GetData<1>(handle.GetIndex());
                bounds.emplace<Hemisphere>(Hemisphere(position, radius, normal));
            }
            else
            {
                ViewFrustumAttributes desc;
                desc.m_aspectRatio = 1.0f;

                desc.m_nearClip = data.m_bulbPositionOffset;
                desc.m_farClip = data.m_bulbPositionOffset + radius;
                desc.m_verticalFovRadians = GetMax(0.001f, acosf(data.m_cosOuterConeAngle) * 2.0f);
                desc.m_worldTransform = AZ::Transform::CreateLookAt(position, position + normal);

                m_lightData.GetData<1>(handle.GetIndex()) = AZ::Frustum(desc);
            }

        }

    } // namespace Render
} // namespace AZ
