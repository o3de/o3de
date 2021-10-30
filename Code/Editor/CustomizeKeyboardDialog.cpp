/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorDefs.h"

#include "CustomizeKeyboardDialog.h"

// Qt
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>

// AzQtComponents
#include <AzQtComponents/Components/WindowDecorationWrapper.h>

AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include "ui_CustomizeKeyboardDialog.h"
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

using namespace AzQtComponents;

namespace
{
    enum CustomRole
    {
        ActionRole = Qt::UserRole,
        KeySequenceRole
    };
}

class NestedQAction
{
public:
    NestedQAction()
        : m_action(nullptr)
    {
    }
    NestedQAction(const QString& path, QAction* action)
        : m_path(path)
        , m_action(action)
    {
    }

    QString Path() const { return m_path; }
    QAction* Action() const { return m_action; }

private:
    QString m_path;
    QAction* m_action;
};

QVector<NestedQAction> GetAllActionsForMenu(const QMenu* menu, const QString& path)
{
    QList<QAction*> menuActions = menu->actions();
    QVector<NestedQAction> actions;
    actions.reserve(menuActions.size());
    foreach(QAction * action, menuActions)
    {
        if (action->menu() != nullptr)
        {
            QString newPath = path + action->text() + QStringLiteral(" | ");
            newPath = RemoveAcceleratorAmpersands(newPath);

            QVector<NestedQAction> subMenuActions = GetAllActionsForMenu(action->menu(), newPath);
            actions.reserve(actions.size() + subMenuActions.size());
            actions += subMenuActions;
        }
        else if (!action->isSeparator())
        {
            actions.push_back(NestedQAction(path + RemoveAcceleratorAmpersands(action->text()), action));
        }
    }

    return actions;
}

class MenuActionsModel
    : public QAbstractListModel
{
public:
    MenuActionsModel(QObject* parent = nullptr)
        : QAbstractListModel(parent)
    {
    }
    ~MenuActionsModel() override {}

    int rowCount([[maybe_unused]] const QModelIndex& parent = QModelIndex()) const override
    {
        return m_actions.size();
    }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
    {
        if (index.row() < 0 || index.row() >= m_actions.size())
        {
            return {};
        }

        switch (role)
        {
        case Qt::DisplayRole:
        {
            return m_actions[index.row()].Path();
        }
        case ActionRole:
            return QVariant::fromValue(m_actions[index.row()].Action());
        }

        return {};
    }

    void Reset(const QVector<NestedQAction>& actions)
    {
        beginResetModel();
        m_actions = actions;
        endResetModel();
    }

private:
    QVector<NestedQAction> m_actions;
};

class ActionShortcutsModel
    : public QAbstractListModel
{
public:
    ActionShortcutsModel(QObject* parent = nullptr)
        : QAbstractListModel(parent)
        , m_action(nullptr)
    {
    }
    ~ActionShortcutsModel() override {}

    int rowCount([[maybe_unused]] const QModelIndex& parent = QModelIndex()) const override
    {
        if (m_action)
        {
            return m_action->shortcuts().size();
        }
        else
        {
            return 0;
        }
    }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
    {
        auto shortcuts = m_action->shortcuts();
        if (index.row() < 0 || index.row() >= shortcuts.size())
        {
            return {};
        }

        switch (role)
        {
        case Qt::DisplayRole:
            return shortcuts.at(index.row()).toString();
        case KeySequenceRole:
            return QVariant::fromValue(shortcuts.at(index.row()));
        }

        return {};
    }

    void RemoveAll()
    {
        auto shortcuts = m_action->shortcuts();
        beginRemoveRows({}, 0, shortcuts.size() - 1);
        shortcuts.clear();
        m_action->setShortcuts(shortcuts);
        endRemoveRows();
    }

    void Remove(const QKeySequence& sequence)
    {
        auto shortcuts = m_action->shortcuts();
        int index = shortcuts.indexOf(sequence);
        if (index >= 0)
        {
            beginRemoveRows({}, index, index);
            shortcuts.removeAll(sequence);
            m_action->setShortcuts(shortcuts);
            endRemoveRows();
        }
    }

    QModelIndex Add(const QKeySequence& sequence)
    {
        auto shortcuts = m_action->shortcuts();
        int position = shortcuts.indexOf(sequence);
        if (-1 == position)
        {
            position = shortcuts.size();
            beginInsertRows({}, position, position);
            shortcuts.append(sequence);
            m_action->setShortcuts(shortcuts);
            endInsertRows();
        }
        return index(position);
    }

    bool Contains(const QKeySequence& sequence) const
    {
        return m_action->shortcuts().contains(sequence);
    }

    void Reset(QAction& action)
    {
        beginResetModel();
        m_action = &action;
        endResetModel();
    }

private:
    QAction* m_action;
};

CustomizeKeyboardDialog::CustomizeKeyboardDialog(KeyboardCustomizationSettings& settings, QWidget* parent /* = nullptr */)
    : QDialog(new WindowDecorationWrapper(WindowDecorationWrapper::OptionAutoAttach | WindowDecorationWrapper::OptionAutoTitleBarButtons, parent))
    , m_ui(new Ui::CustomizeKeyboardDialog)
    , m_settings(settings)
    , m_settingsSnapshot(m_settings.CreateSnapshot())
{
    m_ui->setupUi(this);

    m_menuActionsModel = new MenuActionsModel(this);
    m_actionShortcutsModel = new ActionShortcutsModel(this);
    m_ui->commandsView->setModel(m_menuActionsModel);
    m_ui->shortcutsView->setModel(m_actionShortcutsModel);

    QStringList categories = BuildModels(parent);

    connect(m_ui->categories, &QComboBox::currentTextChanged, this, &CustomizeKeyboardDialog::CategoryChanged);
    connect(m_ui->commandsView->selectionModel(), &QItemSelectionModel::currentChanged, this, &CustomizeKeyboardDialog::CommandSelectionChanged);
    connect(m_ui->shortcutsView->selectionModel(), &QItemSelectionModel::currentChanged, this, &CustomizeKeyboardDialog::ShortcutsViewSelectionChanged);
    connect(m_actionShortcutsModel, &QAbstractItemModel::rowsRemoved, this, &CustomizeKeyboardDialog::ShortcutsViewDataChanged);
    connect(m_actionShortcutsModel, &QAbstractItemModel::rowsInserted, this, &CustomizeKeyboardDialog::ShortcutsViewDataChanged);
    connect(m_ui->keySequenceEdit, &QKeySequenceEdit::editingFinished, this, &CustomizeKeyboardDialog::KeySequenceEditingFinished);
    connect(m_ui->assignButton, &QPushButton::clicked, this, &CustomizeKeyboardDialog::AssignButtonClicked);
    connect(m_ui->removeButton, &QPushButton::clicked, this, &CustomizeKeyboardDialog::ShortcutRemoved);
    connect(m_ui->clearButton, &QPushButton::clicked, m_actionShortcutsModel, &ActionShortcutsModel::RemoveAll);
    connect(m_ui->buttonBox, &QDialogButtonBox::clicked, this, &CustomizeKeyboardDialog::DialogButtonClicked);
    connect(this, &QDialog::rejected, this, [&]() { m_settings.Load(m_settingsSnapshot); });

    m_ui->categories->addItems(categories);
}

CustomizeKeyboardDialog::~CustomizeKeyboardDialog()
{
}

QStringList CustomizeKeyboardDialog::BuildModels(QWidget* parent)
{
    QMenuBar* menuBar = parent->findChild<QMenuBar*>();
    QList<QAction*> menuBarActions = menuBar->actions();

    QStringList categories;
    foreach(QAction * menuAction, menuBarActions)
    {
        QString category = RemoveAcceleratorAmpersands(menuAction->text());
        categories.append(category);

        QMenu* menu = menuAction->menu();
        m_menuActions[category] = GetAllActionsForMenu(menu, QString());
    }

    return categories;
}

void CustomizeKeyboardDialog::CategoryChanged(const QString& category)
{
    m_menuActionsModel->Reset(m_menuActions[category]);

    // Must reset the shortcut sequence text box back to disabled state
    // if the category is changed
    m_ui->keySequenceEdit->setEnabled(false);
    m_ui->commandsView->scrollToTop();
}

void CustomizeKeyboardDialog::CommandSelectionChanged(const QModelIndex& current, [[maybe_unused]] const QModelIndex& previous)
{
    QAction* action = current.data(ActionRole).value<QAction*>();
    m_actionShortcutsModel->Reset(*action);

    m_ui->removeButton->setEnabled(false);
    m_ui->clearButton->setEnabled(m_actionShortcutsModel->rowCount() > 0);
    m_ui->keySequenceEdit->setEnabled(true);

    const auto& description = action->statusTip().size() > 0 ? action->statusTip() : action->toolTip();
    m_ui->descriptionLabel->setText(description);
    m_ui->keySequenceEdit->clear();
}

void CustomizeKeyboardDialog::ShortcutsViewSelectionChanged(const QModelIndex& current, [[maybe_unused]] const QModelIndex& previous)
{
    m_ui->removeButton->setEnabled(current.isValid());
}

void CustomizeKeyboardDialog::ShortcutsViewDataChanged()
{
    m_ui->clearButton->setEnabled(m_actionShortcutsModel->rowCount() > 0);
}

void CustomizeKeyboardDialog::ShortcutRemoved()
{
    auto index = m_ui->shortcutsView->selectionModel()->selectedIndexes().first();
    m_actionShortcutsModel->Remove(index.data(KeySequenceRole).value<QKeySequence>());
}

void CustomizeKeyboardDialog::KeySequenceEditingFinished()
{
    auto keySequence = m_ui->keySequenceEdit->keySequence();
    m_ui->assignButton->setEnabled(!keySequence.isEmpty() && !m_actionShortcutsModel->Contains(keySequence));
}

void CustomizeKeyboardDialog::AssignButtonClicked()
{
    auto sequence = m_ui->keySequenceEdit->keySequence();
    m_ui->keySequenceEdit->clear();

    auto currentAction = m_settings.FindActionForShortcut(sequence);
    if (currentAction)
    {
        auto result = QMessageBox::warning(
                this,
                tr("Shortcut already in use"),
                tr("%1 is currently assigned to '%2'.\n\nAssign and replace?")
                    .arg(sequence.toString()).arg(RemoveAcceleratorAmpersands(currentAction->text())),
                QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No);
        if (result == QMessageBox::No)
        {
            m_ui->keySequenceEdit->setFocus();
            return;
        }

        //remove this sequence from the current shortcut
        auto shortcuts = currentAction->shortcuts();
        shortcuts.removeAll(sequence);
        currentAction->setShortcuts(shortcuts);
    }

    QModelIndex index = m_actionShortcutsModel->Add(sequence);
    m_ui->shortcutsView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::Clear | QItemSelectionModel::SelectCurrent);
    m_ui->assignButton->setEnabled(false);
    m_ui->removeButton->setFocus();
}

void CustomizeKeyboardDialog::DialogButtonClicked(const QAbstractButton* button)
{
    if (button == m_ui->buttonBox->button(QDialogButtonBox::RestoreDefaults))
    {
        auto result = QMessageBox::question(
                this,
                tr("Restore Default Keyboard Shortcuts"),
                tr("Are you sure you wish to restore all keyboard shortcuts to factory defaults?"));
        if (result == QMessageBox::Yes)
        {
            m_settings.LoadDefaults();
        }
    }
    else if (button == m_ui->buttonBox->button(QDialogButtonBox::Close))
    {
        m_settings.Save();
        accept();
    }
    else if (button == m_ui->buttonBox->button(QDialogButtonBox::Cancel))
    {
        m_settings.Load(m_settingsSnapshot);
        reject();
    }
}

#include <moc_CustomizeKeyboardDialog.cpp>
