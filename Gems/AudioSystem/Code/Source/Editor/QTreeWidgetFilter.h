/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <AzCore/std/containers/vector.h>

class QTreeWidget;
class QTreeWidgetItem;

//-----------------------------------------------------------------------------------------------//
struct ITreeWidgetItemFilter
{
    virtual ~ITreeWidgetItemFilter() {}
    virtual bool IsItemValid(QTreeWidgetItem* pItem) = 0;
};

//-----------------------------------------------------------------------------------------------//
class QTreeWidgetFilter
{
public:
    QTreeWidgetFilter();
    void SetTree(QTreeWidget* pTreeWidget);
    void AddFilter(ITreeWidgetItemFilter* filter);
    void ApplyFilter();

private:
    bool IsItemValid(QTreeWidgetItem* pItem);

    QTreeWidget* m_pTreeWidget;
    AZStd::vector<ITreeWidgetItemFilter*> m_filters;
};
