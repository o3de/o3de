/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Color.h>
#include <Atom/Feature/CoreLights/PhotometricValue.h>
#include <Atom/Feature/CoreLights/ShadowConstants.h>

namespace AZ
{
    namespace Render
    {
        class DirectionalLightRequests
            : public ComponentBus
        {
        public:
            //! Gets a directional light's color. This value is independent from its intensity.
            //! @return directional light's color
            virtual const Color& GetColor() const = 0;

            //! Sets a directional light's color. This value is independent from its intensity.
            //! @param color directional light's color
            virtual void SetColor(const Color& color) = 0;

            //! Gets a directional light's intensity. This value is independent from its color.
            //! @return directional light's intensity
            virtual float GetIntensity() const = 0;

            //! Sets a directional light's intensity. This value is independent from its color.
            //! @param intensity directional light's intensity
            //! @param type The photometric unit of the intensity.
            virtual void SetIntensity(float intensity, PhotometricUnit unit) = 0;

            //! Sets a directional light's intensity. This value is independent from its color.
            //! Assumes no change in the current photometric unit of the intensity.
            //! @param intensity directional light's intensity
            virtual void SetIntensity(float intensity) = 0;

            //! Gets a directional light's angular diameter.
            //! @return directional light's angular diameter in degrees.
            virtual float GetAngularDiameter() const = 0;

            //! Sets a directional light's angular diameter. This value should be small, for instance the sun is 0.5 degrees across.
            //! @param angularDiameter Directional light's angular diameter in degrees.
            virtual void SetAngularDiameter(float angularDiameter) = 0;

            //! This gets shadowmap size (width/height).
            //! @return shadowmap size.
            virtual ShadowmapSize GetShadowmapSize() const = 0;

            //! This specifies the size of shadowmaps (for each cascade) to size x size.
            virtual void SetShadowmapSize(ShadowmapSize size) = 0;

            //! This gets cascade count of the shadowmap.
            //! @return cascade count
            virtual uint32_t GetCascadeCount() const = 0;

            //! This sets cascade count of the shadowmap
            //! @param cascadeCount cascade count
            virtual void SetCascadeCount(uint32_t cascadeCount) = 0;

            //! This gets ratio between logarithm/uniform scheme to split view frustum.
            //! @return the ratio (in [0,1]) between uniform scheme and logarithm scheme for splitting the view frustum into cascades.
            //!   ratio==0 means uniform and ratio==1 means logarithm.
            //!   uniform: the most detailed cascade covers wider area but less detailed.
            //!   logarithm: the most detailed cascade covers narrower area but more detailed.
            //!   The least detailed cascade is not affected by this parameter.
            virtual float GetShadowmapFrustumSplitSchemeRatio() const = 0;

            //! This sets ratio between logarithm/uniform scheme to split view frustum.
            //! If this is called, frustum splitting becomes automatic 
            //! and the far depths given by SetCascadeFarDepth() is discarded.
            //! @param ratio the ratio (in [0,1])
            virtual void SetShadowmapFrustumSplitSchemeRatio(float ratio) = 0;

            //! This gets the far depth of the cascade.
            //! @return the far depth for each cascade.
            virtual const Vector4& GetCascadeFarDepth() = 0;

            //! This sets the far depth of the cascade.
            //! If this is called, the ratio of frustum split scheme will be ignored.
            //! @param farDepth the far depth to be set.
            virtual void SetCascadeFarDepth(const Vector4& farDepth) = 0;

            //! This gets the flag split of shadowmap frustum is automatic.
            //! @return it is automatic if true, and it is manual if false.
            virtual bool GetShadowmapFrustumSplitAutomatic() const = 0;

            //! This sets the flag split of shadowmap frustum is automatic.
            //! @param isAutomatic the flag to be set.
            virtual void SetShadowmapFrustumSplitAutomatic(bool isAutomatic) = 0;

            //! This gets the entity ID of the camera used for specifying view frustum to create shadowmaps.
            //! @return camera entity ID.
            virtual EntityId GetCameraEntityId() const = 0;

            //! This sets the entity ID of the camera used for specifying view frustum to create shadowmaps.
            //! @param entityId camera entity ID.
            virtual void SetCameraEntityId(EntityId entityId) = 0;

            //! This gets shadow specific far clip distance.
            //! Pixels further than this value won't have shadows
            //! Smaller values result in higher quality shadows
            //! @return far clip distance
            virtual float GetShadowFarClipDistance() const = 0;

            //! This sets shadow specific far clip distance.
            //! @param farDist the far clip distance for each cascade.
            virtual void SetShadowFarClipDistance(float farDist) = 0;

            //! This gets the height of the ground.
            //! The position of view frustum is corrected using ground height
            //! to get better quality of shadow around the area close to the camera.
            //! To enable the correction, SetViewFrustumCorrectionEnabled(true) is required.
            //! @return ground height
            virtual float GetGroundHeight() const = 0;

            //! This specifies the height of the ground.
            //! @param groundHeight height of the ground
            virtual void SetGroundHeight(float groundHeight) = 0;

            //! This gets the flag whether view frustum correction is enabled not not.
            //! The calculation of it is caused when position or configuration of the camera is changed.
            //! @return flag of view frustum correction is enbled or not
            virtual bool GetViewFrustumCorrectionEnabled() const = 0;

            //! This specifies whether view frustum correction is enabled or not.
            //! @param enabled flag specifying whether view frustum positions are corrected.
            virtual void SetViewFrustumCorrectionEnabled(bool enabled) = 0;

            //! This gets the flag whether debug coloring is enabled or not.
            //! By debug coloring, we can see how cascading of shadowmaps works.
            //! @return the flag whether debug coloring is enabled or not.
            virtual bool GetDebugColoringEnabled() const = 0;

            //! This specifies whether debug coloring is enabled or not.
            //! @param enabled flag specifying whether debug coloring is added.
            virtual void SetDebugColoringEnabled(bool enabled) = 0;

            //! This gets the filter method of shadows.
            //! @return filter method 
            virtual ShadowFilterMethod GetShadowFilterMethod() const = 0;

            //! This specifies filter method of shadows.
            //! @param method filter method.
            virtual void SetShadowFilterMethod(ShadowFilterMethod method) = 0;

            //! This gets the sample count for filtering of the shadow boundary.
            //! @return Sample Count for filtering (up to 64)
            virtual uint32_t GetFilteringSampleCount() const = 0;

            //! This sets the sample count for filtering of the shadow boundary.
            //! @param count Sample Count for filtering (up to 64)
            virtual void SetFilteringSampleCount(uint32_t count) = 0;

            //! Gets whether the directional shadowmap should use receiver plane bias.
            //! This attempts to reduce shadow acne when using large pcf filters.
            virtual bool GetShadowReceiverPlaneBiasEnabled() const = 0;

            //! Sets whether the directional shadowmap should use receiver plane bias.
            //! @param enable flag specifying whether to enable the receiver plane bias feature
            virtual void SetShadowReceiverPlaneBiasEnabled(bool enable) = 0;

            //! Shadow bias reduces acne by applying a small amount of offset along shadow-space z.
            //! @return Returns the amount of bias to apply.
            virtual float GetShadowBias() const = 0;

            //! Shadow bias reduces acne by applying a small amount of offset along shadow-space z.
            //! @param Sets the amount of bias to apply.
            virtual void SetShadowBias(float bias) = 0;

            //! Reduces acne by biasing the shadowmap lookup along the geometric normal.
            //! @return Returns the amount of bias to apply.
            virtual float GetNormalShadowBias() const = 0;

            //! Reduces acne by biasing the shadowmap lookup along the geometric normal.
            //! @param normalShadowBias Sets the amount of normal shadow bias to apply.
            virtual void SetNormalShadowBias(float normalShadowBias) = 0;
        };
        using DirectionalLightRequestBus = EBus<DirectionalLightRequests>;

    } // namespace Render
} // namespace AZ
