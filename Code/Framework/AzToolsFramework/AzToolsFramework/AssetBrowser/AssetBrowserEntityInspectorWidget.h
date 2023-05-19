/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QDockWidget>
#include <QFormLayout>
#include <QLabel>
#include <QLayout>
#include <QStackedLayout>
#include <QWidget>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzQtComponents/Components/Widgets/ElidingLabel.h>
#endif

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class PreviewerFrame;

        class AssetBrowserEntityInspectorWidget
            : public QWidget
            , public AssetBrowserPreviewRequestBus::Handler
        {
        public:
            explicit AssetBrowserEntityInspectorWidget(QWidget *parent = nullptr);
            ~AssetBrowserEntityInspectorWidget();
        private:
            // AssetBrowserPreviewRequestBus overrides ...
            void PreviewAsset(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* selectedEntry) override;
            void ClearPreview() override;

            QLabel* m_previewImage = nullptr;
            QStackedLayout* m_layoutSwitcher = nullptr;
            QWidget* m_emptyLayoutWidget = nullptr;
            QWidget* m_populatedLayoutWidget = nullptr;
            QFormLayout* m_assetDetailLayout = nullptr;
            AzQtComponents::ElidingLabel* m_nameLabel = nullptr;
            AzQtComponents::ElidingLabel* m_locationLabel = nullptr;
            AzQtComponents::ElidingLabel* m_fileTypeLabel = nullptr;
            AzQtComponents::ElidingLabel* m_assetTypeLabel = nullptr;
            AzQtComponents::ElidingLabel* m_diskSizeLabel = nullptr;
            AzQtComponents::ElidingLabel* m_dimensionLabel = nullptr;
            AzQtComponents::ElidingLabel* m_verticesLabel = nullptr;

        };

    } // namespace AssetBrowser
} // namespace AzToolsFramework

