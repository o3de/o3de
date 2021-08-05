/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <QTreeView>

namespace EMotionFX
{
    // A QTreeView subclass that emits selectionChanged whenever it gains focus
    class ReselectingTreeView
        : public QTreeView
    {
    public:
        void focusInEvent(QFocusEvent* event) override;

        void RecursiveGetAllChildren(const QModelIndex& index, QModelIndexList& outIndicies);
    };
} // namespace EMotionFX
