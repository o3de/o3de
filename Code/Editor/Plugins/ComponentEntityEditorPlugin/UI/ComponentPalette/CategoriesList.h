/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include "ComponentDataModel.h"
#include <QTreeWidget>
#endif

//! ComponentCategoryList
//! Provides a list of all reflected categories that users can select for quick
//! filtering the filtered component list.
class ComponentCategoryList : public QTreeWidget
{
    Q_OBJECT

public:

    AZ_CLASS_ALLOCATOR(ComponentCategoryList, AZ::SystemAllocator, 0);

    explicit ComponentCategoryList(QWidget* parent = nullptr);

    void Init();

Q_SIGNALS:
    void OnCategoryChange(const char* category);

protected:

    // Will emit OnCategoryChange signal
    void OnItemClicked(QTreeWidgetItem* item, int column);

};
