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

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/TickBus.h>

#include <AzToolsFramework/Thumbnails/ThumbnailerBus.h>

//! Loads thumbnails that require acccess to renderer
class TextureThumbnailRenderer
    : public AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::Handler
    , public AZ::SystemTickBus::Handler
{
public:
    AZ_CLASS_ALLOCATOR(TextureThumbnailRenderer, AZ::SystemAllocator, 0)

    TextureThumbnailRenderer();
    ~TextureThumbnailRenderer();

    //////////////////////////////////////////////////////////////////////////
    // TickBus
    //////////////////////////////////////////////////////////////////////////
    void OnSystemTick() override;

    //////////////////////////////////////////////////////////////////////////
    // ThumbnailerRendererRequests
    //////////////////////////////////////////////////////////////////////////
    void RenderThumbnail(AZ::Data::AssetId assetId, int thumbnailSize) override;
    bool Installed() const override;

private:
    AZ::Data::AssetType m_assetType;

    bool Render(QPixmap& thumbnail, AZ::Data::AssetId assetId, int thumbnailSize) const;
};
