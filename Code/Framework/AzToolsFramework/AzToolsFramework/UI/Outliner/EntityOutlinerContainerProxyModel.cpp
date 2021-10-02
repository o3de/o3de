/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EntityOutlinerContainerProxyModel.hxx"

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/Entity.h>

#include <AzToolsFramework/ContainerEntity/ContainerEntityInterface.h>

#include "EntityOutlinerListModel.hxx"

namespace AzToolsFramework
{

    EntityOutlinerContainerProxyModel::EntityOutlinerContainerProxyModel(QObject* parent)
        : QSortFilterProxyModel(parent)
    {
        m_containerEntityInterface = AZ::Interface<ContainerEntityInterface>::Get();
        AZ_Assert(
            m_containerEntityInterface != nullptr, "EntityOutlinerContainerProxyModel requires a ContainerEntityInterface instance on construction.");
    }

    QVariant EntityOutlinerContainerProxyModel::data(const QModelIndex& index, int role) const
    {
        QModelIndex sourceIndex = mapToSource(index);
        return sourceModel()->data(sourceIndex, role);
    }

    bool EntityOutlinerContainerProxyModel::filterAcceptsRow([[maybe_unused]] int sourceRow, const QModelIndex& sourceParent) const
    {
        // Retrieve the entityId of the parent entity

        // TODO - retrieve this once on constructor and assert if source model isn't EntityOutlinerListModel
        EntityOutlinerListModel* listModel = qobject_cast<EntityOutlinerListModel*>(sourceModel());
        if (!listModel)
        {
            return false;
        }

        AZ::EntityId parentEntityId = listModel->GetEntityFromIndex(sourceParent);

        return m_containerEntityInterface->IsContainerOpen(parentEntityId);
    }

}

#include <UI/Outliner/moc_EntityOutlinerContainerProxyModel.cpp>

