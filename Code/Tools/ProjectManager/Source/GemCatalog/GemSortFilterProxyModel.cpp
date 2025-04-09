/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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

        const GemInfo& gemInfo = m_sourceModel->GetGemInfo(sourceIndex);
        // Search Bar
        if (!m_searchString.isEmpty() &&
            !gemInfo.m_displayName.contains(m_searchString, Qt::CaseInsensitive) &&
            !gemInfo.m_name.contains(m_searchString, Qt::CaseInsensitive) &&
            !gemInfo.m_origin.contains(m_searchString, Qt::CaseInsensitive) &&
            !gemInfo.m_summary.contains(m_searchString, Qt::CaseInsensitive))
        {
            bool foundFeature = false;
            for (const QString& feature : gemInfo.m_features)
            {
                if (feature.contains(m_searchString, Qt::CaseInsensitive))
                {
                    foundFeature = true;
                    break;
                }
            }

            if (!foundFeature)
            {
                return false;
            }
        }

        // Gem selected
        if (m_gemSelectedFilter == GemSelected::Selected)
        {
            if (!GemModel::NeedsToBeAdded(sourceIndex, true))
            {
                return false;
            }
        }
        // Gem unselected
        else if (m_gemSelectedFilter == GemSelected::Unselected)
        {
            if (!GemModel::NeedsToBeRemoved(sourceIndex, true))
            {
                return false;
            }
        }
        // Gem selected or unselected
        else if (m_gemSelectedFilter == GemSelected::Both)
        {
            if (!GemModel::NeedsToBeAdded(sourceIndex, true)  && !GemModel::NeedsToBeRemoved(sourceIndex, true))
            {
                return false;
            }
        }

        // Update available
        if (m_updateAvailableFilter && !GemModel::HasUpdates(sourceIndex, m_compatibleOnlyFilter))
        {
            return false;
        }

        // Compatible
        if (m_compatibleOnlyFilter && !GemModel::IsCompatible(sourceIndex))
        {
            return false;
        }

        // Missing
        if (m_gemMissingFilter && !GemModel::IsAddedMissing(sourceIndex))
        {
            return false;
        }

        // Gem enabled
        if (m_gemActiveFilter != GemActive::NoFilter)
        {
            const GemActive sourceGemStatus = static_cast<GemActive>(GemModel::IsAdded(sourceIndex) || GemModel::IsAddedDependency(sourceIndex));
            if (m_gemActiveFilter != sourceGemStatus)
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
                    if ((gemInfo.m_gemOrigin == filteredGemOrigin))
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
                    if ((gemInfo.m_platforms & filteredPlatform))
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
                    if ((gemInfo.m_types & filteredType))
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
            for (const QString& feature : gemInfo.m_features)
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

    QString GemSortFilterProxyModel::GetGemSelectedString(GemSelected status)
    {
        switch (status)
        {
        case GemSelected::Unselected:
            return "Unselected";
        case GemSelected::Selected:
            return "Selected";
        default:
            return "<Unknown Selection Status>";
        }
    }

    QString GemSortFilterProxyModel::GetGemActiveString(GemActive status)
    {
        switch (status)
        {
        case GemActive::Inactive:
            return "Inactive";
        case GemActive::Active:
            return "Active";
        default:
            return "<Unknown Active Status>";
        }
    }
    void GemSortFilterProxyModel::InvalidateFilter()
    {
        invalidate();
        emit OnInvalidated();
    }

    void GemSortFilterProxyModel::SetOriginFilterFlag(int flag, bool set)
    {
        auto origin = static_cast<GemInfo::GemOrigin>(flag);
        if (set)
        {
            m_gemOriginFilter |= origin;
        }
        else
        {
            m_gemOriginFilter &= ~origin;
        }
        InvalidateFilter();
    }

    void GemSortFilterProxyModel::SetTypeFilterFlag(int flag, bool set)
    {
        auto type = static_cast<GemInfo::Type>(flag);
        if (set)
        {
            m_typeFilter |= type;
        }
        else
        {
            m_typeFilter &= ~type;
        }
        InvalidateFilter();
    }

    void GemSortFilterProxyModel::SetPlatformFilterFlag(int flag, bool set)
    {
        auto platform = static_cast<GemInfo::Platform>(flag);
        if (set)
        {
            m_platformFilter |= platform;
        }
        else
        {
            m_platformFilter &= ~platform;
        }
        InvalidateFilter();
    }

    void GemSortFilterProxyModel::ResetFilters(bool clearSearchString)
    {
        if (clearSearchString)
        {
            m_searchString.clear();
        }
        m_gemSelectedFilter = GemSelected::NoFilter;
        m_gemActiveFilter = GemActive::NoFilter;
        m_gemOriginFilter = {};
        m_platformFilter = {};
        m_typeFilter = {};
        m_featureFilter = {};
        m_updateAvailableFilter = false;
        m_compatibleOnlyFilter = true;

        InvalidateFilter();
    }

    void GemSortFilterProxyModel::SetUpdateAvailable(bool showGemsWithUpdates)
    {
        m_updateAvailableFilter = showGemsWithUpdates;
        InvalidateFilter();
    }

    void GemSortFilterProxyModel::SetCompatibleFilterFlag(bool showCompatibleGemsOnly)
    {
        m_compatibleOnlyFilter = showCompatibleGemsOnly;
        InvalidateFilter();
    }
} // namespace O3DE::ProjectManager
