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

#include "EditorDefs.h"

#include "TextureThumbnailRenderer.h"

// AzCore
#include <AzCore/Asset/AssetManagerBus.h>

// AzToolsFramework
#include <AzToolsFramework/AssetBrowser/EBusFindAssetTypeByName.h>

// Editor
#include "Util/Image.h"
#include "Util/ImageUtil.h"


TextureThumbnailRenderer::TextureThumbnailRenderer()
{
    EBusFindAssetTypeByName result("Texture");
    AZ::AssetTypeInfoBus::BroadcastResult(result, &AZ::AssetTypeInfo::GetAssetType);
    m_assetType = result.GetAssetType();

    AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::Handler::BusConnect(m_assetType);
    AZ::SystemTickBus::Handler::BusConnect();
}

TextureThumbnailRenderer::~TextureThumbnailRenderer()
{
    AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::Handler::BusDisconnect();
    AZ::SystemTickBus::Handler::BusDisconnect();
}

void TextureThumbnailRenderer::OnSystemTick()
{
    AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::ExecuteQueuedEvents();
}

void TextureThumbnailRenderer::RenderThumbnail(AZ::Data::AssetId assetId, int thumbnailSize)
{
    QPixmap thumbnail;

    if (Render(thumbnail, assetId, thumbnailSize))
    {
        AzToolsFramework::Thumbnailer::ThumbnailerRendererNotificationBus::Event(assetId,
            &AzToolsFramework::Thumbnailer::ThumbnailerRendererNotifications::ThumbnailRendered, thumbnail);
    }
    else
    {
        AzToolsFramework::Thumbnailer::ThumbnailerRendererNotificationBus::Event(assetId,
            &AzToolsFramework::Thumbnailer::ThumbnailerRendererNotifications::ThumbnailFailedToRender);
    }
}

bool TextureThumbnailRenderer::Installed() const 
{
    return true;
}

bool TextureThumbnailRenderer::Render(QPixmap& thumbnail, AZ::Data::AssetId assetId, int thumbnailSize) const
{
    // get filepath
    AZStd::string path;
    AZ::Data::AssetCatalogRequestBus::BroadcastResult(
        path,
        &AZ::Data::AssetCatalogRequests::GetAssetPathById,
        assetId);

    CImageEx img;
    if (!CImageUtil::LoadImage(path.c_str(), img))
    {
        return false;
    }

    unsigned int* data = img.GetData();

    if (!data)
    {
        img.Release();
        return false;
    }

    if (0 == img.GetWidth() || 0 == img.GetHeight())
    {
        img.Release();
        return false;
    }

    thumbnail = QPixmap::fromImage(QImage(reinterpret_cast<uchar*>(data), img.GetWidth(), img.GetHeight(), QImage::Format_ARGB32)
                .scaled(thumbnailSize, thumbnailSize, Qt::IgnoreAspectRatio)).copy();

    return true;
}
