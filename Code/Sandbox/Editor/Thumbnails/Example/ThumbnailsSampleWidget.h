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
#include <Editor/Util/FileUtil.h>
#include <Editor/Util/Image.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzToolsFramework/Thumbnails/Thumbnail.h>

#include <QWidget>
#include <QScopedPointer>
#endif

class QItemSelection;

namespace Ui
{
    class ThumbnailsSampleWidgetClass;
}

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class ProductAssetBrowserEntry;
        class AssetBrowserEntry;
        class AssetBrowserFilterModel;
        class AssetBrowserModel;
    }
}

class ThumbnailsSampleWidget
    : public QWidget
{
    Q_OBJECT
public:
    AZ_CLASS_ALLOCATOR(ThumbnailsSampleWidget, AZ::SystemAllocator, 0);

    explicit ThumbnailsSampleWidget(QWidget* parent = nullptr);
    ~ThumbnailsSampleWidget() override;

    static void RegisterViewClass();

private:
    QScopedPointer<Ui::ThumbnailsSampleWidgetClass> m_ui;
    QScopedPointer<AzToolsFramework::AssetBrowser::AssetBrowserFilterModel> m_filterModel;
    AzToolsFramework::AssetBrowser::AssetBrowserModel* m_assetBrowserModel;

    void UpdateThumbnail() const;

private Q_SLOTS:
    void SelectionChangedSlot(const QItemSelection& selected, const QItemSelection& deselected) const;
};
