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
    //! Edits one preset: its name, description, category, and the components it puts on the entity.
    //!
    //! Works on a copy. Nothing is written until the manager saves, so cancelling anywhere in the
    //! chain leaves the stored presets untouched.
    class AZTF_API EntityPresetEditDialog : public QDialog
    {
        Q_OBJECT

    public:
        //! Describes the preset an edited one would clash with in the menu, or returns an empty
        //! string when there is none. OK stays refused until the clash is resolved.
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

        //! The component row for the current selection, whether a component or one of its
        //! properties is selected. Null if nothing usable is selected.
        QTreeWidgetItem* SelectedComponentItem() const;

        QLineEdit* m_nameEdit = nullptr;
        QLineEdit* m_descriptionEdit = nullptr;

        //! Carried through untouched: level components are not editable here yet, and rebuilding
        //! the preset from the widgets alone would silently drop them from a preset that had some.
        AZStd::vector<EntityPresets::ComponentSpec> m_levelComponents;
        QComboBox* m_categoryCombo = nullptr;
        QTreeWidget* m_tree = nullptr;

        ClashCheck m_clashCheck;
    };

    //! Lists every preset and manages the user's own.
    //!
    //! Almost every preset shown here belongs to a gem, and those are read-only: editing one would
    //! be overwritten the next time that gem updated, and the file it came from is not the
    //! project's to write. The same goes for the handful compiled into the framework. Duplicating
    //! either produces an ordinary user preset, which is how you start from a shipped one and
    //! change it.
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
        //! empty string if it has one of its own. @p ignoreUserIndex is the user preset being
        //! edited, which it may of course match; -1 for none.
        QString ClashingPreset(const EntityPresets::Preset& candidate, int ignoreUserIndex) const;

        //! "<name> Copy", numbered when that is taken, so a duplicate always gets its own entry.
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
