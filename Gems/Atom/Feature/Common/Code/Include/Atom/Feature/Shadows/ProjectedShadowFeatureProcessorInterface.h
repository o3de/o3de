/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/FeatureProcessor.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Transform.h>
#include <Atom/Feature/CoreLights/ShadowConstants.h>

namespace AZ::Render
{
    //! This feature processor handles projected shadows for various lights.
    class ProjectedShadowFeatureProcessorInterface
        : public RPI::FeatureProcessor
    {
    public:

        AZ_RTTI(AZ::Render::ProjectedShadowFeatureProcessorInterface, "{C5651D73-3448-4D76-91C0-0E636A197F63}", AZ::RPI::FeatureProcessor);

        using ShadowId = RHI::Handle<uint16_t, ProjectedShadowFeatureProcessorInterface>;
        static constexpr float MaxProjectedShadowRadians = AZ::DegToRad(150.0f);

        //! Used in SetShadowProperties() to set several related shadow properties in one function call.
        struct ProjectedShadowDescriptor
        {
            Transform m_transform = Transform::CreateIdentity();
            float m_nearPlaneDistance = 0.01f;
            float m_farPlaneDistance = 10000.0f;
            float m_aspectRatio = 1.0f;
            float m_fieldOfViewYRadians = DegToRad(90.0f);
        };

        //! Creates a new projected shadow and returns a handle that can be used to reference it later.
        virtual ShadowId AcquireShadow() = 0;
        //! Releases a projected shadow given its ID.
        virtual void ReleaseShadow(ShadowId id) = 0;
        //! Sets the world space transform of where the shadow is cast from
        virtual void SetShadowTransform(ShadowId id, Transform transform) = 0;
        //! Sets the near and far plane distances for the shadow.
        virtual void SetNearFarPlanes(ShadowId id, float nearPlaneDistance, float farPlaneDistance) = 0;
        //! Sets the aspect ratio for the shadow.
        virtual void SetAspectRatio(ShadowId id, float aspectRatio) = 0;
        //! Sets the field of view for the shadow in radians in the Y direction.
        virtual void SetFieldOfViewY(ShadowId id, float fieldOfView) = 0;
        //! Sets the maximum resolution of the shadow map.
        virtual void SetShadowmapMaxResolution(ShadowId id, ShadowmapSize size) = 0;
        //! Sets the shadow bias.
        virtual void SetShadowBias(ShadowId id, float bias) = 0;
        //! Sets the normal shadow bias.
        virtual void SetNormalShadowBias(ShadowId id, float normalShadowBias) = 0;
        //! Sets the shadow filter method.
        virtual void SetShadowFilterMethod(ShadowId id, ShadowFilterMethod method) = 0;
        //! Sets the sample count for filtering of the shadow boundary, max 64.
        virtual void SetFilteringSampleCount(ShadowId id, uint16_t count) = 0;
        //! Sets all of the shadow properites in one call
        virtual void SetShadowProperties(ShadowId id, const ProjectedShadowDescriptor& descriptor) = 0;
        //! Gets the current shadow properties. Useful for updating several properties at once in SetShadowProperties() without having to set every property.
        virtual const ProjectedShadowDescriptor& GetShadowProperties(ShadowId id) = 0;
    };
}
