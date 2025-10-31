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
        class CubeMapCapture;

        using CubeMapCaptureHandle = AZStd::shared_ptr<CubeMapCapture>;
        using RenderCubeMapCallback = AZStd::function<void(uint8_t* const* cubeMapTextureData, const RHI::Format cubeMapTextureFormat)>;
        
        // CubeMapCaptureFeatureProcessorInterface provides an interface to the feature processor for code outside of Atom
        class CubeMapCaptureFeatureProcessorInterface
            : public RPI::FeatureProcessor
        {
        public:
            AZ_RTTI(AZ::Render::CubeMapCaptureFeatureProcessorInterface, "{77C6838D-6693-4CF4-9FFC-8110C4551761}", AZ::RPI::FeatureProcessor);

            virtual CubeMapCaptureHandle AddCubeMapCapture(const AZ::Transform& transform) = 0;
            virtual void RemoveCubeMapCapture(CubeMapCaptureHandle& cubeMapCapture) = 0;
            virtual void SetTransform(const CubeMapCaptureHandle& cubeMapCapture, const AZ::Transform& transform) = 0;
            virtual void SetExposure(const CubeMapCaptureHandle& cubeMapCapture, float exposure) = 0;
            virtual void SetRelativePath(const CubeMapCaptureHandle& cubeMapCapture, const AZStd::string& relativePath) = 0;
            virtual void RenderCubeMap(const CubeMapCaptureHandle& cubeMapCapture, RenderCubeMapCallback callback, const AZStd::string& relativePath) = 0;
            virtual bool IsCubeMapReferenced(const AZStd::string& relativePath) = 0;
        };
    } // namespace Render
} // namespace AZ
