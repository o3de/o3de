/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <UI/PropertyEditor/Model/AssetCompleterModel.h>
#include <UI/PropertyEditor/Model/moc_AssetCompleterModel.cpp>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Search/Filter.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace AzToolsFramework
{
    AssetCompleterModel::AssetCompleterModel(QObject* parent) 
        : QAbstractTableModel(parent)
    {

        AssetBrowserModel* assetBrowserModel = nullptr;
        AssetBrowserComponentRequestBus::BroadcastResult(assetBrowserModel, &AssetBrowserComponentRequests::GetAssetBrowserModel);

        AZ_Error("AssetCompleterModel", (assetBrowserModel != nullptr), "Unable to setup Source Model, asset browser model was not returned correctly.");

        m_assetBrowserFilterModel = AZStd::unique_ptr<AssetBrowserFilterModel>(aznew AssetBrowserFilterModel(this));
        m_assetBrowserFilterModel->setSourceModel(assetBrowserModel);
        m_assetBrowserFilterModel->sort(0, Qt::DescendingOrder);
    }

    AssetCompleterModel::~AssetCompleterModel()
    {
    }

    void AssetCompleterModel::SetFilter(AZ::Data::AssetType filterType)
    {
        AssetTypeFilter* typeFilter = new AssetTypeFilter();
        typeFilter->SetAssetType(filterType);
        typeFilter->SetFilterPropagation(AssetBrowserEntryFilter::PropagateDirection::Down);

        SetFilter(FilterConstType(typeFilter));
    }

    void AssetCompleterModel::SetFilter(FilterConstType filter)
    {
        m_assetBrowserFilterModel->SetFilter(filter);

        RefreshAssetList();
    }

    void AssetCompleterModel::RefreshAssetList()
    {
        beginResetModel();

        m_assets.clear();
        FetchResources(QModelIndex());

        endResetModel();

        emit dataChanged(index(0, 0), index(rowCount(), columnCount()));
    }

    void AssetCompleterModel::SearchStringHighlight(QString searchString)
    {
        m_highlightString = searchString;
    }

    int AssetCompleterModel::rowCount(const QModelIndex&) const
    {
        return static_cast<int>(m_assets.size());
    }

    int AssetCompleterModel::columnCount(const QModelIndex&) const
    {
        /*  Model has 2 columns:
        *   - Column 0 returns the Asset Name (used for autocompletion)
        *   - Column 1 returns the Asset Name with highlight of current Search String (used in suggestions popup)
        */
        return 2;
    }

    QVariant AssetCompleterModel::data(const QModelIndex& index, int role) const
    {
        switch(role) 
        {
            case Qt::DisplayRole:
            {
                if (index.column() == 0 || m_highlightString.isEmpty())
                {
                    return QString(m_assets[index.row()].m_displayName.data());
                }
                                
                QString displayString = QString(m_assets[index.row()].m_displayName.data());
                return  "<span style=\"color: #fff;\">" + 
                            displayString.replace(m_highlightString, "<span style=\"background-color: #498FE1\">" + m_highlightString + "</span>", Qt::CaseInsensitive) + 
                        "</span>";
            }

            case Qt::ToolTipRole:
            {
                return QString(m_assets[index.row()].m_path.data());
            }

            default:
            {
                return QVariant();
            }
        }
    }

    AssetBrowserEntry* AssetCompleterModel::GetAssetEntry(QModelIndex index) const
    {
        if (!index.isValid())
        {
            AZ_Error("AssetCompleterModel", false, "Invalid Source Index provided to GetAssetEntry.");
            return nullptr;
        }

        return static_cast<AssetBrowserEntry*>(index.internalPointer());
    }

    void AssetCompleterModel::FetchResources(QModelIndex index)
    {
        int rows = m_assetBrowserFilterModel->rowCount(index);
        if (rows == 0)
        {
            return;
        }

        for (int i = 0; i < rows; ++i)
        {
            QModelIndex childIndex = m_assetBrowserFilterModel->index(i, 0, index);
            AssetBrowserEntry* childEntry = GetAssetEntry(m_assetBrowserFilterModel->mapToSource(childIndex));

            if (childEntry->GetEntryType() == m_entryType)
            {
                ProductAssetBrowserEntry* productEntry = static_cast<ProductAssetBrowserEntry*>(childEntry);
                AZStd::string assetName;
                AzFramework::StringFunc::Path::GetFileName(productEntry->GetFullPath().c_str(), assetName);
                m_assets.push_back({ productEntry->GetName(), productEntry->GetFullPath(), productEntry->GetAssetId()
                });
            }

            if (childEntry->GetChildCount() > 0) 
            {
                FetchResources(childIndex);
            }
        }
    }

    Qt::ItemFlags AssetCompleterModel::flags(const QModelIndex &index) const
    {
        if (!index.isValid())
        {
            return Qt::NoItemFlags;
        }

        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    }

    const AZStd::string_view AssetCompleterModel::GetNameFromIndex(const QModelIndex& index) 
    {
        if (!index.isValid())
        {
            return "";
        }

        return m_assets[index.row()].m_displayName;
    }

    const AZ::Data::AssetId AssetCompleterModel::GetAssetIdFromIndex(const QModelIndex& index) 
    {
        if (!index.isValid())
        {
            return AZ::Data::AssetId();
        }

        return m_assets[index.row()].m_assetId;
    }

    const AZStd::string_view AssetCompleterModel::GetPathFromIndex(const QModelIndex& index)
    {
        if (!index.isValid())
        {
            return "";
        }

        return m_assets[index.row()].m_path;
    }

    void AssetCompleterModel::SetFetchEntryType(AssetBrowserEntry::AssetEntryType entryType)
    {
        m_entryType = entryType;
    }
}
