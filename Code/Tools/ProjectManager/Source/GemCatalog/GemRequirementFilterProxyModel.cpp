/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GemCatalog/GemRequirementFilterProxyModel.h>
#include <GemCatalog/GemModel.h>

#include <QItemSelectionModel>

namespace O3DE::ProjectManager
{
    GemRequirementFilterProxyModel::GemRequirementFilterProxyModel(GemModel* sourceModel, QObject* parent)
        : QSortFilterProxyModel(parent)
    {
        setSourceModel(sourceModel);
        m_selectionProxyModel = new AzQtComponents::SelectionProxyModel(sourceModel->GetSelectionModel(), this, parent);
    }

    bool GemRequirementFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
    {
        // Do not use sourceParent->child because an invalid parent does not produce valid children (which our index function does)
        QModelIndex sourceIndex = sourceModel()->index(sourceRow, 0, sourceParent);
        const bool hasNewVersion = !GemModel::GetNewVersion(sourceIndex).isEmpty();
        return GemModel::IsAdded(sourceIndex) && GemModel::HasRequirement(sourceIndex) && (!GemModel::WasPreviouslyAdded(sourceIndex) || hasNewVersion);
    }

} // namespace O3DE::ProjectManager
