/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "AddToLayerMenu.h"

#include <AzCore/Component/TransformBus.h>
#include <AzCore/std/containers/stack.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/containers/stack.h>
#include <AzQtComponents/AzQtComponentsAPI.h>
#include <AzQtComponents/Components/Style.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 'QLayoutItem::align': class 'QFlags<Qt::AlignmentFlag>' needs to have dll-interface to be used by clients of class 'QLayoutItem'
#include <QHBoxLayout>
#include <QMenu>
#include <QWidgetAction>
AZ_POP_DISABLE_WARNING
#include <QtWidgets/QLabel>

namespace AzToolsFramework
{
    struct LayerMenuRow;
    typedef AZStd::unordered_map<AZ::EntityId, AZStd::shared_ptr<LayerMenuRow>> EntityIdToLayerMenuRow;

    struct LayerMenuRow
    {
    public:
        LayerMenuRow() { }

        QString m_rowLabel;
        EntityIdToLayerMenuRow m_rowChildren;
    };

    void AssignParentToAllEntities(
        const AZ::EntityId& newParent,
        const AzToolsFramework::EntityIdSet &entities)
    {
        for (const AZ::EntityId& entityToReparent : entities)
        {
            AZ::TransformBus::Event(
                entityToReparent,
                &AZ::TransformBus::Events::SetParent,
                newParent);
        }
    }

    void BuildAddToLayerRows(
        QMenu* assignToLayerMenu,
        const AzToolsFramework::EntityIdSet &entitySelectionWithFlatHierarchy,
        bool isHierarchyActive,
        const EntityIdToLayerMenuRow& layerRows,
        int indent)
    {
        for (auto& layerRow : layerRows)
        {
            AZ::EntityId currentLayerId(layerRow.first);
            QColor layerColor;
            AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
                layerColor,
                currentLayerId,
                &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::GetLayerColor);

            QWidgetAction* assignToLayerAction = new QWidgetAction(assignToLayerMenu);
            QWidget* assignToLayerWidget = new QWidget(assignToLayerMenu);
            // Special behavior and properties are defined in NewEditorStyleSheet.qss based on this name.
            assignToLayerWidget->setObjectName("LayerHierarchyMenuItem");
            // Add class to fix hover state styling for WidgetAction
            AzQtComponents::Style::addClass(assignToLayerWidget, "WidgetAction");
            QHBoxLayout* assignToLayerLayout = new QHBoxLayout(assignToLayerWidget);
            assignToLayerAction->setDefaultWidget(assignToLayerWidget);
            
            // If the menu is too thin, it feels awkward to use.
            assignToLayerWidget->setMinimumWidth(200);

            const int layerIconSize = 8;

            QPixmap layerPixmap(layerIconSize, layerIconSize);
            layerPixmap.fill(layerColor);

            // If this layer is a child of another layer, then indent it in under the parent
            if (indent > 0)
            {
                const int indentPerLevel = 20;
                const int spacingWidth = indent * indentPerLevel;

                QLabel* indentLabel = new QLabel(assignToLayerMenu);
                indentLabel->setFixedWidth(spacingWidth);
                assignToLayerLayout->addWidget(indentLabel);
            }

            // Create a square icon of the layer's color.
            QLabel* iconLabel = new QLabel(assignToLayerMenu);
            iconLabel->setPixmap(layerPixmap);
            iconLabel->setFixedSize(QSize(layerIconSize, layerIconSize));
            assignToLayerLayout->addWidget(iconLabel);

            QLabel* iconPaddingLabel = new QLabel(assignToLayerMenu);
            const int layerIconPadding = 4;
            iconPaddingLabel->setFixedWidth(layerIconPadding);
            assignToLayerLayout->addWidget(iconPaddingLabel);

            // Use just the layer's entity name, and not full file path, to keep the menu simple.
            QLabel* layerLabel = new QLabel(layerRow.second->m_rowLabel, assignToLayerMenu);
            assignToLayerLayout->addWidget(layerLabel);

            // Once a parent layer is an invalid target for this selection, all children will be invalid, too.
            // Siblings may still be fine, which is why a helper bool is used here instead of just flipping isHierarchyACtive.
            bool childrenHierarchyActive = isHierarchyActive;
            // Check if it's safe to re-assign the selection to this entity.
            if (!childrenHierarchyActive)
            {
                assignToLayerWidget->setEnabled(false);
                assignToLayerWidget->setToolTip(QObject::tr("The selected layer can't be moved to one of its child layers."));
            }
            else if (entitySelectionWithFlatHierarchy.find(currentLayerId) != entitySelectionWithFlatHierarchy.end())
            {
                assignToLayerWidget->setEnabled(false);
                assignToLayerWidget->setToolTip(QObject::tr("The selected layer can't be moved to itself."));
                childrenHierarchyActive = false;
            }
            else
            {
                assignToLayerWidget->setToolTip(QObject::tr("Move the selection to this layer."));
                QObject::connect(assignToLayerAction, &QAction::triggered,
                    [entitySelectionWithFlatHierarchy, currentLayerId]
                {
                    AssignParentToAllEntities(currentLayerId, entitySelectionWithFlatHierarchy);
                });
            }

            assignToLayerMenu->addAction(assignToLayerAction);

            // Build the add to layer rows for all of this layer's children.
            BuildAddToLayerRows(
                assignToLayerMenu,
                entitySelectionWithFlatHierarchy,
                childrenHierarchyActive,
                layerRow.second->m_rowChildren,
                indent + 1);
        }
    }

    void BuildLayerAcenstry(AZStd::stack<AZ::EntityId>& layerAncestry, AZ::EntityId layerEntityId)
    {
        do
        {
            if (layerEntityId.IsValid())
            {
                layerAncestry.push(layerEntityId);
            }
            AZ::EntityId parentId;
            AZ::TransformBus::EventResult(
                parentId,
                layerEntityId,
                &AZ::TransformBus::Events::GetParentId);
            if (layerEntityId == parentId)
            {
                break;
            }
            layerEntityId = parentId;

        } while (layerEntityId.IsValid());
    }

    void BuildNewLayerRow(QMenu* assignToLayerMenu,
        const AzToolsFramework::EntityIdSet& entitySelectionWithFlatHierarchy,
        NewLayerFunction newLayerFunction)
    {
        QWidgetAction* assignToLayerAction = new QWidgetAction(assignToLayerMenu);
        QWidget* assignToLayerWidget = new QWidget(assignToLayerMenu);
        // Special behavior and properties are defined in NewEditorStyleSheet.qss based on this name.
        assignToLayerWidget->setObjectName("LayerHierarchyMenuItem");
        // Add class to fix hover state styling for WidgetAction
        AzQtComponents::Style::addClass(assignToLayerWidget, "WidgetAction");
        QHBoxLayout* assignToLayerLayout = new QHBoxLayout(assignToLayerWidget);
        assignToLayerAction->setDefaultWidget(assignToLayerWidget);

        QLabel* layerLabel = new QLabel(QObject::tr("New"), assignToLayerMenu);
        assignToLayerLayout->addWidget(layerLabel);

        QObject::connect(assignToLayerAction, &QAction::triggered,
            [entitySelectionWithFlatHierarchy, newLayerFunction]
        {
            AZ::EntityId newLayerId(newLayerFunction());
            AssignParentToAllEntities(newLayerId, entitySelectionWithFlatHierarchy);
        });
        assignToLayerMenu->addAction(assignToLayerAction);
    }

    void SetupAddToLayerMenu(
        QMenu* parentMenu,
        const AzToolsFramework::EntityIdSet& entitySelectionWithFlatHierarchy,
        NewLayerFunction newLayerFunction)
    {
        // If nothing is selected, there is no reason to make the menu.
        if (entitySelectionWithFlatHierarchy.size() == 0)
        {
            return;
        }

        QMenu* assignToLayerMenu = new QMenu(parentMenu);
        assignToLayerMenu->setTitle(QObject::tr("Assign to layer"));
        parentMenu->addMenu(assignToLayerMenu);

        BuildNewLayerRow(assignToLayerMenu, entitySelectionWithFlatHierarchy, newLayerFunction);
        assignToLayerMenu->addSeparator();

        AZStd::vector<AZ::Entity*> editorEntities;
        AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
            &AzToolsFramework::EditorEntityContextRequestBus::Events::GetLooseEditorEntities,
            editorEntities);

        // Construct the hierarchy of layer rows, so child layers can appear under their parents correctly.
        EntityIdToLayerMenuRow layerRootRows;

        for (AZ::Entity* entity : editorEntities)
        {
            bool isLayer = false;
            AzToolsFramework::Layers::EditorLayerComponentRequestBus::EventResult(
                isLayer,
                entity->GetId(),
                &AzToolsFramework::Layers::EditorLayerComponentRequestBus::Events::HasLayer);
            if (!isLayer)
            {
                continue;
            }

            AZStd::stack<AZ::EntityId> layerAncestry;
            BuildLayerAcenstry(layerAncestry, entity->GetId());

            EntityIdToLayerMenuRow* currentRow = &layerRootRows;
            while (!layerAncestry.empty())
            {
                AZ::EntityId ancestorId(layerAncestry.top());
                layerAncestry.pop();

                EntityIdToLayerMenuRow::iterator currentAncestorRow = currentRow->find(ancestorId);
                if (currentAncestorRow == currentRow->end())
                {
                    currentRow->insert(AZStd::pair<AZ::EntityId, AZStd::shared_ptr<LayerMenuRow>>(ancestorId, AZStd::make_shared<LayerMenuRow>()));
                    currentAncestorRow = currentRow->find(ancestorId);
                }
                currentRow = &currentAncestorRow->second->m_rowChildren;
                if (currentAncestorRow->first == entity->GetId())
                {
                    currentAncestorRow->second->m_rowLabel = entity->GetName().c_str();
                }

            }

        }

        BuildAddToLayerRows(
            assignToLayerMenu,
            entitySelectionWithFlatHierarchy,
            true,
            layerRootRows,
            0);

    }

}
