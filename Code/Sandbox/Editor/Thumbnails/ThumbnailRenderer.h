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
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/TickBus.h>

#include <AzToolsFramework/Thumbnails/ThumbnailerBus.h>

class CPreviewModelCtrl;

//! Loads thumbnails that require acccess to renderer
class ThumbnailRenderer
    : public AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestsBus::Handler
    , public AZ::SystemTickBus::Handler
{
public:
    AZ_CLASS_ALLOCATOR(ThumbnailRenderer, AZ::SystemAllocator, 0)

    ThumbnailRenderer();
    ~ThumbnailRenderer();

    //////////////////////////////////////////////////////////////////////////
    // TickBus
    //////////////////////////////////////////////////////////////////////////
    void OnSystemTick() override;

    //////////////////////////////////////////////////////////////////////////
    // ThumbnailerRendererRequests
    //////////////////////////////////////////////////////////////////////////
    void RenderThumbnail(AZ::Data::AssetId assetId, int thumbnailSize) override;

private:
    AZStd::unique_ptr<CPreviewModelCtrl> m_previewControl;

    bool RenderMesh(QPixmap& thumbnail, const char* path, int thumbnailSize) const;
    bool RenderMaterial(QPixmap& thumbnail, const char* path, int thumbnailSize) const;
    bool RenderTexture(QPixmap& thumbnail, const char* path, int thumbnailSize) const;
};
