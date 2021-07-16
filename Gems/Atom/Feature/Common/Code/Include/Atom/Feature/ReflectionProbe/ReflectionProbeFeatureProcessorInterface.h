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

        using ReflectionProbeHandle = AZStd::shared_ptr<ReflectionProbe>;
        using BuildCubeMapCallback = AZStd::function<void(uint8_t* const* cubeMapTextureData, const RHI::Format cubeMapTextureFormat)>;

        enum CubeMapAssetNotificationType
        {
            None,
            Ready,
            Error,
            Reloaded
        };

        // ReflectionProbeFeatureProcessorInterface provides an interface to the feature processor for code outside of Atom
        class ReflectionProbeFeatureProcessorInterface
            : public RPI::FeatureProcessor
        {
        public:
            AZ_RTTI(AZ::Render::ReflectionProbeFeatureProcessorInterface, "{805FA0F8-765A-4072-A8B1-41C4708B6E36}");

            virtual ReflectionProbeHandle AddProbe(const AZ::Transform& transform, bool useParallaxCorrection) = 0;
            virtual void RemoveProbe(ReflectionProbeHandle& handle) = 0;
            virtual void SetProbeOuterExtents(const ReflectionProbeHandle& handle, const AZ::Vector3& outerExtents) = 0;
            virtual void SetProbeInnerExtents(const ReflectionProbeHandle& handle, const AZ::Vector3& innerExtents) = 0;
            virtual void SetProbeCubeMap(const ReflectionProbeHandle& handle, Data::Instance<RPI::Image>& cubeMapImage, const AZStd::string& relativePath) = 0;
            virtual void SetProbeTransform(const ReflectionProbeHandle& handle, const AZ::Transform& transform) = 0;
            virtual void BakeProbe(const ReflectionProbeHandle& handle, BuildCubeMapCallback callback, const AZStd::string& relativePath) = 0;
            virtual bool CheckCubeMapAssetNotification(const AZStd::string& relativePath, Data::Asset<RPI::StreamingImageAsset>& outCubeMapAsset, CubeMapAssetNotificationType& outNotificationType) = 0;
            virtual bool IsCubeMapReferenced(const AZStd::string& relativePath) = 0;
            virtual bool IsValidProbeHandle(const ReflectionProbeHandle& probe) const = 0;
            virtual void ShowProbeVisualization(const ReflectionProbeHandle& probe, bool showVisualization) = 0;
        };
    } // namespace Render
} // namespace AZ
