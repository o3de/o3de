/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
