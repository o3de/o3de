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
