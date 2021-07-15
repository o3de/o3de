/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UiCanvasEditor_precompiled.h"

#include "EditorCommon.h"
#include <AzToolsFramework/Slice/SliceUtilities.h>
#include <AzToolsFramework/ToolsComponents/EditorOnlyEntityComponentBus.h>
#include "SliceMenuHelpers.h"
#include "QtHelpers.h"

// Define for enabling/disabling the UI Slice system
#define ENABLE_UI_SLICE_MENU_ITEMS 1

HierarchyMenu::HierarchyMenu(HierarchyWidget* hierarchy,
    size_t showMask,
    bool addMenuForNewElement,
    const QPoint* optionalPos)
    : QMenu()
{
    setStyleSheet(UICANVASEDITOR_QMENU_ITEM_DISABLED_STYLESHEET);

    QTreeWidgetItemRawPtrQList selectedItems = hierarchy->selectedItems();


    if (showMask & (Show::kNew_EmptyElement | Show::kNew_EmptyElementAtRoot))
    {
        QMenu* menu = (addMenuForNewElement ? addMenu("&New...") : this);

        if (showMask & (Show::kNew_EmptyElement | Show::kNew_EmptyElementAtRoot))
        {
            New_EmptyElement(hierarchy, selectedItems, menu, (showMask & Show::kNew_EmptyElementAtRoot), optionalPos);
        }

        if (showMask & Show::kNew_InstantiateSlice | Show::kNew_InstantiateSliceAtRoot)
        {
            New_ElementFromSlice(hierarchy, selectedItems, menu, (showMask & Show::kNew_InstantiateSliceAtRoot), optionalPos);
        }
    }

    if (showMask & (Show::kNewSlice | Show::kPushToSlice))
    {
        SliceMenuItems(hierarchy, selectedItems, showMask);
    }

    addSeparator();

    if (showMask & Show::kCutCopyPaste)
    {
        CutCopyPaste(hierarchy, selectedItems);
    }

    if (showMask & Show::kDeleteElement)
    {
        DeleteElement(hierarchy, selectedItems);
    }

    addSeparator();

    if (showMask & Show::kAddComponents)
    {
        AddComponents(hierarchy, selectedItems);
    }

    addSeparator();

    if (showMask & Show::kFindElements)
    {
        FindElements(hierarchy, selectedItems);
    }

    addSeparator();

    if (showMask & Show::kEditorOnly)
    {
        EditorOnly(hierarchy, selectedItems);
    }
}

void HierarchyMenu::CutCopyPaste(HierarchyWidget* hierarchy,
    QTreeWidgetItemRawPtrQList& selectedItems)
{
    QAction* action;

    bool itemsAreSelected = (!selectedItems.isEmpty());

    // Cut element.
    {
        action = new QAction("Cut", this);
        action->setShortcut(QKeySequence::Cut);
        action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        QObject::connect(action,
            &QAction::triggered,
            hierarchy,
            [ hierarchy ]([[maybe_unused]] bool checked)
            {
                QMetaObject::invokeMethod(hierarchy, "Cut", Qt::QueuedConnection);
            });
        addAction(action);

        if (!itemsAreSelected)
        {
            // Nothing has been selected.
            // We want the menu to be visible, but disabled.
            action->setEnabled(false);
        }
    }

    // Copy element.
    {
        action = new QAction("Copy", this);
        action->setShortcut(QKeySequence::Copy);
        action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        QObject::connect(action,
            &QAction::triggered,
            hierarchy,
            [ hierarchy ]([[maybe_unused]] bool checked)
            {
                QMetaObject::invokeMethod(hierarchy, "Copy", Qt::QueuedConnection);
            });
        addAction(action);

        if (!itemsAreSelected)
        {
            // Nothing has been selected.
            // We want the menu to be visible, but disabled.
            action->setEnabled(false);
        }
    }

    bool thereIsContentInTheClipboard = ClipboardContainsOurDataType();

    // Paste element.
    {
        action = new QAction(QIcon(":/Icons/Eye_Open.png"), (itemsAreSelected ? "Paste as sibling" : "Paste"), this);
        action->setShortcut(QKeySequence::Paste);
        action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        QObject::connect(action,
            &QAction::triggered,
            hierarchy,
            [ hierarchy ]([[maybe_unused]] bool checked)
            {
                QMetaObject::invokeMethod(hierarchy, "PasteAsSibling", Qt::QueuedConnection);
            });
        addAction(action);

        if (!thereIsContentInTheClipboard)
        {
            // Nothing in the clipboard.
            // We want the menu to be visible, but disabled.
            action->setEnabled(false);
        }

        if (itemsAreSelected)
        {
            action = new QAction(QIcon(":/Icons/Eye_Open.png"), ("Paste as child"), this);
            {
                action->setShortcuts(QList<QKeySequence>{QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_V),
                                                         QKeySequence(Qt::META + Qt::SHIFT + Qt::Key_V)});
                action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
            }
            QObject::connect(action,
                &QAction::triggered,
                hierarchy,
                [hierarchy]([[maybe_unused]] bool checked)
                {
                    QMetaObject::invokeMethod(hierarchy, "PasteAsChild", Qt::QueuedConnection);
                });
            addAction(action);

            if (!thereIsContentInTheClipboard)
            {
                // Nothing in the clipboard.
                // We want the menu to be visible, but disabled.
                action->setEnabled(false);
            }
        }
    }
}

void HierarchyMenu::SliceMenuItems(HierarchyWidget* hierarchy,
    QTreeWidgetItemRawPtrQList& selectedItems,
    size_t showMask)
{
#if ENABLE_UI_SLICE_MENU_ITEMS
    // Get the EntityId's of the selected elements
    auto selectedEntities = SelectionHelpers::GetSelectedElementIds(hierarchy, selectedItems, false);

    // Determine if any of the selected entities are in a slice
    AZ::SliceComponent::EntityAncestorList referenceAncestors;

    AZStd::vector<AZ::SliceComponent::SliceInstanceAddress> sliceInstances;
    for (const AZ::EntityId& entityId : selectedEntities)
    {
        AZ::SliceComponent::SliceInstanceAddress sliceAddress;
        AzFramework::SliceEntityRequestBus::EventResult(sliceAddress, entityId,
            &AzFramework::SliceEntityRequestBus::Events::GetOwningSlice);

        if (sliceAddress.IsValid())
        {
            if (sliceInstances.end() == AZStd::find(sliceInstances.begin(), sliceInstances.end(), sliceAddress))
            {
                if (sliceInstances.empty())
                {
                    sliceAddress.GetReference()->GetInstanceEntityAncestry(entityId, referenceAncestors);
                }

                sliceInstances.push_back(sliceAddress);
            }
        }
    }

    bool sliceSelected = sliceInstances.size() > 0;

    if (sliceSelected)
    {
        if (showMask & Show::kPushToSlice)
        {
            // Push slice action currently acts on entities and all descendants, so include those as part of the selection
            AzToolsFramework::EntityIdSet selectedTransformHierarchyEntities =
                hierarchy->GetEditorWindow()->GetSliceManager()->GatherEntitiesAndAllDescendents(selectedEntities);

            AzToolsFramework::EntityIdList selectedPushEntities;
            selectedPushEntities.insert(selectedPushEntities.begin(), selectedTransformHierarchyEntities.begin(), selectedTransformHierarchyEntities.end());

            QAction* action = addAction(QObject::tr("&Push to Slice..."));
            QObject::connect(action, &QAction::triggered, hierarchy, [hierarchy, selectedPushEntities]
                {
                    hierarchy->GetEditorWindow()->GetSliceManager()->PushEntitiesModal(selectedPushEntities, nullptr);
                }
            );
        }

        if (showMask & Show::kNewSlice)
        {
            QAction* action = addAction("Make Cascaded Slice from Selected Slices && Entities...");
            QObject::connect(action, &QAction::triggered, hierarchy, [hierarchy, selectedEntities]
                {
                    hierarchy->GetEditorWindow()->GetSliceManager()->MakeSliceFromSelectedItems(hierarchy, true);
                }
            );

            action = addAction(QObject::tr("Make Detached Slice from Selected Entities..."));
            QObject::connect(action, &QAction::triggered, hierarchy, [hierarchy, selectedEntities]
                {
                    hierarchy->GetEditorWindow()->GetSliceManager()->MakeSliceFromSelectedItems(hierarchy, false);
                }
            );
        }

        if (showMask & Show::kPushToSlice)  // use the push to slice flag to show detach since it appears in all the same situations
        {
            // Detach slice entity
            {
                // Detach entities action currently acts on entities and all descendants, so include those as part of the selection
                AzToolsFramework::EntityIdSet selectedTransformHierarchyEntities =
                    hierarchy->GetEditorWindow()->GetSliceManager()->GatherEntitiesAndAllDescendents(selectedEntities);

                AzToolsFramework::EntityIdList selectedDetachEntities;
                selectedDetachEntities.insert(selectedDetachEntities.begin(), selectedTransformHierarchyEntities.begin(), selectedTransformHierarchyEntities.end());

                QString detachEntitiesActionText;
                if (selectedDetachEntities.size() == 1)
                {
                    detachEntitiesActionText = QObject::tr("Detach slice entity...");
                }
                else
                {
                    detachEntitiesActionText = QObject::tr("Detach slice entities...");
                }
                QAction* action = addAction(detachEntitiesActionText);
                QObject::connect(action, &QAction::triggered, hierarchy, [hierarchy, selectedDetachEntities]
                    {
                        hierarchy->GetEditorWindow()->GetSliceManager()->DetachSliceEntities(selectedDetachEntities);
                        hierarchy->UpdateSliceInfo();
                    }
                );
            }

            // Detach slice instance
            {
                QString detachSlicesActionText;
                if (sliceInstances.size() == 1)
                {
                    detachSlicesActionText = QObject::tr("Detach slice instance...");
                }
                else
                {
                    detachSlicesActionText = QObject::tr("Detach slice instances...");
                }
                QAction* action = addAction(detachSlicesActionText);
                QObject::connect(action, &QAction::triggered, hierarchy, [hierarchy, selectedEntities]
                    {
                        hierarchy->GetEditorWindow()->GetSliceManager()->DetachSliceInstances(selectedEntities);
                        hierarchy->UpdateSliceInfo();
                    }
                );
            }

            // Edit slice in new tab
            {
                QMenu* menu = addMenu("Edit slice in new tab");

                // Catalog all unique slices to which any of the selected entities are associated (anywhere in their ancestry).
                // This is used to make a menu allowing any of them to be edited in a new tab
                AZStd::vector<AZ::Data::AssetId> slicesAddedToMenu;
                AZ::SliceComponent::EntityAncestorList tempAncestors;

                for (AZ::EntityId entityId : selectedEntities)
                {
                    AZ::SliceComponent::SliceInstanceAddress sliceAddress;
                    AzFramework::SliceEntityRequestBus::EventResult(sliceAddress, entityId,
                        &AzFramework::SliceEntityRequestBus::Events::GetOwningSlice);

                    if (sliceAddress.IsValid())
                    {
                        tempAncestors.clear();
                        sliceAddress.GetReference()->GetInstanceEntityAncestry(entityId, tempAncestors);

                        for (const AZ::SliceComponent::Ancestor& ancestor : tempAncestors)
                        {
                            const AZ::Data::Asset<AZ::SliceAsset>& sliceAsset = ancestor.m_sliceAddress.GetReference()->GetSliceAsset();

                            // If this slice has not already been added to the menu then add it.
                            if (slicesAddedToMenu.end() == AZStd::find(slicesAddedToMenu.begin(), slicesAddedToMenu.end(), sliceAsset.GetId()))
                            {
                                const AZStd::string& assetPath = sliceAsset.GetHint();
                                slicesAddedToMenu.push_back(sliceAsset.GetId());

                                QAction* action = menu->addAction(assetPath.c_str());
                                QObject::connect(action, &QAction::triggered, [this, hierarchy, sliceAsset]
                                    {
                                        hierarchy->GetEditorWindow()->EditSliceInNewTab(sliceAsset.GetId());
                                    }
                                );
                            }
                        }
                    }
                }


            }
        }

    }
    else
    {
        if (showMask & Show::kNewSlice)
        {
            QAction* action = addAction(QObject::tr("Make New &Slice from Selection..."));
            QObject::connect(action, &QAction::triggered, hierarchy, [hierarchy]
                {
                    hierarchy->GetEditorWindow()->GetSliceManager()->MakeSliceFromSelectedItems(hierarchy, false);
                }
            );

            if (selectedItems.size() == 0)
            {
                action->setEnabled(false);
            }
        }
    }
#endif
}

void HierarchyMenu::New_EmptyElement(HierarchyWidget* hierarchy,
    QTreeWidgetItemRawPtrQList& selectedItems,
    QMenu* menu,
    bool addAtRoot,
    const QPoint* optionalPos)
{
    menu->addAction(HierarchyHelpers::CreateAddElementAction(hierarchy,
            selectedItems,
            addAtRoot,
            optionalPos));
}

void HierarchyMenu::New_ElementFromSlice(HierarchyWidget* hierarchy,
    QTreeWidgetItemRawPtrQList& selectedItems,
    QMenu* menu,
    bool addAtRoot,
    const QPoint* optionalPos)
{
#if ENABLE_UI_SLICE_MENU_ITEMS
    AZ::Vector2 viewportPosition(-1.0f,-1.0f); // indicates no viewport position specified
    if (optionalPos)
    {
        viewportPosition = QtHelpers::QPointFToVector2(*optionalPos);
    }

    SliceMenuHelpers::CreateInstantiateSliceMenu(hierarchy,
        selectedItems,
        menu,
        addAtRoot,
        viewportPosition);

    QAction* action = menu->addAction(QObject::tr("Element from Slice &Browser..."));
    QObject::connect(
        action,
        &QAction::triggered,
        hierarchy, [hierarchy, optionalPos]
        {
            AZ::Vector2 viewportPosition(-1.0f,-1.0f); // indicates no viewport position specified
            if (optionalPos)
            {
                viewportPosition = QtHelpers::QPointFToVector2(*optionalPos);
            }
            hierarchy->GetEditorWindow()->GetSliceManager()->InstantiateSliceUsingBrowser(hierarchy, viewportPosition);
        }
    );
#endif
}

void HierarchyMenu::AddComponents(HierarchyWidget* hierarchy,
    QTreeWidgetItemRawPtrQList& selectedItems)
{
    addActions(ComponentHelpers::CreateAddComponentActions(hierarchy,
            selectedItems,
            this));
}

void HierarchyMenu::DeleteElement(HierarchyWidget* hierarchy,
    QTreeWidgetItemRawPtrQList& selectedItems)
{
    QAction* action;

    // Delete element.
    {
        action = new QAction("Delete", this);
        action->setShortcut(QKeySequence::Delete);
        action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        QObject::connect(action,
            &QAction::triggered,
            hierarchy,
            [ hierarchy ]([[maybe_unused]] bool checked)
            {
                QMetaObject::invokeMethod(hierarchy, "DeleteSelectedItems", Qt::QueuedConnection);
            });
        addAction(action);

        if (selectedItems.empty())
        {
            // Nothing has been selected.
            // We want the menu to be visible, but disabled.
            action->setEnabled(false);
        }
    }
}

void HierarchyMenu::FindElements(HierarchyWidget* hierarchy,
    [[maybe_unused]] QTreeWidgetItemRawPtrQList& selectedItems)
{
    QAction* action;

    // Find elements
    {
        action = new QAction("Find Elements...", this);
        action->setShortcut(QKeySequence::Find);
        action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        QObject::connect(action,
            &QAction::triggered,
            hierarchy,
            [ hierarchy ]([[maybe_unused]] bool checked)
            {
                hierarchy->GetEditorWindow()->ShowEntitySearchModal();
            });
        addAction(action);
    }
}

void HierarchyMenu::EditorOnly(HierarchyWidget* hierarchy,
    QTreeWidgetItemRawPtrQList& selectedItems)
{
    QAction* action;

    // Toggle editor only state.
    {
        action = new QAction("Editor Only", this);
        action->setCheckable(true);
        
        if (selectedItems.empty())
        {
            action->setChecked(false);
            action->setEnabled(false);
        }
        else
        {
            EntityHelpers::EntityIdList entityIds = SelectionHelpers::GetSelectedElementIds(hierarchy, selectedItems, false);

            bool checked = true;
            for (auto entityId : entityIds)
            {
                bool isEditorOnly = false;
                AzToolsFramework::EditorOnlyEntityComponentRequestBus::EventResult(isEditorOnly, entityId, &AzToolsFramework::EditorOnlyEntityComponentRequests::IsEditorOnlyEntity);
            
                if (!isEditorOnly)
                {
                    checked = false;
                    break;
                }
            }

            action->setChecked(checked);
            action->setEnabled(true);
        }

        QObject::connect(action,
            &QAction::triggered,
            [hierarchy](bool checked)
        {
            QMetaObject::invokeMethod(hierarchy, "SetEditorOnlyForSelectedItems", Qt::QueuedConnection, Q_ARG(bool, checked));
        });
        addAction(action);
    }
}

#include <moc_HierarchyMenu.cpp>
