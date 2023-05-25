/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/utils.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetDatabase/AssetDatabaseConnection.h>
#include <AzToolsFramework/AssetBrowser/Entries/ProductAssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Entries/SourceAssetBrowserEntry.h>
#include <AzQtComponents/Components/Widgets/ElidingLabel.h>
#include <AzQtComponents/Components/Widgets/Card.h>
#include <QDockWidget>
#include <QFont>
#include <QFormLayout>
#include <QLabel>
#include <QLayout>
#include <QStackedLayout>
#include <QTreeWidget>
#include <QWidget>
#endif

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class PreviewerFrame;

        class AssetBrowserEntityInspectorWidget
            : public QWidget
            , public AssetBrowserPreviewRequestBus::Handler
            , public AssetDatabaseLocationNotificationBus::Handler
        {
        public:
            explicit AssetBrowserEntityInspectorWidget(QWidget *parent = nullptr);
            ~AssetBrowserEntityInspectorWidget();

            //////////////////////////////////////////////////////////////////////////
            // AssetDatabaseLocationNotificationBus
            //////////////////////////////////////////////////////////////////////////
            void OnDatabaseInitialized() override;

            //////////////////////////////////////////////////////////////////////////
            // AssetBrowserPreviewRequestBus
            //////////////////////////////////////////////////////////////////////////
            void PreviewAsset(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* selectedEntry) override;
            void ClearPreview() override;

            void PopulateSourceDependencies(const SourceAssetBrowserEntry* sourceEntry, AZStd::vector<const ProductAssetBrowserEntry*> productList);
            bool PopulateProductDependencies(const ProductAssetBrowserEntry* productEntry);
            void CreateSourceDependencyTree(AZStd::set<AZ::Uuid> sourceUuids, bool isOutgoing);
            void CreateProductDependencyTree(AZStd::set<AZ::Data::AssetId> dependencyUuids, bool isOutgoing);
            void AddAssetBrowserEntryToTree(const AssetBrowserEntry* entry, QTreeWidgetItem* headerItem);
        private:
            AZStd::shared_ptr<AssetDatabase::AssetDatabaseConnection> m_databaseConnection;
            QLabel* m_previewImage = nullptr;
            QStackedLayout* m_layoutSwitcher = nullptr;
            QWidget* m_emptyLayoutWidget = nullptr;
            QWidget* m_populatedLayoutWidget = nullptr;
            QFormLayout* m_assetDetailLayout = nullptr;
            QWidget* m_assetDetailWidget = nullptr;
            QTreeWidget* m_dependentProducts = nullptr;
            AzQtComponents::Card* m_dependentAssetsCard = nullptr;
            QFont m_headerFont;
        };

    } // namespace AssetBrowser
} // namespace AzToolsFramework

