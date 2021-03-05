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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

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
