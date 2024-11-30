/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/Mesh/MeshCommon.h>
#include <CoreLights/LightCommon.h>
#include <Shadows/ProjectedShadowFeatureProcessor.h>

namespace AZ
{
    namespace Render
    {
        //! Utility functions for some common calculations for SpotLights
        namespace SpotLightUtils
        {
            //! Max angle for the cone of a spotlight when not generating shadows
            static constexpr float MaxConeRadians = AZ::DegToRad(90.0f);
            //! Max angle for the cone of a spotlight when generating shadows
            static constexpr float MaxProjectedShadowRadians = ProjectedShadowFeatureProcessorInterface::MaxProjectedShadowRadians * 0.5f;
            using ShadowId = ProjectedShadowFeatureProcessor::ShadowId;

            // Classes used for handling getting a member that may not exist.
            struct general_ {};
            struct special_ : general_{};
            template<typename> struct int_ { typedef int type; };

            //! Returns the bulb position offset for light data that has the m_bulbPositionOffset member
            template<class LightData, typename int_<decltype(LightData::m_bulbPositionOffset)>::type = 0>
            float GetBulbPositionOffset(const LightData& light, special_)
            {
                return light.m_bulbPositionOffset;
            }

            //! Returns 0 as the bulb position offset because the light data doesn't contain a m_bulbPositionOffset member
            template<class LightData>
            float GetBulbPositionOffset([[maybe_unused]] const LightData& light, general_)
            {
                return 0.0f;
            }

            //! Creates the bounds for a spot light
            template<class LightData>
            MeshCommon::BoundsVariant BuildBounds(const LightData& data)
            {
                float radius = LightCommon::GetRadiusFromInvRadiusSquared(data.m_invAttenuationRadiusSquared);
                AZ::Vector3 position = AZ::Vector3::CreateFromFloat3(data.m_position.data());
                AZ::Vector3 normal = AZ::Vector3::CreateFromFloat3(data.m_direction.data());

                // At greater than a 68 degree cone angle, a hemisphere will have a smaller volume than a frustum.
                constexpr float CosFrustumHemisphereVolumeCrossoverAngle = 0.37f;

                if (data.m_cosOuterConeAngle < CosFrustumHemisphereVolumeCrossoverAngle)
                {
                    // Wide angle, use a hemisphere for bounds instead of frustum
                    return MeshCommon::BoundsVariant(Hemisphere(position, radius, normal));
                }
                else
                {
                    float bulbPositionOffset = GetBulbPositionOffset(data, special_());
                    ViewFrustumAttributes desc;
                    desc.m_aspectRatio = 1.0f;

                    desc.m_nearClip = bulbPositionOffset;
                    desc.m_farClip = bulbPositionOffset + radius;
                    desc.m_verticalFovRadians = GetMax(0.001f, acosf(data.m_cosOuterConeAngle) * 2.0f);
                    desc.m_worldTransform = AZ::Transform::CreateLookAt(position, position + normal);

                    return MeshCommon::BoundsVariant(AZ::Frustum(desc));
                }
            }

            //! Clamps and update the inner and outer cone angles of a spotlight
            template<class LightData>
            void ValidateAndSetConeAngles(LightData& light, float innerRadians, float outerRadians)
            {
                ShadowId shadowId = ShadowId(light.m_shadowIndex);
                float maxRadians = shadowId.IsNull() ? MaxConeRadians : MaxProjectedShadowRadians;
                float minRadians = 0.001f;

                outerRadians = AZStd::clamp(outerRadians, minRadians, maxRadians);
                innerRadians = AZStd::clamp(innerRadians, minRadians, outerRadians);

                light.m_cosInnerConeAngle = cosf(innerRadians);
                light.m_cosOuterConeAngle = cosf(outerRadians);
            }

            //! Updates a shadow descriptor for a spotlight according to it's position, direction, angle, etc 
            template<class LightData>
            void UpdateShadowDescriptor(const LightData& light, ProjectedShadowFeatureProcessorInterface::ProjectedShadowDescriptor& desc)
            {
                Vector3 position = Vector3::CreateFromFloat3(light.m_position.data());
                const Vector3 direction = Vector3::CreateFromFloat3(light.m_direction.data());

                constexpr float SmallAngle = 0.01f;
                float halfFov = acosf(light.m_cosOuterConeAngle);
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
                float bulbPositionOffset = GetBulbPositionOffset(light, special_());
                position += bulbPositionOffset * -direction;
                desc.m_transform = Transform::CreateLookAt(position, position + direction);

                desc.m_aspectRatio = 1.0f;
                desc.m_nearPlaneDistance = bulbPositionOffset;
                const float attenuationRadius = LightCommon::GetRadiusFromInvRadiusSquared(light.m_invAttenuationRadiusSquared);
                desc.m_farPlaneDistance = attenuationRadius + bulbPositionOffset;
            }
        } // namespace SpotLightUtils
    } // namespace Render
} // namespace AZ
