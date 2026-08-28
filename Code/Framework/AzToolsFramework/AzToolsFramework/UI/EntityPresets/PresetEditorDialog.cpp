/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/EntityPresets/PresetEditorDialog.h>
#include <AzToolsFramework/UI/EntityPresets/EntityPresetMenu.h>
#include <AzToolsFramework/UI/EntityPresets/EntityPresetsStringUtils.h>

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Interface/Interface.h>

#include <AzToolsFramework/Component/EditorComponentAPIBus.h>

#include <QBrush>
#include <QColor>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QStringList>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

namespace AzToolsFramework
{
    using EntityPresets::ToAZString;
    using EntityPresets::ToQString;

    namespace
    {


        //! Names as they appear in Add Component. Unlike property paths, these *can* be
        //! enumerated, so the component picker is a real list rather than a text box to mistype.
        QStringList AllComponentNames()
        {
            AZStd::vector<AZStd::string> names;
            EditorComponentAPIBus::BroadcastResult(
                names, &EditorComponentAPIRequests::BuildComponentTypeNameListByEntityType,
                EditorComponentAPIRequests::EntityType::Game);

            QStringList result;
            result.reserve(aznumeric_cast<int>(names.size()));
            for (const AZStd::string& name : names)
            {
                result.append(ToQString(name));
            }

            result.sort(Qt::CaseInsensitive);
            return result;
        }

        const char* TypeLabel(const EntityPresets::PropertyValue::Type type)
        {
            switch (type)
            {
            case EntityPresets::PropertyValue::Type::Bool:
                return "Bool";
            case EntityPresets::PropertyValue::Type::Double:
                return "Number";
            case EntityPresets::PropertyValue::Type::String:
                return "Text";
            case EntityPresets::PropertyValue::Type::AssetPath:
                return "Asset path";
            case EntityPresets::PropertyValue::Type::Int:
            default:
                return "Whole number";
            }
        }

        QStringList AllTypeLabels()
        {
            return { QStringLiteral("Whole number"), QStringLiteral("Number"), QStringLiteral("Bool"),
                     QStringLiteral("Text"), QStringLiteral("Asset path") };
        }

        EntityPresets::PropertyValue::Type TypeFromLabel(const QString& label)
        {
            if (label == QLatin1String("Bool"))
            {
                return EntityPresets::PropertyValue::Type::Bool;
            }
            if (label == QLatin1String("Number"))
            {
                return EntityPresets::PropertyValue::Type::Double;
            }
            if (label == QLatin1String("Text"))
            {
                return EntityPresets::PropertyValue::Type::String;
            }
            if (label == QLatin1String("Asset path"))
            {
                return EntityPresets::PropertyValue::Type::AssetPath;
            }
            return EntityPresets::PropertyValue::Type::Int;
        }

        QString ValueText(const EntityPresets::PropertyValue& value)
        {
            switch (value.m_type)
            {
            case EntityPresets::PropertyValue::Type::Bool:
                return value.m_bool ? QStringLiteral("true") : QStringLiteral("false");
            case EntityPresets::PropertyValue::Type::Double:
                return QString::number(value.m_double);
            case EntityPresets::PropertyValue::Type::String:
            case EntityPresets::PropertyValue::Type::AssetPath:
                return ToQString(value.m_string);
            case EntityPresets::PropertyValue::Type::Int:
            default:
                return QString::number(aznumeric_cast<qlonglong>(value.m_int));
            }
        }

        EntityPresets::PropertyValue ValueFromText(
            const EntityPresets::PropertyValue::Type type, const QString& text)
        {
            EntityPresets::PropertyValue value;
            value.m_type = type;

            switch (type)
            {
            case EntityPresets::PropertyValue::Type::Bool:
                value.m_bool = text.compare(QLatin1String("true"), Qt::CaseInsensitive) == 0 ||
                    text == QLatin1String("1");
                break;
            case EntityPresets::PropertyValue::Type::Double:
                value.m_double = text.toDouble();
                break;
            case EntityPresets::PropertyValue::Type::String:
            case EntityPresets::PropertyValue::Type::AssetPath:
                value.m_string = ToAZString(text);
                break;
            case EntityPresets::PropertyValue::Type::Int:
            default:
                value.m_int = aznumeric_cast<AZ::s64>(text.toLongLong());
                break;
            }

            return value;
        }
    } // namespace

    // -- Edit one preset --------------------------------------------------------------------

    EntityPresetEditDialog::EntityPresetEditDialog(const EntityPresets::Preset& preset, QWidget* parent)
        : QDialog(parent)
    {
        setWindowTitle(QStringLiteral("Edit Preset"));
        setMinimumSize(680, 460);

        auto* layout = new QVBoxLayout(this);

        {
            auto* row = new QHBoxLayout();
            row->addWidget(new QLabel(QStringLiteral("Name"), this));
            m_nameEdit = new QLineEdit(ToQString(preset.m_name), this);
            row->addWidget(m_nameEdit);
            layout->addLayout(row);
        }

        {
            auto* row = new QHBoxLayout();
            row->addWidget(new QLabel(QStringLiteral("Description"), this));
            m_descriptionEdit = new QLineEdit(ToQString(preset.m_description), this);
            m_descriptionEdit->setPlaceholderText(
                QStringLiteral("Shown as the menu tooltip - say what it makes, and anything the user "
                               "still has to do themselves"));
            row->addWidget(m_descriptionEdit);
            layout->addLayout(row);
        }

        {
            auto* row = new QHBoxLayout();
            row->addWidget(new QLabel(QStringLiteral("Category"), this));

            m_categoryCombo = new QComboBox(this);
            // Editable and seeded with what already exists: picking an existing category groups the
            // preset with it in the menu, typing a new one starts a new submenu.
            m_categoryCombo->setEditable(true);

            QStringList categories;
            for (const EntityPresets::Preset& existing : EntityPresets::All())
            {
                const QString category = ToQString(existing.m_category);
                if (!category.isEmpty() && !categories.contains(category))
                {
                    categories.append(category);
                }
            }
            categories.sort(Qt::CaseInsensitive);
            m_categoryCombo->addItems(categories);
            m_categoryCombo->setCurrentText(ToQString(preset.m_category));

            row->addWidget(m_categoryCombo);
            layout->addLayout(row);
        }

        m_levelComponents = preset.m_levelComponents;
        if (!m_levelComponents.empty())
        {
            QStringList names;
            for (const EntityPresets::ComponentSpec& component : m_levelComponents)
            {
                names.append(ToQString(component.m_componentName));
            }

            // Worth saying out loud: these are added to the level entity, not to the new entity, so
            // they do not appear in the tree below and their absence there is not a mistake.
            auto* levelLabel = new QLabel(
                QStringLiteral("Level components: %1").arg(names.join(QStringLiteral(", "))), this);
            levelLabel->setEnabled(false);
            layout->addWidget(levelLabel);
        }

        auto* hint = new QLabel(
            QStringLiteral("Components are added in order. Property paths are spelled the way the "
                           "Entity Inspector shows them, for example "
                           "Controller|Configuration|Light type - leave properties out to keep a "
                           "component's defaults."),
            this);
        hint->setWordWrap(true);
        layout->addWidget(hint);

        m_tree = new QTreeWidget(this);
        m_tree->setColumnCount(ColumnCount);
        m_tree->setHeaderLabels(
            { QStringLiteral("Component / Property"), QStringLiteral("Type"), QStringLiteral("Value") });
        m_tree->header()->setSectionResizeMode(ColumnName, QHeaderView::Stretch);
        m_tree->header()->setSectionResizeMode(ColumnType, QHeaderView::ResizeToContents);
        m_tree->header()->setSectionResizeMode(ColumnValue, QHeaderView::ResizeToContents);
        layout->addWidget(m_tree);

        PopulateTree(preset);
        m_tree->expandAll();

        {
            auto* row = new QHBoxLayout();

            auto* addComponent = new QPushButton(QStringLiteral("Add Component"), this);
            connect(addComponent, &QPushButton::clicked, this, &EntityPresetEditDialog::OnAddComponent);
            row->addWidget(addComponent);

            auto* addProperty = new QPushButton(QStringLiteral("Add Property"), this);
            addProperty->setToolTip(QStringLiteral("Adds a property to the selected component."));
            connect(addProperty, &QPushButton::clicked, this, &EntityPresetEditDialog::OnAddProperty);
            row->addWidget(addProperty);

            auto* remove = new QPushButton(QStringLiteral("Remove Selected"), this);
            connect(remove, &QPushButton::clicked, this, &EntityPresetEditDialog::OnRemoveSelected);
            row->addWidget(remove);

            row->addStretch();
            layout->addLayout(row);
        }

        auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        connect(
            buttons, &QDialogButtonBox::accepted, this,
            [this]()
            {
                if (m_nameEdit->text().trimmed().isEmpty())
                {
                    QMessageBox::warning(
                        this, QStringLiteral("Preset"),
                        QStringLiteral("Give the preset a name - it is what appears in the menu."));
                    return;
                }
                accept();
            });
        connect(buttons, &QDialogButtonBox::rejected, this, &EntityPresetEditDialog::reject);
        layout->addWidget(buttons);
    }

    void EntityPresetEditDialog::PopulateTree(const EntityPresets::Preset& preset)
    {
        for (const EntityPresets::ComponentSpec& component : preset.m_components)
        {
            QTreeWidgetItem* componentItem = AddComponentRow(component.m_componentName);
            for (const EntityPresets::PropertyAssignment& assignment : component.m_properties)
            {
                AddPropertyRow(componentItem, assignment);
            }
        }
    }

    QTreeWidgetItem* EntityPresetEditDialog::AddComponentRow(const AZStd::string& componentName)
    {
        auto* item = new QTreeWidgetItem(m_tree);
        item->setText(ColumnName, ToQString(componentName));
        item->setExpanded(true);
        return item;
    }

    void EntityPresetEditDialog::AddPropertyRow(
        QTreeWidgetItem* componentItem, const EntityPresets::PropertyAssignment& assignment)
    {
        auto* item = new QTreeWidgetItem(componentItem);
        item->setText(ColumnName, ToQString(assignment.m_path));
        item->setText(ColumnValue, ValueText(assignment.m_value));
        item->setFlags(item->flags() | Qt::ItemIsEditable);

        // The type lives in a combo rather than free text so it cannot be misspelled - it decides
        // how the value is converted for the property setter, and a wrong type silently fails.
        auto* typeCombo = new QComboBox(m_tree);
        typeCombo->addItems(AllTypeLabels());
        typeCombo->setCurrentText(QString::fromUtf8(TypeLabel(assignment.m_value.m_type)));
        m_tree->setItemWidget(item, ColumnType, typeCombo);
    }

    QTreeWidgetItem* EntityPresetEditDialog::SelectedComponentItem() const
    {
        QTreeWidgetItem* item = m_tree->currentItem();
        if (item == nullptr)
        {
            return nullptr;
        }

        // A property row stands in for its component, so "Add Property" works whether the user
        // clicked the component or one of its existing properties.
        return item->parent() != nullptr ? item->parent() : item;
    }

    void EntityPresetEditDialog::OnAddComponent()
    {
        const QStringList names = AllComponentNames();
        if (names.isEmpty())
        {
            QMessageBox::warning(
                this, QStringLiteral("Preset"),
                QStringLiteral("No components could be listed. This needs a level to be open."));
            return;
        }

        bool accepted = false;
        const QString name = QInputDialog::getItem(
            this, QStringLiteral("Add Component"), QStringLiteral("Component:"), names, 0, true, &accepted);

        if (accepted && !name.isEmpty())
        {
            QTreeWidgetItem* item = AddComponentRow(ToAZString(name));
            m_tree->setCurrentItem(item);
        }
    }

    void EntityPresetEditDialog::OnAddProperty()
    {
        QTreeWidgetItem* componentItem = SelectedComponentItem();
        if (componentItem == nullptr)
        {
            QMessageBox::information(
                this, QStringLiteral("Preset"), QStringLiteral("Select a component to add a property to."));
            return;
        }

        bool accepted = false;
        const QString path = QInputDialog::getText(
            this, QStringLiteral("Add Property"),
            QStringLiteral("Property path, as the Entity Inspector spells it:"), QLineEdit::Normal,
            QStringLiteral("Controller|Configuration|"), &accepted);

        if (!accepted || path.trimmed().isEmpty())
        {
            return;
        }

        EntityPresets::PropertyAssignment assignment;
        assignment.m_path = ToAZString(path.trimmed());
        assignment.m_value.m_type = EntityPresets::PropertyValue::Type::Int;

        AddPropertyRow(componentItem, assignment);
        componentItem->setExpanded(true);
    }

    void EntityPresetEditDialog::OnRemoveSelected()
    {
        QTreeWidgetItem* item = m_tree->currentItem();
        if (item == nullptr)
        {
            return;
        }

        // Removing a component takes its properties with it, which is what deleting the parent
        // row already does - they are its children.
        delete item;
    }

    EntityPresets::Preset EntityPresetEditDialog::Result() const
    {
        EntityPresets::Preset preset;
        preset.m_name = ToAZString(m_nameEdit->text().trimmed());
        preset.m_description = ToAZString(m_descriptionEdit->text().trimmed());
        preset.m_category = ToAZString(m_categoryCombo->currentText().trimmed());
        preset.m_levelComponents = m_levelComponents;
        preset.m_readOnly = false;

        for (int componentIndex = 0; componentIndex < m_tree->topLevelItemCount(); ++componentIndex)
        {
            QTreeWidgetItem* componentItem = m_tree->topLevelItem(componentIndex);

            EntityPresets::ComponentSpec component;
            component.m_componentName = ToAZString(componentItem->text(ColumnName).trimmed());
            if (component.m_componentName.empty())
            {
                continue;
            }

            for (int propertyIndex = 0; propertyIndex < componentItem->childCount(); ++propertyIndex)
            {
                QTreeWidgetItem* propertyItem = componentItem->child(propertyIndex);

                const QString path = propertyItem->text(ColumnName).trimmed();
                if (path.isEmpty())
                {
                    continue;
                }

                auto* typeCombo = qobject_cast<QComboBox*>(m_tree->itemWidget(propertyItem, ColumnType));
                const EntityPresets::PropertyValue::Type type =
                    typeCombo != nullptr ? TypeFromLabel(typeCombo->currentText())
                                         : EntityPresets::PropertyValue::Type::Int;

                EntityPresets::PropertyAssignment assignment;
                assignment.m_path = ToAZString(path);
                assignment.m_value = ValueFromText(type, propertyItem->text(ColumnValue));

                component.m_properties.push_back(AZStd::move(assignment));
            }

            preset.m_components.push_back(AZStd::move(component));
        }

        return preset;
    }

    // -- Manage the set ---------------------------------------------------------------------

    EntityPresetManagerDialog::EntityPresetManagerDialog(QWidget* parent)
        : QDialog(parent)
        , m_userPresets(EntityPresets::User())
    {
        setWindowTitle(QStringLiteral("Entity Presets"));
        setMinimumSize(560, 460);

        auto* layout = new QVBoxLayout(this);

        auto* info = new QLabel(
            QStringLiteral("Presets appear under Create Preset in the viewport and Entity Outliner "
                           "right-click menus. Presets shipped by gems cannot be changed - duplicate "
                           "one to make an editable copy that is saved with this project."),
            this);
        info->setWordWrap(true);
        layout->addWidget(info);

        m_list = new QListWidget(this);
        connect(m_list, &QListWidget::currentRowChanged, this, [this](int) { SyncButtons(); });
        connect(m_list, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem*) { OnEdit(); });
        layout->addWidget(m_list);

        {
            auto* row = new QHBoxLayout();

            auto* add = new QPushButton(QStringLiteral("New"), this);
            connect(add, &QPushButton::clicked, this, &EntityPresetManagerDialog::OnAdd);
            row->addWidget(add);

            auto* edit = new QPushButton(QStringLiteral("Edit"), this);
            connect(edit, &QPushButton::clicked, this, &EntityPresetManagerDialog::OnEdit);
            row->addWidget(edit);
            m_editButton = edit;

            auto* duplicate = new QPushButton(QStringLiteral("Duplicate"), this);
            connect(duplicate, &QPushButton::clicked, this, &EntityPresetManagerDialog::OnDuplicate);
            row->addWidget(duplicate);
            m_duplicateButton = duplicate;

            auto* remove = new QPushButton(QStringLiteral("Delete"), this);
            connect(remove, &QPushButton::clicked, this, &EntityPresetManagerDialog::OnRemove);
            row->addWidget(remove);
            m_removeButton = remove;

            row->addStretch();

            // Re-read everything from disk. Presets are plain JSON, so a gem author editing a
            // Presets/*.json file wants to see the result now rather than after an editor restart.
            auto* reload = new QPushButton(QStringLiteral("Reload from Disk"), this);
            reload->setToolTip(QStringLiteral(
                "Re-read presets from this project and from every enabled gem's Presets folder."));
            connect(reload, &QPushButton::clicked, this, &EntityPresetManagerDialog::OnReload);
            row->addWidget(reload);
            layout->addLayout(row);
        }

        auto* pathLabel = new QLabel(
            QStringLiteral("Saved to %1").arg(ToQString(EntityPresets::UserPresetsPath())), this);
        pathLabel->setWordWrap(true);
        pathLabel->setStyleSheet(QStringLiteral("color: #8a8a9e; font-size: 10px;"));
        layout->addWidget(pathLabel);

        auto* buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
        connect(buttons, &QDialogButtonBox::accepted, this, &EntityPresetManagerDialog::OnSaveAndClose);
        connect(buttons, &QDialogButtonBox::rejected, this, &EntityPresetManagerDialog::reject);
        layout->addWidget(buttons);

        Populate();
    }

    void EntityPresetManagerDialog::Populate()
    {
        const int previousRow = m_list->currentRow();

        m_list->clear();

        const QBrush readOnlyBrush(QColor(0x8a, 0x8a, 0x9e));

        for (const EntityPresets::Preset& preset : EntityPresets::BuiltIn())
        {
            auto* item = new QListWidgetItem(
                QStringLiteral("%1  -  %2  (engine)")
                    .arg(ToQString(preset.m_category), ToQString(preset.m_name)),
                m_list);
            item->setForeground(readOnlyBrush);
        }

        for (const EntityPresets::Preset& preset : EntityPresets::FromGems())
        {
            auto* item = new QListWidgetItem(
                QStringLiteral("%1  -  %2  (from %3)")
                    .arg(
                        ToQString(preset.m_category), ToQString(preset.m_name),
                        ToQString(preset.m_sourceGem)),
                m_list);
            item->setForeground(readOnlyBrush);
            item->setToolTip(QStringLiteral(
                "Shipped by the %1 gem. Duplicate it to get an editable copy - editing it here "
                "would be lost the next time that gem updated.")
                                 .arg(ToQString(preset.m_sourceGem)));
        }

        for (const EntityPresets::Preset& preset : m_userPresets)
        {
            new QListWidgetItem(
                QStringLiteral("%1  -  %2").arg(ToQString(preset.m_category), ToQString(preset.m_name)), m_list);
        }

        if (previousRow >= 0 && previousRow < m_list->count())
        {
            m_list->setCurrentRow(previousRow);
        }

        SyncButtons();
    }

    int EntityPresetManagerDialog::SelectedUserIndex() const
    {
        const int row = m_list->currentRow();

        // Read-only rows come first - compiled-in, then whatever the enabled gems ship - and
        // anything past them indexes into the user list.
        const int readOnlyCount = aznumeric_cast<int>(EntityPresets::BuiltIn().size()) +
            aznumeric_cast<int>(EntityPresets::FromGems().size());

        if (row < readOnlyCount)
        {
            return -1;
        }

        const int userIndex = row - readOnlyCount;
        return userIndex < aznumeric_cast<int>(m_userPresets.size()) ? userIndex : -1;
    }

    void EntityPresetManagerDialog::SyncButtons()
    {
        const bool userSelected = SelectedUserIndex() >= 0;
        const bool anySelected = m_list->currentRow() >= 0;

        m_editButton->setEnabled(userSelected);
        m_removeButton->setEnabled(userSelected);
        // Duplicating a shipped preset is the supported way to start from one, so this stays enabled for
        // any selection.
        m_duplicateButton->setEnabled(anySelected);
    }

    void EntityPresetManagerDialog::OnAdd()
    {
        EntityPresets::Preset preset;
        preset.m_category = "Custom";

        EntityPresetEditDialog dialog(preset, this);
        if (dialog.exec() == QDialog::Accepted)
        {
            m_userPresets.push_back(dialog.Result());
            Populate();
            m_list->setCurrentRow(m_list->count() - 1);
        }
    }

    void EntityPresetManagerDialog::OnEdit()
    {
        const int index = SelectedUserIndex();
        if (index < 0)
        {
            return;
        }

        EntityPresetEditDialog dialog(m_userPresets[aznumeric_cast<size_t>(index)], this);
        if (dialog.exec() == QDialog::Accepted)
        {
            m_userPresets[aznumeric_cast<size_t>(index)] = dialog.Result();
            Populate();
        }
    }

    void EntityPresetManagerDialog::OnDuplicate()
    {
        const int row = m_list->currentRow();
        if (row < 0)
        {
            return;
        }

        const int builtInCount = aznumeric_cast<int>(EntityPresets::BuiltIn().size());
        const int gemCount = aznumeric_cast<int>(EntityPresets::FromGems().size());

        EntityPresets::Preset source;
        if (row < builtInCount)
        {
            source = EntityPresets::BuiltIn()[aznumeric_cast<size_t>(row)];
        }
        else if (row < builtInCount + gemCount)
        {
            source = EntityPresets::FromGems()[aznumeric_cast<size_t>(row - builtInCount)];
        }
        else
        {
            source = m_userPresets[aznumeric_cast<size_t>(row - builtInCount - gemCount)];
        }

        // The copy is the user's, wherever it came from - editable, saved to the project, and no
        // longer tied to the gem that may have supplied the original.
        source.m_readOnly = false;
        source.m_sourceGem.clear();
        source.m_name += " Copy";

        m_userPresets.push_back(source);
        Populate();
        m_list->setCurrentRow(m_list->count() - 1);
    }

    void EntityPresetManagerDialog::OnRemove()
    {
        const int index = SelectedUserIndex();
        if (index < 0)
        {
            return;
        }

        const QString name = ToQString(m_userPresets[aznumeric_cast<size_t>(index)].m_name);
        if (QMessageBox::question(
                this, QStringLiteral("Delete Preset"), QStringLiteral("Delete '%1'?").arg(name)) !=
            QMessageBox::Yes)
        {
            return;
        }

        m_userPresets.erase(m_userPresets.begin() + index);
        Populate();
    }

    void EntityPresetManagerDialog::OnReload()
    {
        // Reloading replaces the working copy, so anything unsaved goes with it. Cheaper to ask
        // than to track a dirty flag, and the answer is obvious in the moment.
        if (QMessageBox::question(
                this, QStringLiteral("Reload Presets"),
                QStringLiteral("Re-read presets from disk? Any unsaved changes here will be lost.")) !=
            QMessageBox::Yes)
        {
            return;
        }

        EntityPresets::Reload();
        m_userPresets = EntityPresets::User();
        Populate();

        // Rebuild the menus too - a preset that appeared on disk should be usable straight away,
        // not just visible in this list.
        EntityPresetMenu::Refresh();
    }

    void EntityPresetManagerDialog::OnSaveAndClose()
    {
        if (!EntityPresets::SaveUser(m_userPresets))
        {
            QMessageBox::warning(
                this, QStringLiteral("Entity Presets"),
                QStringLiteral("Could not write the presets file. See the console for why."));
            return;
        }

        // Rebuild the actions and menus so the change shows up on the next right click rather than
        // after a restart.
        EntityPresetMenu::Refresh();

        accept();
    }
} // namespace AzToolsFramework
