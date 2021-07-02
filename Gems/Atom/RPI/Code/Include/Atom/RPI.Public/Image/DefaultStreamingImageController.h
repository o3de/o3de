/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Reflect/Image/DefaultStreamingImageControllerAsset.h>

#include <Atom/RPI.Public/Image/StreamingImageController.h>
#include <Atom/RPI.Public/Image/StreamingImageContext.h>
#include <Atom/RPI.Public/Image/StreamingImage.h>

namespace AZ
{
    namespace RPI
    {
        class DefaultStreamingImageController final
            : public StreamingImageController
        {
            friend class ImageSystem;
        public:
            AZ_RTTI(DefaultStreamingImageController, "{C733B6FC-4918-42CE-A9A5-B68FF7F6C77D}", StreamingImageController)

            static Data::Instance<DefaultStreamingImageController> FindOrCreate(const Data::Asset<DefaultStreamingImageControllerAsset>& asset);

        private:
            // Standard init for InstanceData subclass
            DefaultStreamingImageController() = default;
            static Data::Instance<DefaultStreamingImageController> CreateInternal(Data::AssetData* assetData);
            RHI::ResultCode Init(DefaultStreamingImageControllerAsset& imageControllerAsset);

            ///////////////////////////////////////////////////////////////////
            // StreamingImageController Overrides
            StreamingImageContextPtr CreateContextInternal() override;
            void UpdateInternal(size_t timestamp, const StreamingImageContextList& contexts) override;
            ///////////////////////////////////////////////////////////////////

            // A work queue for doing initial setup after an image is attached.
            AZStd::vector<StreamingImageContextPtr> m_recentlyAttachedContexts;
        };
    }
}
