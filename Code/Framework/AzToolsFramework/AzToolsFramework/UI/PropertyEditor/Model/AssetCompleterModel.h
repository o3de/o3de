/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/AssetBrowser/AssetBrowserFilterModel.h>
#endif

namespace AzToolsFramework
{
    using namespace AzToolsFramework::AssetBrowser;
    
    //! Model storing all the files that can be suggested in the Asset Autocompleter for PropertyAssetCtrl
    class AssetCompleterModel
        : public QAbstractTableModel
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(AssetCompleterModel, AZ::SystemAllocator, 0);
        explicit AssetCompleterModel(QObject* parent = nullptr);
        ~AssetCompleterModel();

        int rowCount(const QModelIndex &parent = QModelIndex()) const override;
        int columnCount(const QModelIndex &parent = QModelIndex()) const override;
        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

        void SetFilter(AZ::Data::AssetType filterType);
        void RefreshAssetList();
        void SearchStringHighlight(QString searchString);

        Qt::ItemFlags flags(const QModelIndex &index) const override;

        const AZStd::string_view GetNameFromIndex(const QModelIndex& index);
        const AZ::Data::AssetId GetAssetIdFromIndex(const QModelIndex& index);

    private:
        struct AssetItem 
        {
            AZStd::string m_displayName;
            AZStd::string m_path;
            AZ::Data::AssetId m_assetId;
        };

        AssetBrowserEntry* GetAssetEntry(QModelIndex index) const;
        void FetchResources(QModelIndex index);

        //! Store assetBrowserFilterModel
        AZStd::unique_ptr<AssetBrowserFilterModel> m_assetBrowserFilterModel;
        //! Stores list of assets to suggest via the autocompleter
        AZStd::vector<AssetItem> m_assets;
        //! String that will be highlighted in the suggestions
        QString m_highlightString;
    };

}
