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

#include <AzToolsFramework/MaterialBrowser/MaterialThumbnail.h>

#include <QPixmap>

namespace AzToolsFramework
{
    namespace MaterialBrowser
    {
        static constexpr const char* SimpleMaterialIconPath = ":/MaterialBrowser/images/material_04.png";
        static constexpr const char* MultiMaterialIconPath = ":/MaterialBrowser/images/material_06.png";

        //////////////////////////////////////////////////////////////////////////
        // MaterialThumbnail
        //////////////////////////////////////////////////////////////////////////
        MaterialThumbnail::MaterialThumbnail(Thumbnailer::SharedThumbnailKey key)
            : Thumbnail(key)
        {
            auto productKey = azrtti_cast<const AzToolsFramework::AssetBrowser::ProductThumbnailKey*>(m_key.data());
            AZ_Assert(productKey, "Incorrect key type, excpected ProductThumbnailKey");

            bool multiMat = false;
            MaterialBrowserRequestBus::BroadcastResult(multiMat, &MaterialBrowserRequests::IsMultiMaterial, productKey->GetAssetId());

            QString iconPath = multiMat ? MultiMaterialIconPath : SimpleMaterialIconPath;
            m_pixmap.load(iconPath);
            m_state = m_pixmap.isNull() ? State::Failed : State::Ready;
        }

        //////////////////////////////////////////////////////////////////////////
        // MaterialThumbnailCache
        //////////////////////////////////////////////////////////////////////////
        MaterialThumbnailCache::MaterialThumbnailCache()
            : ThumbnailCache<MaterialThumbnail, MaterialKeyHash, MaterialKeyEqual>() {}

        MaterialThumbnailCache::~MaterialThumbnailCache() = default;

        int MaterialThumbnailCache::GetPriority() const
        {
            return 1;
        }

        const char* MaterialThumbnailCache::GetProviderName() const
        {
            return ProviderName;
        }

        bool MaterialThumbnailCache::IsSupportedThumbnail(Thumbnailer::SharedThumbnailKey key) const
        {
            return azrtti_istypeof<const AzToolsFramework::AssetBrowser::ProductThumbnailKey*>(key.data());
        }

    } // namespace MaterialBrowser
} // namespace AzToolsFramework

#include "MaterialBrowser/moc_MaterialThumbnail.cpp"
