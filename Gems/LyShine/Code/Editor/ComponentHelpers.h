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

#include "ComponentAssetHelpers.h"

namespace ComponentHelpers
{
    using EntityComponentPair = AZStd::pair<AZ::EntityId, AZ::TypeId>;

    QList<QAction*> CreateAddComponentActions(HierarchyWidget* hierarchy,
        QTreeWidgetItemRawPtrQList& selectedItems,
        QWidget* parent);
    QAction* CreateRemoveComponentsAction(QWidget* parent);
    void UpdateRemoveComponentsAction(QAction* action);
    QAction* CreateCutComponentsAction(QWidget* parent);
    void UpdateCutComponentsAction(QAction* action);
    QAction* CreateCopyComponentsAction(QWidget* parent);
    void UpdateCopyComponentsAction(QAction* action);
    QAction* CreatePasteComponentsAction(QWidget* parent);
    void UpdatePasteComponentsAction(QAction* action);

    //! Returns whether a list of components can be added to all currently selected entities
    bool CanAddComponentsToSelectedEntities(const AZStd::vector<AZ::TypeId>& componentTypes,
        EntityComponentPair* firstIncompatibleComponentType = nullptr);

    //! Add a list of components to all currently selected entities and assign the components a primary asset
    void AddComponentsWithAssetToSelectedEntities(const ComponentAssetHelpers::ComponentAssetPairs& componentAssetPairs);

    //! Returns whether a list of components can be added to a specified entity
    bool CanAddComponentsToEntity(const AZStd::vector<AZ::TypeId>& componentTypes,
        const AZ::EntityId& entityId,
        EntityComponentPair* firstIncompatibleComponentType = nullptr);

    //! Add a list of components to a specified entity and assign the components a primary asset
    void AddComponentsWithAssetToEntity(const ComponentAssetHelpers::ComponentAssetPairs& componentAssetPairs, const AZ::EntityId& entityId);

    struct ComponentTypeData
    {
        const AZ::SerializeContext::ClassData* classData;
        bool isLyShineComponent;
    };
    AZStd::vector<ComponentTypeData> GetAllComponentTypesThatCanAppearInAddComponentMenu();

}   // namespace ComponentHelpers
