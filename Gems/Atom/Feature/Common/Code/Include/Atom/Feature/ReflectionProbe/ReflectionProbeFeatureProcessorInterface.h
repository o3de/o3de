/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/Math/Aabb.h>
#include <Atom/Utils/StableDynamicArray.h>
#include <Atom/RPI.Public/FeatureProcessor.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>
#include <Atom/RPI.Reflect/Image/Image.h>

namespace AZ
{
    namespace Render
    {
        class ReflectionProbe;

        using ReflectionProbeHandle = AZ::Uuid;
        using ReflectionProbeHandleVector = AZStd::vector<ReflectionProbeHandle>;
        using BuildCubeMapCallback = AZStd::function<void(uint8_t* const* cubeMapTextureData, const RHI::Format cubeMapTextureFormat)>;

        enum CubeMapAssetNotificationType
        {
            None,
            Ready,
            Error,
            Reloaded
        };

        //! The ReflectionProbeFeatureProcessorInterface provides an interface to the feature processor for code outside of Atom.
        class ReflectionProbeFeatureProcessorInterface
            : public RPI::FeatureProcessor
        {
        public:
            AZ_RTTI(AZ::Render::ReflectionProbeFeatureProcessorInterface, "{805FA0F8-765A-4072-A8B1-41C4708B6E36}");

            //! Add a new reflection probe, returns the handle of the new probe
            virtual ReflectionProbeHandle AddReflectionProbe(const AZ::Transform& transform, bool useParallaxCorrection) = 0;

            //! Remove an existing reflection probe
            virtual void RemoveReflectionProbe(ReflectionProbeHandle& handle) = 0;

            //! Check to see if a reflection probe handle is valid
            virtual bool IsValidHandle(const ReflectionProbeHandle& handle) const = 0;

            //! Set or retrieve outer extents
            virtual void SetOuterExtents(const ReflectionProbeHandle& handle, const AZ::Vector3& outerExtents) = 0;
            virtual AZ::Vector3 GetOuterExtents(const ReflectionProbeHandle& handle) const = 0;

            //! Set or retrieve inner extents
            virtual void SetInnerExtents(const ReflectionProbeHandle& handle, const AZ::Vector3& innerExtents) = 0;
            virtual AZ::Vector3 GetInnerExtents(const ReflectionProbeHandle& handle) const = 0;

            //! Retrieve outer OBB
            virtual AZ::Obb GetOuterObbWs(const ReflectionProbeHandle& handle) const = 0;

            //! Retrieve inner OBB
            virtual AZ::Obb GetInnerObbWs(const ReflectionProbeHandle& handle) const = 0;

            //! Set or retrieve transform
            virtual void SetTransform(const ReflectionProbeHandle& handle, const AZ::Transform& transform) = 0;
            virtual AZ::Transform GetTransform(const ReflectionProbeHandle& handle) const = 0;

            //! Set or retrieve cubemap
            virtual void SetCubeMap(const ReflectionProbeHandle& handle, Data::Instance<RPI::Image>& cubeMapImage, const AZStd::string& relativePath) = 0;
            virtual Data::Instance<RPI::Image> GetCubeMap(const ReflectionProbeHandle& handle) const = 0;

            //! Set or retrieve render exposure
            virtual void SetRenderExposure(const ReflectionProbeHandle& handle, float renderExposure) = 0;
            virtual float GetRenderExposure(const ReflectionProbeHandle& handle) const = 0;

            //! Set or retrieve bake exposure
            virtual void SetBakeExposure(const ReflectionProbeHandle& handle, float bakeExposure) = 0;
            virtual float GetBakeExposure(const ReflectionProbeHandle& handle) const = 0;

            //! Retrieve parallax correction setting
            virtual bool GetUseParallaxCorrection(const ReflectionProbeHandle& handle) const = 0;

            //! Show or Hide visualization sphere
            virtual void ShowVisualization(const ReflectionProbeHandle& handle, bool showVisualization) = 0;

            //! Bake a reflection cubemap
            virtual void Bake(const ReflectionProbeHandle& handle, BuildCubeMapCallback callback, const AZStd::string& relativePath) = 0;

            //! Check the status of a cubemap bake
            //! Note: For new cubemap bakes only, re-bakes of an existing cubemap are automatically hot-reloaded by the RPI
            virtual bool CheckCubeMapAssetNotification(const AZStd::string& relativePath, Data::Asset<RPI::StreamingImageAsset>& outCubeMapAsset, CubeMapAssetNotificationType& outNotificationType) = 0;

            //! Check to see if a cubemap is referenced by any reflection probes
            virtual bool IsCubeMapReferenced(const AZStd::string& relativePath) = 0;

            //! Find all reflection probes that overlap the specified position
            //! Note: result list is sorted by descending inner volume size
            virtual void FindReflectionProbes(const AZ::Vector3& position, ReflectionProbeHandleVector& reflectionProbeHandles) = 0;

            //! Find all reflection probes that overlap the specified AABB
            //! Note: result list is sorted by descending inner volume size
            virtual void FindReflectionProbes(const AZ::Aabb& aabb, ReflectionProbeHandleVector& reflectionProbeHandles) = 0;
        };
    } // namespace Render
} // namespace AZ
