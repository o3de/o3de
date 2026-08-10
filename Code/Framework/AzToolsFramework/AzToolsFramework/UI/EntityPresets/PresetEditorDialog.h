/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/AzToolsFrameworkAPI.h>
#include <AzToolsFramework/Entity/EntityPresets/EntityPresets.h>

#include <QDialog>
#endif

class QComboBox;
class QLineEdit;
class QListWidget;
class QTreeWidget;
class QTreeWidgetItem;

namespace AzToolsFramework
{
    //! Edits one preset: its name, its category, and the components it puts on the entity.
    //!
    //! Works on a copy. Nothing is written until the manager saves, so cancelling anywhere in the
    //! chain leaves the stored presets untouched.
    class AZTF_API EntityPresetEditDialog : public QDialog
    {
        Q_OBJECT

    public:
        EntityPresetEditDialog(const EntityPresets::Preset& preset, QWidget* parent = nullptr);

        //! The edited preset. Only meaningful after exec() returned Accepted.
        EntityPresets::Preset Result() const;

    private:
        //! Component rows are top level; property rows are their children.
        enum Column
        {
            ColumnName = 0, //!< Component name, or property path.
            ColumnType,     //!< Property rows only.
            ColumnValue,    //!< Property rows only.
            ColumnCount
        };

        void PopulateTree(const EntityPresets::Preset& preset);
        QTreeWidgetItem* AddComponentRow(const AZStd::string& componentName);
        void AddPropertyRow(QTreeWidgetItem* componentItem, const EntityPresets::PropertyAssignment& assignment);

        void OnAddComponent();
        void OnAddProperty();
        void OnRemoveSelected();

        //! The component row for the current selection, whether a component or one of its
        //! properties is selected. Null if nothing usable is selected.
        QTreeWidgetItem* SelectedComponentItem() const;

        QLineEdit* m_nameEdit = nullptr;
        QComboBox* m_categoryCombo = nullptr;
        QTreeWidget* m_tree = nullptr;
    };

    //! Lists every preset and manages the user's own.
    //!
    //! Built-ins are shown but not editable - they are compiled in, so there is nothing to write
    //! back to. Duplicating one produces an ordinary user preset, which is how you start from a
    //! built-in and change it.
    class AZTF_API EntityPresetManagerDialog : public QDialog
    {
        Q_OBJECT

    public:
        explicit EntityPresetManagerDialog(QWidget* parent = nullptr);

    private:
        void Populate();

        //! The user preset selected in the list, or -1 for a built-in or nothing.
        int SelectedUserIndex() const;

        void OnAdd();
        void OnEdit();
        void OnDuplicate();
        void OnRemove();

        //! Re-read presets from the project and from every enabled gem.
        void OnReload();

        void OnSaveAndClose();

        //! Enable only the buttons that make sense for what is selected.
        void SyncButtons();

        QListWidget* m_list = nullptr;
        //! The working copy. Edited freely; written to disk only on save.
        AZStd::vector<EntityPresets::Preset> m_userPresets;

        QWidget* m_editButton = nullptr;
        QWidget* m_duplicateButton = nullptr;
        QWidget* m_removeButton = nullptr;
    };
} // namespace AzToolsFramework
