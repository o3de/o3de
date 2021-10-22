/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/Feature/CoreLights/PhotometricValue.h>
#include <Atom/Feature/CoreLights/ShadowConstants.h>
#include <Atom/RPI.Public/FeatureProcessor.h>
#include <AzCore/Math/Color.h>
#include <AzFramework/Components/CameraBus.h>

namespace AZ
{
    namespace Render
    {
        //! DirectionalLightFeatureProcessorInterface provides an interface
        //! to acquire, release, and update a directional light.
        //! This is necessary for code outside of the Atom features gem
        //! to communicate with the DirectionalLightFeatureProcessor.
        class DirectionalLightFeatureProcessorInterface
            : public RPI::FeatureProcessor
        {
        public:
            AZ_RTTI(AZ::Render::DirectionalLightFeatureProcessorInterface, "82C274F8-C635-4725-9ECB-0D7FA0DC0C6C", AZ::RPI::FeatureProcessor);

            using LightHandle = RHI::Handle<uint16_t, class DirectionalLight>;

            enum DebugDrawFlags : uint32_t // unscoped for easier bit flag combinations.
            {
                DebugDrawNone          = 0b000, // No debug drawing
                DebugDrawColoring      = 0b001, // Draw different colors for the various cascades
                DebugDrawBoundingBoxes = 0b010, // Draw bounding boxes for the cascades

                DebugDrawAll = ~DebugDrawNone,  // Draw all debug features
            };

            //! This creates a new directional light which can be referenced by the returned LightHandle.
            //! This must be released via ReleaseLight() when no longer needed.
            //! @return the handle of the new directional light.
            virtual LightHandle AcquireLight() = 0;

            //! This releases a LightHandle which removes the directional light.
            //! @param lightHandle the handle to release.
            //! @return true if it is released.
            virtual bool ReleaseLight(LightHandle& lightHandle) = 0;

            //! This creates a new LightHandle by copying data from an existing LightHandle.
            //! @param lightHandle the handle to clone.
            //! @return the handle of the new cloned light.
            virtual LightHandle CloneLight(LightHandle lightHandle) = 0;

            ////////// light specific

            //! This sets the intensity in RGB lux for a given LightHandle.
            //! @param handle the light handle.
            //! @param lightColor the intensity in RGB lux.
            virtual void SetRgbIntensity(LightHandle handle, const PhotometricColor<PhotometricUnit::Lux>& lightColor) = 0;

            //! This sets the direction of the light. direction should be normalized.
            //! @param handle the light handle.
            //! @return direction the direction of the light.
            virtual void SetDirection(LightHandle handle, const Vector3& direction) = 0;

            //! This sets a directional light's angular diameter. This value should be small, for instance the sun is 0.5 degrees across.
            //! @param handle the light handle.
            //! @param angularDiameter Directional light's angular diameter in degrees.
            virtual void SetAngularDiameter(LightHandle handle, float angularDiameter) = 0;

            ////////// shadow specific

            //! This sets the shadowmap size (width and height) of the light.
            //! @param handle the light handle.
            //! @param size the shadowmap size (width/height).
            virtual void SetShadowmapSize(LightHandle handle, ShadowmapSize size) = 0;

            //! This sets cascade count of the shadowmap.
            //! @param handle the light handle.
            //! @param cascadeCount the count of cascade (from 1 to 4).
            virtual void SetCascadeCount(LightHandle handle, uint16_t cascadeCount) = 0;

            //! This sets ratio between logarithm/uniform scheme to split view frustum.
            //! If this is called, frustum splitting becomes automatic 
            //! and the far depths given by SetCascadeFarDepth() are discarded.
            //! @param handle the light handle.
            //! @param ratio the ratio (in [0,1]) between logarithm scheme and uniform scheme to split view frustum into segments.
            //!   ratio==0 means uniform and ratio==1 means logarithm.
            //!   uniform: the most detailed cascade covers wider area but less detailed.
            //!   logarithm: the most detailed cascade covers narrower area but more detailed.
            //!   The least detailed cascade is not affected by this parameter.
            virtual void SetShadowmapFrustumSplitSchemeRatio(LightHandle handle, float ratio) = 0;

            //! This sets the far depth of the cascade.  If this is called,
            //! the ratio of frustum split scheme will be ignored.
            //! @param handle the light handle.
            //! @param cascadeIndex the index of the cascade to be changed.
            //! @param farDepth the far depth to be set.
            virtual void SetCascadeFarDepth(LightHandle handle, uint16_t cascadeIndex, float farDepth) = 0;

            //! This sets camera configuration which affect to cascade segmentation.
            //! @param handle the light handle.
            //! @param cameraConfiguration the camera configuration.
            virtual void SetCameraConfiguration(
                LightHandle handle,
                const Camera::Configuration& cameraConfiguration,
                const RPI::RenderPipelineId& renderPipelineId = RPI::RenderPipelineId()) = 0;

            //! This sets shadow specific far clip depth.
            //! Pixels beyond the far clip depth do not receive shadows.
            //! Reducing this value improves shadow quality.
            //! @param handle the light handle.
            //! @param farDist the far clip depth.
            virtual void SetShadowFarClipDistance(LightHandle handle, float farDist) = 0;

            //! This sets camera transform which affect to cascade segmentation.
            //! @param handle the light handle.
            //! @return cameraTransform the camera transform.
            virtual void SetCameraTransform(
                LightHandle handle,
                const Transform& cameraTransform,
                const RPI::RenderPipelineId& renderPipelineId = RPI::RenderPipelineId()) = 0;

            //! This specifies the height of the ground.
            //! @param handle the light handle.
            //! @param groundHeight height of the ground.
            //! The position of view frustum is corrected using groundHeight
            //! to get better quality of shadow around the area close to the camera.
            //! To enable the correction, SetViewFrustumCorrectionEnabled(true) is required.
            virtual void SetGroundHeight(LightHandle handle, float groundHeight) = 0;

            //! This specifies whether view frustum correction is enabled or not.
            //! @param handle the light handle.
            //! @param enabled flag specifying whether view frustum positions are corrected.
            //! The calculation of it is caused when position or configuration of the camera is changed.
            virtual void SetViewFrustumCorrectionEnabled(LightHandle, bool enabled) = 0;

            //! This specifies what debug features to display.
            //! @param handle the light handle.
            //! @param flags flags specifying the debug features. See DebugDrawFlags for more info.
            //! By drawing debug colors and bounding boxes, we can see how cascading of shadowmaps works.
            virtual void SetDebugFlags(LightHandle handle, DebugDrawFlags flags) = 0;

            //! This specifies filter method of shadows.
            //! @param handle the light handle.
            //! @param method filter method.
            virtual void SetShadowFilterMethod(LightHandle handle, ShadowFilterMethod method) = 0;

            //! This sets sample count for filtering of shadow boundary.
            //! @param handle the light handle.
            //! @param count Sample Count for filtering (up to 64)
            virtual void SetFilteringSampleCount(LightHandle handle, uint16_t count) = 0;

            //! Sets whether the directional shadowmap should use receiver plane bias.
            //! This attempts to reduce shadow acne when using large pcf filters.
            virtual void SetShadowReceiverPlaneBiasEnabled(LightHandle handle, bool enable) = 0;

            //! Reduces acne by applying a small amount of bias along shadow-space z.
            virtual void SetShadowBias(LightHandle handle, float bias) = 0;

            //! Reduces acne by biasing the shadowmap lookup along the geometric normal.
            virtual void SetNormalShadowBias(LightHandle handle, float normalShadowBias) = 0;
        };
    } // namespace Render
} // namespace AZ
