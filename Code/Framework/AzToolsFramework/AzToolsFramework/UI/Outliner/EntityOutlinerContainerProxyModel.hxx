/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)

#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <QSortFilterProxyModel>

#endif

namespace AzToolsFramework
{
    class ContainerEntityInterface;

    /*!
     * Enables the Outliner to filter entries based on entity container state.
     */
    class EntityOutlinerContainerProxyModel
        : public QSortFilterProxyModel
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(EntityOutlinerContainerProxyModel, AZ::SystemAllocator, 0);

        EntityOutlinerContainerProxyModel(QObject* parent = nullptr);

        // Qt overrides
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

    private:
        ContainerEntityInterface* m_containerEntityInterface = nullptr;
    };

}
