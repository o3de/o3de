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

#ifndef FIND_ENTITY_SORT_FILTER_PROXY_MODEL_H
#define FIND_ENTITY_SORT_FILTER_PROXY_MODEL_H

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <QtCore/QSortFilterProxyModel>
#endif

#pragma once

/*!
    * Enables the Find Entity widget to filter entries based on search string.
    */
class FindEntitySortFilterProxyModel
    : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    AZ_CLASS_ALLOCATOR(FindEntitySortFilterProxyModel, AZ::SystemAllocator, 0);

    FindEntitySortFilterProxyModel(QObject* pParent = nullptr);

    void UpdateFilter();

    // Qt overrides
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

private:

};

#endif
