/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/ReselectingTreeView.h>
#include <QItemSelectionModel>

namespace EMotionFX
{
    void ReselectingTreeView::focusInEvent(QFocusEvent* event)
    {
        emit selectionModel()->selectionChanged({}, {});
        QTreeView::focusInEvent(event);
    }

    void ReselectingTreeView::RecursiveGetAllChildren(const QModelIndex& index, QModelIndexList& outIndicies)
    {
        outIndicies.push_back(index);
        for (int i = 0; i < model()->rowCount(index); ++i)
        {
            RecursiveGetAllChildren(model()->index(i, 0, index), outIndicies);
        }
    }

} // namespace EMotionFX
