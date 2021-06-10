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
#include <AzQtComponents/Utilities/SelectionProxyModel.h>
#include <GemCatalog/GemModel.h>
#include <QtCore/QSortFilterProxyModel>
#include <QSet>
#endif

QT_FORWARD_DECLARE_CLASS(QItemSelectionModel)

namespace O3DE::ProjectManager
{
    class GemSortFilterProxyModel
        : public QSortFilterProxyModel
    {
        Q_OBJECT // AUTOMOC

    public:
        GemSortFilterProxyModel(GemModel* sourceModel, QObject* parent = nullptr);

        bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

        GemModel* GetSourceModel() const { return m_sourceModel; } 
        AzQtComponents::SelectionProxyModel* GetSelectionModel() const { return m_selectionProxyModel; }

        void SetSearchString(const QString& searchString) { m_searchString = searchString; InvalidateFilter(); }

        GemInfo::GemOrigins GetGemOrigins() const { return m_gemOriginFilter; }
        void SetGemOrigins(const GemInfo::GemOrigins& gemOrigins) { m_gemOriginFilter = gemOrigins; InvalidateFilter(); }

        GemInfo::Platforms GetPlatforms() const { return m_platformFilter; }
        void SetPlatforms(const GemInfo::Platforms& platforms) { m_platformFilter = platforms; InvalidateFilter(); }

        GemInfo::Types GetTypes() const { return m_typeFilter; }
        void SetTypes(const GemInfo::Types& types) { m_typeFilter = types; InvalidateFilter(); }

        const QSet<QString>& GetFeatures() const { return m_featureFilter; }
        void SetFeatures(const QSet<QString>& features) { m_featureFilter = features; InvalidateFilter(); }

        void InvalidateFilter();
        void ResetFilters();

    signals:
        void OnInvalidated();

    private:
        GemModel* m_sourceModel = nullptr;
        AzQtComponents::SelectionProxyModel* m_selectionProxyModel = nullptr;

        QString m_searchString;
        GemInfo::GemOrigins m_gemOriginFilter = {};
        GemInfo::Platforms m_platformFilter = {};
        GemInfo::Types m_typeFilter = {};
        QSet<QString> m_featureFilter;
    };
} // namespace O3DE::ProjectManager
