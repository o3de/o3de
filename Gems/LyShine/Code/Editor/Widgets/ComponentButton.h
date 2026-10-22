/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "Widgets/HierarchyWidget/HierarchyItem.h"

#include <QPushButton>

class HierarchyWidget;

class ComponentButton : public QPushButton
{
    Q_OBJECT

public:
    ComponentButton(HierarchyWidget* hierarchy, QWidget* parent = nullptr);

private slots:

    void UserSelectionChanged(HierarchyItemRawPtrList* items);
};
