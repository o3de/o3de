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

#include "MaterialThumbnailRenderer.h"

// AzCore
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>

// AzToolsFramework
#include <AzToolsFramework/AssetBrowser/EBusFindAssetTypeByName.h>

// CryCommon
#include <CryCommon/IStreamEngine.h>

// Editor
#include "Util/Image.h"
#include "Controls/PreviewModelCtrl.h"
#include "Material/MaterialManager.h"


const char* MATERIAL_PREVIEW_MODEL_FILE = "Editor/Objects/MtlSphere.cgf";

MaterialThumbnailRenderer::MaterialThumbnailRenderer()
{
    m_previewControl = AZStd::make_unique<CPreviewModelCtrl>();
    m_previewControl->SetGrid(false);
    m_previewControl->SetAxis(false);
    m_previewControl->SetClearColor(ColorF(0, 0, 0, 0));

    EBusFindAssetTypeByName result("Material");
    AZ::AssetTypeInfoBus::BroadcastResult(result, &AZ::AssetTypeInfo::GetAssetType);
    m_assetType = result.GetAssetType();

    AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::Handler::BusConnect(m_assetType);
    AZ::SystemTickBus::Handler::BusConnect();
}

MaterialThumbnailRenderer::~MaterialThumbnailRenderer()
{
    AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::Handler::BusDisconnect();
    AZ::SystemTickBus::Handler::BusDisconnect();
}

void MaterialThumbnailRenderer::OnSystemTick()
{
    AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestBus::ExecuteQueuedEvents();
}

void MaterialThumbnailRenderer::RenderThumbnail(AZ::Data::AssetId assetId, int thumbnailSize)
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

bool MaterialThumbnailRenderer::Installed() const 
{
    return true;
}

bool MaterialThumbnailRenderer::Render(QPixmap& thumbnail, AZ::Data::AssetId assetId, int thumbnailSize) const
{
    // get filepath
    AZStd::string path;
    AZ::Data::AssetCatalogRequestBus::BroadcastResult(
        path,
        &AZ::Data::AssetCatalogRequests::GetAssetPathById,
        assetId);

    auto material = GetIEditor()->GetMaterialManager()->LoadMaterial(path.c_str(), false);
    m_previewControl->LoadFile(MATERIAL_PREVIEW_MODEL_FILE);
    m_previewControl->SetMaterial(material);
    m_previewControl->FitToScreen();

    gEnv->p3DEngine->Update();
    gEnv->pSystem->GetStreamEngine()->Update();

    m_previewControl->Update(true);
    m_previewControl->repaint();
    CImageEx img;
    m_previewControl->show();
    // ensure all the initial (might be first time show) event handling is done for m_previewControl
    QCoreApplication::sendPostedEvents(m_previewControl.get());
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
