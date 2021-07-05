/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GemCatalog/GemSortFilterProxyModel.h>
#include <QItemSelectionModel>

namespace O3DE::ProjectManager
{
    GemSortFilterProxyModel::GemSortFilterProxyModel(GemModel* sourceModel, QObject* parent)
        : QSortFilterProxyModel(parent)
        , m_sourceModel(sourceModel)
    {
        setSourceModel(sourceModel);
        m_selectionProxyModel = new AzQtComponents::SelectionProxyModel(sourceModel->GetSelectionModel(), this, parent);
    }

    bool GemSortFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
    {
        // Do not use sourceParent->child because an invalid parent does not produce valid children (which our index function does)
        QModelIndex sourceIndex = sourceModel()->index(sourceRow, 0, sourceParent);
        if (!sourceIndex.isValid())
        {
            return false;
        }

        if (!m_sourceModel->GetName(sourceIndex).contains(m_searchString, Qt::CaseInsensitive))
        {
            return false;
        }

        // Gem status
        if (m_gemStatusFilter != GemStatus::NoFilter)
        {
            const GemStatus sourceGemStatus = static_cast<GemStatus>(GemModel::IsAdded(sourceIndex));
            if (m_gemStatusFilter != sourceGemStatus)
            {
                return false;
            }
        }

        // Gem origins
        if (m_gemOriginFilter)
        {
            bool supportsAnyFilteredGemOrigin = false;
            for (int i = 0; i < GemInfo::NumGemOrigins; ++i)
            {
                const GemInfo::GemOrigin filteredGemOrigin = static_cast<GemInfo::GemOrigin>(1 << i);
                if (m_gemOriginFilter & filteredGemOrigin)
                {
                    if ((GemModel::GetGemOrigin(sourceIndex) == filteredGemOrigin))
                    {
                        supportsAnyFilteredGemOrigin = true;
                        break;
                    }
                }
            }
            if (!supportsAnyFilteredGemOrigin)
            {
                return false;
            }
        }

        // Platform
        if (m_platformFilter)
        {
            bool supportsAnyFilteredPlatform = false;
            for (int i = 0; i < GemInfo::NumPlatforms; ++i)
            {
                const GemInfo::Platform filteredPlatform = static_cast<GemInfo::Platform>(1 << i);
                if (m_platformFilter & filteredPlatform)
                {
                    if ((GemModel::GetPlatforms(sourceIndex) & filteredPlatform))
                    {
                        supportsAnyFilteredPlatform = true;
                        break;
                    }
                }
            }
            if (!supportsAnyFilteredPlatform)
            {
                return false;
            }
        }

        // Types (Asset, Code, Tool)
        if (m_typeFilter)
        {
            bool supportsAnyFilteredType = false;
            for (int i = 0; i < GemInfo::NumTypes; ++i)
            {
                const GemInfo::Type filteredType = static_cast<GemInfo::Type>(1 << i);
                if (m_typeFilter & filteredType)
                {
                    if ((GemModel::GetTypes(sourceIndex) & filteredType))
                    {
                        supportsAnyFilteredType = true;
                        break;
                    }
                }
            }
            if (!supportsAnyFilteredType)
            {
                return false;
            }
        }

        // Features
        if (!m_featureFilter.isEmpty())
        {
            bool containsFilterFeature = false;
            const QStringList features = m_sourceModel->GetFeatures(sourceIndex);
            for (const QString& feature : features)
            {
                if (m_featureFilter.contains(feature))
                {
                    containsFilterFeature = true;
                    break;
                }
            }
            if (!containsFilterFeature)
            {
                return false;
            }
        }

        return true;
    }

    QString GemSortFilterProxyModel::GetGemStatusString(GemStatus status)
    {
        switch (status)
        {
        case Unselected:
            return "Unselected";
        case Selected:
            return "Selected";
        default:
            return "<Unknown Gem Status>";
        }
    }

    void GemSortFilterProxyModel::InvalidateFilter()
    {
        invalidate();
        emit OnInvalidated();
    }

    void GemSortFilterProxyModel::ResetFilters()
    {
        m_searchString.clear();
        m_gemOriginFilter = {};
        m_platformFilter = {};
        m_typeFilter = {};
        m_featureFilter = {};

        InvalidateFilter();
    }
} // namespace O3DE::ProjectManager
