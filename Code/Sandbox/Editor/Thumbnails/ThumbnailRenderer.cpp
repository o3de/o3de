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

#include <AzCore/Asset/AssetTypeInfoBus.h>
#include <IStreamEngine.h>
#include <Editor/Thumbnails/ThumbnailRenderer.h>
#include <Editor/Controls/PreviewModelCtrl.h>
#include <Editor/Material/MaterialManager.h>

const char* MATERIAL_PREVIEW_MODEL_FILE = "Editor/Objects/MtlSphere.cgf";

ThumbnailRenderer::ThumbnailRenderer()
{
    m_previewControl = std::make_unique<CPreviewModelCtrl>();
    m_previewControl->SetGrid(false);
    m_previewControl->SetAxis(false);
    m_previewControl->SetClearColor(ColorF(0, 0, 0, 0));

    AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestsBus::Handler::BusConnect();
    AZ::SystemTickBus::Handler::BusConnect();
}

ThumbnailRenderer::~ThumbnailRenderer()
{
    AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestsBus::Handler::BusDisconnect();
    AZ::SystemTickBus::Handler::BusDisconnect();
}

void ThumbnailRenderer::OnSystemTick()
{
    AzToolsFramework::Thumbnailer::ThumbnailerRendererRequestsBus::ExecuteQueuedEvents();
}

void ThumbnailRenderer::RenderThumbnail(AZ::Data::AssetId assetId, int thumbnailSize)
{
    m_previewControl->setFixedSize(thumbnailSize, thumbnailSize);

    // get asset type name
    AZ::Data::AssetInfo info;
    AZ::Data::AssetCatalogRequestBus::BroadcastResult(info, &AZ::Data::AssetCatalogRequests::GetAssetInfoById, assetId);
    AZ::Data::AssetType assetType = info.m_assetType;
    QString assetTypeName;
    AZ::AssetTypeInfoBus::EventResult(assetTypeName, assetType, &AZ::AssetTypeInfo::GetAssetTypeDisplayName);

    // get filepath
    AZStd::string path;
    AZ::Data::AssetCatalogRequestBus::BroadcastResult(
        path,
        &AZ::Data::AssetCatalogRequests::GetAssetPathById,
        assetId);

    bool success = false;
    QPixmap thumbnail;

    if (assetTypeName == "Static Mesh")
    {
        success = RenderMesh(thumbnail, path.c_str(), thumbnailSize);
    }
    else if (assetTypeName == "Material")
    {
        success = RenderMaterial(thumbnail, path.c_str(), thumbnailSize);
    }
    else if (assetTypeName == "Texture")
    {
        success = RenderTexture(thumbnail, path.c_str(), thumbnailSize);
    }

    if (success)
    {
        AzToolsFramework::Thumbnailer::ThumbnailerRendererNotificationsBus::Event(assetId,
            &AzToolsFramework::Thumbnailer::ThumbnailerRendererNotifications::ThumbnailRendered, thumbnail);
    }
    else
    {
        AzToolsFramework::Thumbnailer::ThumbnailerRendererNotificationsBus::Event(assetId,
            &AzToolsFramework::Thumbnailer::ThumbnailerRendererNotifications::ThumbnailFailedToRender);
    }
}

bool ThumbnailRenderer::RenderMesh(QPixmap& thumbnail, const char* path, int thumbnailSize) const
{
    m_previewControl->SetMaterial(nullptr);
    m_previewControl->LoadFile(path);
    m_previewControl->FitToScreen();

    gEnv->p3DEngine->Update();
    gEnv->pSystem->GetStreamEngine()->Update();

    m_previewControl->Update(true);
    m_previewControl->repaint();
    m_previewControl->hide();

    CImageEx img;
    m_previewControl->GetImageOffscreen(img, QSize(thumbnailSize, thumbnailSize));

    thumbnail = QPixmap::fromImage(QImage(reinterpret_cast<uchar*>(img.GetData()),
                img.GetWidth(), img.GetHeight(), QImage::Format_ARGB32)).copy();

    img.Release();
    return true;
}

bool ThumbnailRenderer::RenderMaterial(QPixmap& thumbnail, const char* path, int thumbnailSize) const
{
    auto material = GetIEditor()->GetMaterialManager()->LoadMaterial(path, false);
    m_previewControl->LoadFile(MATERIAL_PREVIEW_MODEL_FILE);
    m_previewControl->SetMaterial(material);
    m_previewControl->FitToScreen();

    gEnv->p3DEngine->Update();
    gEnv->pSystem->GetStreamEngine()->Update();

    m_previewControl->Update(true);
    m_previewControl->repaint();
    m_previewControl->hide();

    CImageEx img;
    m_previewControl->GetImageOffscreen(img, QSize(thumbnailSize, thumbnailSize));

    thumbnail = QPixmap::fromImage(QImage(reinterpret_cast<uchar*>(img.GetData()),
                img.GetWidth(), img.GetHeight(), QImage::Format_ARGB32)).copy();

    img.Release();

    return true;
}

bool ThumbnailRenderer::RenderTexture(QPixmap& thumbnail, const char* path, int thumbnailSize) const
{
    CImageEx img;
    if (!CImageUtil::LoadImage(path, img))
    {
        return false;
    }

    unsigned int* pData = img.GetData();

    if (!pData)
    {
        img.Release();
        return false;
    }

    if (0 == img.GetWidth() || 0 == img.GetHeight())
    {
        img.Release();
        return false;
    }

    thumbnail = QPixmap::fromImage(QImage(reinterpret_cast<uchar*>(pData), img.GetWidth(), img.GetHeight(), QImage::Format_ARGB32)
            .scaled(thumbnailSize, thumbnailSize, Qt::IgnoreAspectRatio)).copy();

    return true;
}
