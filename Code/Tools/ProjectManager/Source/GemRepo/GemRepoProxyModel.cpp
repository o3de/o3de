/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GemRepo/GemRepoProxyModel.h>
#include <GemRepo/GemRepoModel.h>
#include <ProjectManagerDefs.h>

namespace O3DE::ProjectManager
{
    GemRepoProxyModel::GemRepoProxyModel(QObject* parent)
        : QSortFilterProxyModel(parent)
    {
    }

    bool GemRepoProxyModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
    {
        GemRepoModel* model = qobject_cast<GemRepoModel*>(sourceModel());
        if (model)
        {
            const QString leftUri = model->GetRepoUri(left);
            if (leftUri.compare(CanonicalRepoUri, Qt::CaseInsensitive) == 0)
            {
                // make sure canonical is at top
                return true;
            }

            const QString rightUri = model->GetRepoUri(right);
            if (rightUri.compare(CanonicalRepoUri, Qt::CaseInsensitive) == 0)
            {
                // make sure canonical is at top
                return false;
            }

            return model->GetName(left).compare(model->GetName(right), Qt::CaseInsensitive) < 0;
        }
        return true;
    }

} // namespace O3DE::ProjectManager
