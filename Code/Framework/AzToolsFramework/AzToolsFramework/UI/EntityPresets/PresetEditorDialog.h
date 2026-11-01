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

#include <AzCore/std/functional.h>

#include <QDialog>
#include <QString>
#endif

class QComboBox;
class QLineEdit;
class QListWidget;
class QTreeWidget;
class QTreeWidgetItem;

namespace AzToolsFramework
{
    //! Edits a copy of one preset: name, description, category and components.
    class AZTF_API EntityPresetEditDialog : public QDialog
    {
        Q_OBJECT

    public:
        //! Describes the preset an edited one would share a menu entry with, or returns an empty
        //! string. OK is refused while there is a clash.
        using ClashCheck = AZStd::function<QString(const EntityPresets::Preset&)>;

        EntityPresetEditDialog(
            const EntityPresets::Preset& preset, QWidget* parent = nullptr, ClashCheck clashCheck = {});

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

        //! The component row for the selection (a selected property stands in for its component), or null.
        QTreeWidgetItem* SelectedComponentItem() const;

        QLineEdit* m_nameEdit = nullptr;
        QLineEdit* m_descriptionEdit = nullptr;

        //! Not editable here; carried through so Result() keeps them.
        AZStd::vector<EntityPresets::ComponentSpec> m_levelComponents;
        QComboBox* m_categoryCombo = nullptr;
        QTreeWidget* m_tree = nullptr;

        ClashCheck m_clashCheck;
    };

    //! Lists every preset and edits the user's own. Compiled-in and gem presets are read-only;
    //! duplicating one makes an editable user copy.
    class AZTF_API EntityPresetManagerDialog : public QDialog
    {
        Q_OBJECT

    public:
        explicit EntityPresetManagerDialog(QWidget* parent = nullptr);

    private:
        void Populate();

        //! The user preset selected in the list, or -1 for a read-only one or nothing.
        int SelectedUserIndex() const;

        //! The preset @p candidate would share a menu entry with, described for a message, or an
        //! empty string. @p ignoreUserIndex is the user preset being edited, or -1.
        QString ClashingPreset(const EntityPresets::Preset& candidate, int ignoreUserIndex) const;

        //! "<name> Copy", numbered if needed so the copy gets its own menu entry.
        AZStd::string UniqueCopyName(EntityPresets::Preset copy) const;

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
