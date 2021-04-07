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

#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/Thumbnails/Thumbnail.h>
#include <AzToolsFramework/AssetBrowser/Thumbnails/ProductThumbnail.h>
#include <AzToolsFramework/MaterialBrowser/MaterialBrowserBus.h>
#endif

namespace AzToolsFramework
{
    namespace MaterialBrowser
    {
        //! Material Browser uses only 2 thumbnails: simple and multimaterial
        class MaterialThumbnail
            : public Thumbnailer::Thumbnail
        {
            Q_OBJECT
        public:
            MaterialThumbnail(Thumbnailer::SharedThumbnailKey key, int thumbnailSize);
        };

        namespace
        {
            class MaterialKeyHash
            {
            public:
                size_t operator()(const Thumbnailer::SharedThumbnailKey& /*val*/) const
                {
                    return 0; // there is only 2 thumbnails in this cache
                }
            };

            class MaterialKeyEqual
            {
            public:
                bool operator()(const Thumbnailer::SharedThumbnailKey& val1, const Thumbnailer::SharedThumbnailKey& val2) const
                {
                    auto productThumbnailKey1 = azrtti_cast<const AzToolsFramework::AssetBrowser::ProductThumbnailKey*>(val1.data());
                    auto productThumbnailKey2 = azrtti_cast<const AzToolsFramework::AssetBrowser::ProductThumbnailKey*>(val2.data());
                    if (!productThumbnailKey1 || !productThumbnailKey2)
                    {
                        return false;
                    }

                    // check whether keys point to single or multimaterial asset type
                    bool multiMat1 = false;
                    bool multiMat2 = false;
                    MaterialBrowserRequestBus::BroadcastResult(multiMat1, &MaterialBrowserRequests::IsMultiMaterial, productThumbnailKey1->GetAssetId());
                    MaterialBrowserRequestBus::BroadcastResult(multiMat2, &MaterialBrowserRequests::IsMultiMaterial, productThumbnailKey2->GetAssetId());
                    return multiMat1 == multiMat2;
                }
            };
        }

        //! MaterialBrowserEntry thumbnails
        class MaterialThumbnailCache
            : public Thumbnailer::ThumbnailCache<MaterialThumbnail, MaterialKeyHash, MaterialKeyEqual>
        {
        public:
            MaterialThumbnailCache();
            ~MaterialThumbnailCache() override;

            int GetPriority() const override;
            const char* GetProviderName() const override;

            static constexpr const char* ProviderName = "CryMaterial Thumbnails";

        protected:
            bool IsSupportedThumbnail(Thumbnailer::SharedThumbnailKey key) const override;
        };
    } // namespace MaterialBrowser
} // namespace AzToolsFramework


