/*
 * Copyright (c) Contributors to the Open 3D Engine Project
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "EditorCommon.h"

namespace SliceMenuHelpers
{
    void CreateInstantiateSliceMenu(HierarchyWidget* hierarchy,
        QTreeWidgetItemRawPtrQList& selectedItems,
        QMenu* parent,
        bool addAtRoot,
        const AZ::Vector2& viewportPosition);

}   // namespace SliceMenuHelpers
