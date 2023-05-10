/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/AssetBrowser/AssetBrowserEntityInspectorWidget.h>
#include <AzToolsFramework/AssetBrowser/Previewer/PreviewerFrame.h>
#include <AzToolsFramework/AssetBrowser/Entries/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/AssetBrowser/Entries/SourceAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/ProductAssetBrowserEntry.h>
#include <QLayout>
#include <AzCore/Asset/AssetTypeInfoBus.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzQtComponents/Components/Widgets/Card.h>
#include <AzQtComponents/Components/Widgets/CardHeader.h>

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        AssetBrowserEntityInspectorWidget::AssetBrowserEntityInspectorWidget(QWidget* parent)
            : QWidget(parent)
        {
            setLayout(new QVBoxLayout);
            m_previewerFrame = new PreviewerFrame;
            layout()->addWidget(m_previewerFrame);

            auto secondary = new AzQtComponents::Card();
            secondary->setTitle("Secondary Details");
            secondary->header()->setHasContextMenu(false);
            secondary->hideFrame();
            layout()->addWidget(secondary);

            auto dependent = new AzQtComponents::Card();
            dependent->setTitle("Dependent Assets");
            dependent->header()->setHasContextMenu(false);
            dependent->hideFrame();
            layout()->addWidget(dependent);

            auto advanced = new AzQtComponents::Card();
            advanced->setTitle("Advanced Options");
            advanced->header()->setHasContextMenu(false);
            advanced->hideFrame();
            layout()->addWidget(advanced);

            layout()->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding));

            AssetBrowserPreviewRequestBus::Handler::BusConnect();
        }
        AssetBrowserEntityInspectorWidget::~AssetBrowserEntityInspectorWidget()
        {
            AssetBrowserPreviewRequestBus::Handler::BusDisconnect();
        }
        void AssetBrowserEntityInspectorWidget::PreviewAsset(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* selectedEntry)
        {
            if (selectedEntry == nullptr)
            {
                m_previewerFrame->Clear();
                return;
            }
            m_previewerFrame->Display(selectedEntry);
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework
