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

#include "StaticMeshThumbnailRenderer.h"

// AzCore
#include <AzCore/Asset/AssetManagerBus.h>

// AzToolsFramework
#include <AzToolsFramework/AssetBrowser/EBusFindAssetTypeByName.h>

// CryCommon
#include <CryCommon/IStreamEngine.h>

// Editor
#include "Controls/PreviewModelCtrl.h"
#include "Util/Image.h"


StaticMeshThumbnailRenderer::StaticMeshThumbnailRenderer()
{
    m_previewControl = AZStd::make_unique<CPreviewModelCtrl>();
    m_previewControl->SetGrid(false);
    m_previewControl->SetAxis(false);
    m_previewControl->SetClearColor(ColorF(0, 0, 0, 0));

    EBusFindAssetTypeByName result("Static Mesh");
    AZ::AssetTypeInfoBus::BroadcastResult(result, &AZ::AssetTypeInfo::GetAssetType);
    m_assetType = result.GetAssetType();

    AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::Handler::BusConnect(m_assetType);
    AZ::SystemTickBus::Handler::BusConnect();
}

StaticMeshThumbnailRenderer::~StaticMeshThumbnailRenderer()
{
    AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::Handler::BusDisconnect();
    AZ::SystemTickBus::Handler::BusDisconnect();
}

void StaticMeshThumbnailRenderer::OnSystemTick()
{
    AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::ExecuteQueuedEvents();
}

void StaticMeshThumbnailRenderer::RenderThumbnail(AZ::Data::AssetId assetId, int thumbnailSize)
{
    m_previewControl->setFixedSize(thumbnailSize, thumbnailSize);

    // get asset type name
    AZ::Data::AssetInfo info;
    AZ::Data::AssetCatalogRequestBus::BroadcastResult(info, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, assetId);
    AZ::Data::AssetType assetType = info.m_assetType;
    QString assetTypeName;
    AZ::AssetTypeInfoBus::EventResult(assetTypeName, assetType, &AZ::AssetTypeInfo::GetAssetTypeDisplayName);

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

bool StaticMeshThumbnailRenderer::Installed() const 
{
    return true;
}

bool StaticMeshThumbnailRenderer::Render(QPixmap& thumbnail, AZ::Data::AssetId assetId, int thumbnailSize) const
{
    // get filepath
    AZStd::string path;
    AZ::Data::AssetCatalogRequestBus::BroadcastResult(
        path,
        &AZ::Data::AssetCatalogRequests::GetAssetPathById,
        assetId);

    m_previewControl->SetMaterial(nullptr);
    m_previewControl->LoadFile(path.c_str());
    m_previewControl->FitToScreen();

    gEnv->p3DEngine->Update();
    gEnv->pSystem->GetStreamEngine()->Update();

    m_previewControl->Update(true);
    m_previewControl->repaint();
    CImageEx img;
    // getimageoffscreen actually requires a real operating system window handle resource, which hiding the window can cause
    // to be lost.
    m_previewControl->GetImageOffscreen(img, QSize(thumbnailSize, thumbnailSize));
    m_previewControl->hide();

    if (img.IsValid())
    {
        // this can fail if the request to draw the thumbnail was queued up but then the window
        // was hidden or deleted in the interim.
        thumbnail = QPixmap::fromImage(QImage(reinterpret_cast<uchar*>(img.GetData()),
            img.GetWidth(), img.GetHeight(), QImage::Format_ARGB32)).copy();
        img.Release();
        return true;
    }

    return false;
}
