/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/Utilities/SelectionProxyModel.h>
#include <QtCore/QSortFilterProxyModel>
#endif

QT_FORWARD_DECLARE_CLASS(QItemSelectionModel)

namespace O3DE::ProjectManager
{
    QT_FORWARD_DECLARE_CLASS(GemModel)

    class GemRequirementFilterProxyModel
        : public QSortFilterProxyModel
    {
        Q_OBJECT // AUTOMOC

    public:
        GemRequirementFilterProxyModel(GemModel* sourceModel, const QVector<QModelIndex>& addedGems, QObject* parent = nullptr);

        AzQtComponents::SelectionProxyModel* GetSelectionModel() const { return m_selectionProxyModel; }

        bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

    private:
        GemModel* m_sourceModel = nullptr;
        AzQtComponents::SelectionProxyModel* m_selectionProxyModel = nullptr;

        QVector<QModelIndex> m_addedGems;
    };
} // namespace O3DE::ProjectManager
