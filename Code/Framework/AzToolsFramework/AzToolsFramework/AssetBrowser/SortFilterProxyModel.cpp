/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "SortFilterProxyModel.hxx"

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        //////////////////////////////////////////////////////////////////////////
        //SortFilterProxyModel
        SortFilterProxyModel::SortFilterProxyModel(QObject* parent)
            : QSortFilterProxyModel(parent)
            , m_assetMatchFiltersOperator(AzToolsFramework::FilterOperatorType::And)
        {
            //uncomment any column you want to see in the view
            m_showColumn.insert(AssetBrowserEntry::Column::Name);
            //m_showColumn.insert( Entry::Column_SourceID );
            //m_showColumn.insert( Entry::Column_FingerprintValue );
            //m_showColumn.insert( Entry::Colbumn_Guid );
            //m_showColumn.insert( Entry::Column_ScanFolderID );
            //m_showColumn.insert( Entry::Column_ProductID );
            //m_showColumn.insert( Entry::Column_JobID );
            //m_showColumn.insert( Entry::Column_JobKey );
            //m_showColumn.insert( Entry::Column_SubID );
            //m_showColumn.insert( Entry::Column_AssetType );
            //m_showColumn.insert( Entry::Column_Platform );
            //m_showColumn.insert( Entry::Column_ClassID );
        }

        void SortFilterProxyModel::OnSearchCriteriaChanged(QStringList& criteriaList, AzToolsFramework::FilterOperatorType filterOperator)
        {
            removeAllAssetMatchFilters();
            setAssetMatchFilterOperator(filterOperator);

            for (QString criteria : criteriaList)
            {
                auto parts = criteria.split(": ", QString::SkipEmptyParts);
                addAssetMatchFilter(parts.last().toUtf8().constData());
            }
        }

        void SortFilterProxyModel::addAssetTypeFilter(AZ::Data::AssetType assetType)
        {
            m_assetTypeFilters.push_back(assetType);
            invalidateFilter();
        }

        void SortFilterProxyModel::addAssetPathFilter(const char* assetPathFilter)
        {
            m_assetPathFilters.push_back(assetPathFilter);
            invalidateFilter();
        }

        void SortFilterProxyModel::removeAllAssetPathFilters()
        {
            m_assetPathFilters.clear();
            invalidateFilter();
        }

        void SortFilterProxyModel::setAssetMatchSubDirFilter(bool val)
        {
            m_includeSubdir = val;
            invalidateFilter();
        }

        void SortFilterProxyModel::removeAllAssetMatchFilters()
        {
            m_assetMatchFilters.clear();
            invalidateFilter();
        }

        void SortFilterProxyModel::addAssetMatchFilter(const char* assetMatchFilter)
        {
            m_assetMatchFilters.push_back(assetMatchFilter);
            invalidateFilter();
        }

        void SortFilterProxyModel::setAssetMatchFilterOperator(AzToolsFramework::FilterOperatorType type)
        {
            m_assetMatchFiltersOperator = type;
            invalidateFilter();
        }

        bool SortFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
        {
            //get the source idx, if invalid early out
            QModelIndex idx = sourceModel()->index(source_row, 0, source_parent);
            if (!idx.isValid())
            {
                return false;
            }

            //the entry is the internal pointer of the index
            auto entry = static_cast<AssetBrowserEntry*>(idx.internalPointer());

            if (!entry->isValid())
            {
                return false;
            }

            ////////////////////////////////////////////////////////////////////////
            //we only want to see assets that have at least one child product that has a valid assetType
            if (entry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Source)
            {
                //we have a asset with at least one valid child product assetType
                //we only want to see assets that have at least one child product that matches the assetType filter
                if (!m_assetTypeFilters.empty())
                {
                    for (int i = 0; i < entry->GetChildCount(); ++i)
                    {
                        auto product = static_cast<ProductAssetBrowserEntry*>(entry->GetChild(i));
                        if (product->isValid())
                        {
                            if (AZStd::find(m_assetTypeFilters.begin(), m_assetTypeFilters.end(), product->GetAssetType()) == m_assetTypeFilters.end())
                            {
                                return false;
                            }
                        }
                    }
                }
            }

            //////////////////////////////////////////////////////////////////////////
            //we only want to see assets that match all the match filters
            if (!m_assetMatchFilters.empty())
            {
                if (m_assetMatchFiltersOperator == AzToolsFramework::FilterOperatorType::And)
                {
                    for (const auto& item : m_assetMatchFilters)
                    {
                        if (!entry->Match(item.c_str()))
                        {
                            return false;
                        }
                    }
                }
                else if (m_assetMatchFiltersOperator == AzToolsFramework::FilterOperatorType::Or)
                {
                    for (const auto& item : m_assetMatchFilters)
                    {
                        if (entry->Match(item.c_str()))
                        {
                            return true;
                        }
                    }
                    return false;
                }
            }
            ////////////////////////////////////////////////////////////////////////

            return true;
        }

        bool SortFilterProxyModel::filterAcceptsColumn(int source_column, const QModelIndex& source_parent) const
        {
            (void)source_parent;

            //if the column is in the set we want to show it
            return m_showColumn.find(static_cast<AssetBrowserEntry::Column>(source_column)) != m_showColumn.end();
        }

        bool SortFilterProxyModel::lessThan(const QModelIndex& source_left, const QModelIndex& source_right) const
        {
            if (source_left.column() == source_right.column())
            {
                QVariant leftData = sourceModel()->data(source_left);
                QVariant rightData = sourceModel()->data(source_right);
                if ((leftData.type() == QVariant::String) &&
                    (rightData.type() == QVariant::String))
                {
                    QString leftString = leftData.toString();
                    QString rightString = rightData.toString();
                    return QString::compare(leftString, rightString, Qt::CaseInsensitive) > 0;
                }
            }
            return QSortFilterProxyModel::lessThan(source_left, source_right);
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework// namespace AssetBrowser

#include <AssetBrowser/moc_SortFilterProxyModel.cpp>
