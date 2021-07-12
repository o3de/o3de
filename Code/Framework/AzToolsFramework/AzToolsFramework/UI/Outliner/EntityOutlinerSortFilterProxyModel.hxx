/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef ENTITYOUTLINER_SORT_FILTER_PROXY_MODEL_H
#define ENTITYOUTLINER_SORT_FILTER_PROXY_MODEL_H

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <QtCore/QSortFilterProxyModel>
#include <AzCore/Memory/SystemAllocator.h>
#endif

#pragma once

namespace AzToolsFramework
{

    /*!
        * Enables the Outliner to filter entries based on search string.
        * Enables the Outliner to do custom sorting on entries.
        */
    class EntityOutlinerSortFilterProxyModel
        : public QSortFilterProxyModel
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(EntityOutlinerSortFilterProxyModel, AZ::SystemAllocator, 0);

        EntityOutlinerSortFilterProxyModel(QObject* pParent = nullptr);

        void UpdateFilter();

        // Qt overrides
        bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;
        bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;
        void sort(int column, Qt::SortOrder order) override;

    private:
        QString m_filterName;
    };

}

#endif
