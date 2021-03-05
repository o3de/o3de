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