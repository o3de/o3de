/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "HierarchyMenu.h"

#include "Helpers/ComponentHelpers.h"
#include "Helpers/EntityHelpers.h"
#include "Helpers/HierarchyHelpers.h"
#include "Helpers/QtHelpers.h"
#include "Helpers/SelectionHelpers.h"
#include "HierarchyItem.h"
#include "HierarchyWidget.h"
#include "Widgets/ViewportWidget/ViewportWidget.h"
#include "Windows/EditorCommon.h"
#include "Windows/EditorWindow/EditorWindow.h"
#include "Windows/EditorWindow/UiPrefabManager.h"

#include <AzCore/Math/Vector2.h>
#include <AzToolsFramework/ToolsComponents/EditorOnlyEntityComponentBus.h>

#include <QAction>
#include <QKeySequence>
#include <QPoint>

HierarchyMenu::HierarchyMenu(HierarchyWidget* hierarchy, size_t showMask, bool addMenuForNewElement, const QPoint* optionalPos)
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

    if (showMask & Show::kPrefab)
    {
        Prefab(hierarchy, selectedItems);
    }

    addSeparator();

    if (showMask & Show::kEditorOnly)
    {
        EditorOnly(hierarchy, selectedItems);
    }
}

void HierarchyMenu::CutCopyPaste(HierarchyWidget* hierarchy, QTreeWidgetItemRawPtrQList& selectedItems)
{
    QAction* action;

    bool itemsAreSelected = (!selectedItems.isEmpty());

    // Cut element.
    {
        action = new QAction("Cut", this);
        action->setShortcut(QKeySequence::Cut);
        action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        QObject::connect(
            action,
            &QAction::triggered,
            hierarchy,
            [hierarchy]([[maybe_unused]] bool checked)
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
        QObject::connect(
            action,
            &QAction::triggered,
            hierarchy,
            [hierarchy]([[maybe_unused]] bool checked)
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
        QObject::connect(
            action,
            &QAction::triggered,
            hierarchy,
            [hierarchy]([[maybe_unused]] bool checked)
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
                action->setShortcuts(
                    QList<QKeySequence>{ QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_V), QKeySequence(Qt::META | Qt::SHIFT | Qt::Key_V) });
                action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
            }
            QObject::connect(
                action,
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

void HierarchyMenu::New_EmptyElement(
    HierarchyWidget* hierarchy, QTreeWidgetItemRawPtrQList& selectedItems, QMenu* menu, bool addAtRoot, const QPoint* optionalPos)
{
    menu->addAction(HierarchyHelpers::CreateAddElementAction(hierarchy, selectedItems, addAtRoot, optionalPos));
}

void HierarchyMenu::AddComponents(HierarchyWidget* hierarchy, QTreeWidgetItemRawPtrQList& selectedItems)
{
    addActions(ComponentHelpers::CreateAddComponentActions(hierarchy, selectedItems, this));
}

void HierarchyMenu::DeleteElement(HierarchyWidget* hierarchy, QTreeWidgetItemRawPtrQList& selectedItems)
{
    QAction* action;

    // Delete element.
    {
        action = new QAction("Delete", this);
        action->setShortcut(QKeySequence::Delete);
        action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        QObject::connect(
            action,
            &QAction::triggered,
            hierarchy,
            [hierarchy]([[maybe_unused]] bool checked)
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

void HierarchyMenu::FindElements(HierarchyWidget* hierarchy, [[maybe_unused]] QTreeWidgetItemRawPtrQList& selectedItems)
{
    QAction* action;

    // Find elements
    {
        action = new QAction("Find Elements...", this);
        action->setShortcut(QKeySequence::Find);
        action->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        QObject::connect(
            action,
            &QAction::triggered,
            hierarchy,
            [hierarchy]([[maybe_unused]] bool checked)
            {
                hierarchy->GetEditorWindow()->ShowEntitySearchModal();
            });
        addAction(action);
    }
}

void HierarchyMenu::Prefab(HierarchyWidget* hierarchy, QTreeWidgetItemRawPtrQList& selectedItems)
{
    bool itemsAreSelected = (!selectedItems.isEmpty());

    // Instantiate UI Prefab
    {
        QAction* action = new QAction("Instantiate UI Prefab...", this);
        QObject::connect(
            action,
            &QAction::triggered,
            hierarchy,
            [hierarchy]([[maybe_unused]] bool checked)
            {
                hierarchy->GetEditorWindow()->GetPrefabManager()->InstantiateUsingBrowser(hierarchy);
            });
        addAction(action);
    }

    // Save as UI Prefab
    {
        QAction* action = new QAction("Save Selection as UI Prefab...", this);
        QObject::connect(
            action,
            &QAction::triggered,
            hierarchy,
            [hierarchy]([[maybe_unused]] bool checked)
            {
                hierarchy->GetEditorWindow()->GetPrefabManager()->CreatePrefabFromSelection(hierarchy);
            });
        addAction(action);

        if (!itemsAreSelected)
        {
            action->setEnabled(false);
        }
    }
}

void HierarchyMenu::EditorOnly(HierarchyWidget* hierarchy, QTreeWidgetItemRawPtrQList& selectedItems)
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
                AzToolsFramework::EditorOnlyEntityComponentRequestBus::EventResult(
                    isEditorOnly, entityId, &AzToolsFramework::EditorOnlyEntityComponentRequests::IsEditorOnlyEntity);

                if (!isEditorOnly)
                {
                    checked = false;
                    break;
                }
            }

            action->setChecked(checked);
            action->setEnabled(true);
        }

        QObject::connect(
            action,
            &QAction::triggered,
            [hierarchy](bool checked)
            {
                QMetaObject::invokeMethod(hierarchy, "SetEditorOnlyForSelectedItems", Qt::QueuedConnection, Q_ARG(bool, checked));
            });
        addAction(action);
    }
}

#include <moc_HierarchyMenu.cpp>
