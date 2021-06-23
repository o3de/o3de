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
