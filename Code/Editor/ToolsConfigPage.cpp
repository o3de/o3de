/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "ToolsConfigPage.h"

// Qt
#include <QCompleter>
#include <QMessageBox>

// AzToolsFramework
#include <AzToolsFramework/API/EditorPythonConsoleBus.h>

// AzQtComponents
#include <AzQtComponents/Components/Widgets/TabWidget.h>

// Editor
#include "Settings.h"
#include "MainWindow.h"
#include "ToolBox.h"


AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <ui_ToolsConfigPage.h>
#include <ui_IconListDialog.h>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

namespace
{
    QColor COLOR_FOR_EDITOR_COMMAND = QColor(0, 255, 0);
    QColor COLOR_FOR_CONSOLE_COMMAND = QColor(0, 0, 255);
    QColor COLOR_FOR_TOGGLE_COMMAND = QColor(128, 0, 255);
    QColor COLOR_FOR_INVALID_COMMAND = QColor(255, 0, 0);
};

class IconListModel
    : public QAbstractListModel
{
public:
    IconListModel(QObject* parent = nullptr)
        : QAbstractListModel(parent)
    {
        IFileUtil::FileArray pngFiles;

        // Search for the png files in Editor/UI/Icons folder
        // and add them to the image list.
        const int iconSize = 32;    // Currently, accepts the image of this size only.
        if (gSettings.searchPaths[EDITOR_PATH_UI_ICONS].empty())
        {
            return;
        }
        const QString iconsDir = gSettings.searchPaths[EDITOR_PATH_UI_ICONS][0];
        CFileUtil::ScanDirectory(iconsDir, "*.png", pngFiles);
        m_iconImages.reserve(static_cast<int>(pngFiles.size()));
        m_iconFiles.reserve(static_cast<int>(pngFiles.size()));
        for (size_t i = 0; i < pngFiles.size(); ++i)
        {
            const QString path = Path::Make(iconsDir, pngFiles[i].filename);
            QPixmap bm(path);
            if (bm.isNull())
            {
                continue;
            }
            if (bm.width() == iconSize && bm.height() == iconSize)
            {
                QIcon icon(bm);
                icon.addPixmap(bm, QIcon::Selected);
                m_iconImages.push_back(icon);
                m_iconFiles.push_back(path);
            }
        }
    }

    int rowCount(const QModelIndex& parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : m_iconImages.count();
    }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
    {
        if (!index.isValid() || index.row() >= rowCount(index.parent()))
        {
            return QVariant();
        }

        switch (role)
        {
        case Qt::DisplayRole:
            return m_iconFiles[index.row()];
        case Qt::DecorationRole:
            return m_iconImages[index.row()];
        default:
            return QVariant();
        }
    }

private:
    QVector<QIcon> m_iconImages;
    QStringList m_iconFiles;
};

CIconListDialog::CIconListDialog(QWidget* pParent /* = nullptr */)
    : QDialog(pParent)
    , m_ui(new Ui::IconListDialog)
{
    m_ui->setupUi(this);
    m_ui->m_iconListCtrl->setModel(new IconListModel(this));
}

bool CIconListDialog::GetSelectedIconPath(QString& path) const
{
    if (!m_ui->m_iconListCtrl->currentIndex().isValid())
    {
        return false;
    }
    path = m_ui->m_iconListCtrl->currentIndex().data().toString();
    return true;
}

class CommandModel
    : public QAbstractListModel
{
public:
    CommandModel(QObject* parent = nullptr)
        : QAbstractListModel(parent)
    {
    }

    void setMacroIndex(const QModelIndex& macroIndex)
    {
        if (m_macroIndex == macroIndex)
        {
            return;
        }
        beginResetModel();
        m_macroIndex = macroIndex;
        endResetModel();
    }

    bool addRow()
    {
        if (macro() == nullptr)
        {
            return false;
        }
        beginInsertRows(QModelIndex(), rowCount(), rowCount());
        macro()->AddCommand(CToolBoxCommand::eT_INVALID_COMMAND, "nop");
        endInsertRows();
        return true;
    }

    bool moveRow(int row, bool up)
    {
        const int targetRow = up ? row - 1 : row + 1;
        if (row < 0 || row >= rowCount() || targetRow < 0 || targetRow >= rowCount())
        {
            return false;
        }
        if (up)
        {
            beginMoveRows(QModelIndex(), row, row, QModelIndex(), row - 1);
        }
        else
        {
            beginMoveRows(QModelIndex(), row + 1, row + 1, QModelIndex(), row);
        }
        macro()->SwapCommand(row, targetRow);
        endMoveRows();
        return true;
    }

    bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override
    {
        if (row < 0 || row + count - 1 >= rowCount(parent))
        {
            return false;
        }

        beginRemoveRows(QModelIndex(), row, row + count - 1);
        for (int r = row + count - 1; r >= row; --r)
        {
            macro()->RemoveCommand(r);
        }
        endRemoveRows();
        return true;
    }

    int rowCount(const QModelIndex& parent = QModelIndex()) const override
    {
        return parent.isValid() || macro() == nullptr ? 0 : macro()->GetCommandCount();
    }

    bool setData(const QModelIndex& index, [[maybe_unused]] const QVariant& value, int role = Qt::EditRole) override
    {
        if (!index.isValid() || index.row() >= rowCount(index.parent()))
        {
            return false;
        }

        if (role != Qt::UserRole)
        {
            return false;
        }

        // this has already been set
        emit dataChanged(index, index);
        return true;
    }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
    {
        if (!index.isValid() || index.row() >= rowCount(index.parent()))
        {
            return QVariant();
        }

        auto command = macro()->GetCommandAt(index.row());
        switch (role)
        {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return command->m_text;
        case Qt::ForegroundRole:
            switch (command->m_type)
            {
            case CToolBoxCommand::eT_SCRIPT_COMMAND:
                return QVariant::fromValue(COLOR_FOR_EDITOR_COMMAND);
            case CToolBoxCommand::eT_CONSOLE_COMMAND:
                return command->m_bVariableToggle ? QVariant::fromValue(COLOR_FOR_TOGGLE_COMMAND) : QVariant::fromValue(COLOR_FOR_CONSOLE_COMMAND);
            default:
                return QVariant::fromValue(COLOR_FOR_INVALID_COMMAND);
            }
        case Qt::UserRole:
            return QVariant::fromValue(command);
        default:
            return QVariant();
        }
    }

private:
    CToolBoxMacro* macro() const
    {
        return m_macroIndex.data(Qt::UserRole).value<CToolBoxMacro*>();
    }

    QPersistentModelIndex m_macroIndex;
};

class MacroModel
    : public QAbstractListModel
{
public:
    MacroModel(QObject* parent = nullptr)
        : QAbstractListModel(parent)
        , m_hasEmptyRow(false)
        , m_currentlyRemovingRows(false)
    {
    }

    bool moveRow(int row, bool up)
    {
        if (m_hasEmptyRow)
        {
            return false;
        }

        const int targetRow = up ? row - 1 : row + 1;
        if (row < 0 || row >= rowCount() || targetRow < 0 || targetRow >= rowCount())
        {
            return false;
        }
        if (up)
        {
            beginMoveRows(QModelIndex(), row, row, QModelIndex(), row - 1);
        }
        else
        {
            beginMoveRows(QModelIndex(), row + 1, row + 1, QModelIndex(), row);
        }
        GetIEditor()->GetToolBoxManager()->SwapMacro(row, targetRow, true);
        endMoveRows();
        return true;
    }

    int rowCount(const QModelIndex& parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : (GetIEditor()->GetToolBoxManager()->GetMacroCount(true) + (m_hasEmptyRow ? 1 : 0));
    }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
    {
        if (!index.isValid() || index.row() >= rowCount(index.parent()))
        {
            return QVariant();
        }

        if (isEmptyRow(index))
        {
            switch (role)
            {
            case Qt::DisplayRole:
            case Qt::EditRole:
                return QString();
            default:
                return QVariant();
            }
        }

        auto macro = GetIEditor()->GetToolBoxManager()->GetMacro(index.row(), true);

        switch (role)
        {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return macro->GetTitle();
        case Qt::UserRole:
            return QVariant::fromValue(macro);
        default:
            return QVariant();
        }
    }

    bool setData(const QModelIndex& index, const QVariant& data, [[maybe_unused]] int role = Qt::EditRole) override
    {
        if (!index.isValid() || index.row() >= rowCount(index.parent()))
        {
            return false;
        }

        // Retrieve the dialog parent widget for this model so we can display
        // error popups properly (if needed)
        QWidget* parentWidget = qobject_cast<QWidget*>(parent());

        // isNull and isValid in Qvariant/QString in this case cannot detect null input
        // check null data input
        if (data.toString().isEmpty())
        {
            if (!m_currentlyRemovingRows)
            {
                QMessageBox::critical(parentWidget, QString(), tr("Please enter a valid name!"));

                // If this is a newly added empty row, then just delete it
                // Otherwise if the user was renaming an existing row, the previous
                // value will be restored
                if (isEmptyRow(index))
                {
                    removeRow(index.row());
                    assert(!m_hasEmptyRow);
                }
            }

            return false;
        }

        if (isEmptyRow(index))
        {
            auto macro = GetIEditor()->GetToolBoxManager()->NewMacro(data.toString(), true, nullptr);
            if (macro == nullptr)
            {
                QMessageBox::critical(parentWidget, QString(), tr("There is a macro of that name, already!"));

                removeRow(index.row());
                assert(!m_hasEmptyRow);
                return false;
            }

            emit dataChanged(index, index);
            m_hasEmptyRow = false;
        }
        else if (GetIEditor()->GetToolBoxManager()->SetMacroTitle(index.row(), data.toString(), true))
        {
            emit dataChanged(index, index);
            return true;
        }
        else
        {
            QMessageBox::critical(parentWidget, QString(), tr("There is a macro of that name, already!"));
        }

        return false;
    }

    Qt::ItemFlags flags(const QModelIndex& index) const override
    {
        return QAbstractListModel::flags(index) | Qt::ItemIsEditable;
    }

    bool addRow()
    {
        if (m_hasEmptyRow)
        {
            return false;
        }
        beginInsertRows(QModelIndex(), rowCount(), rowCount());
        m_hasEmptyRow = true;
        endInsertRows();
        return true;
    }

    bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override
    {
        if (row < 0 || row + count - 1 >= rowCount(parent))
        {
            return false;
        }

        m_currentlyRemovingRows = true;
        beginRemoveRows(QModelIndex(), row, row + count - 1);

        auto tools = GetIEditor()->GetToolBoxManager();
        for (int r = row + count - 1; r >= row; --r)
        {
            if (r == rowCount() - 1 && m_hasEmptyRow)
            {
                m_hasEmptyRow = false;
            }
            else
            {
                tools->RemoveMacro(r, true);
            }
        }

        endRemoveRows();
        m_currentlyRemovingRows = false;
        return true;
    }

private:
    bool m_hasEmptyRow;
    bool m_currentlyRemovingRows;

    // Empty row is the last row in the list if the proper flag is set
    bool isEmptyRow(const QModelIndex& index) const
    {
        return m_hasEmptyRow && index.row() == rowCount() - 1;
    }
};

// CToolsConfigPage dialog

ToolsConfigDialog::ToolsConfigDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Configure ToolBox Macros"));
    setLayout(new QVBoxLayout);
    AzQtComponents::TabWidget* tabs = new AzQtComponents::TabWidget;
    AzQtComponents::TabWidget::applySecondaryStyle(tabs, false);
    layout()->addWidget(tabs);
    CToolsConfigPage* page = new CToolsConfigPage;
    tabs->addTab(page, page->windowTitle());
    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout()->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, page, &CToolsConfigPage::OnOK);
    connect(buttons, &QDialogButtonBox::rejected, page, &CToolsConfigPage::OnCancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void ToolsConfigDialog::reject()
{
    // Revert to the original.
    GetIEditor()->GetToolBoxManager()->Load();
    QDialog::reject();
}

void ToolsConfigDialog::closeEvent(QCloseEvent* ev)
{
    reject();
    QDialog::closeEvent(ev);
}

CToolsConfigPage::CToolsConfigPage(QWidget* parent)
    : QWidget(parent)
    , m_macroModel(new MacroModel(this))
    , m_commandModel(new CommandModel(this))
    , m_completionModel(new QStringListModel(this))
    , m_ui(new Ui::ToolsConfigPage)
{
    m_ui->setupUi(this);

    m_ui->m_macroCmd->setCompleter(new QCompleter(m_completionModel));

    OnInitDialog();
}

CToolsConfigPage::~CToolsConfigPage()
{
}

// CToolsConfigPage message handlers
void CToolsConfigPage::OnInitDialog()
{
    m_ui->m_macroList->setModel(m_macroModel);
    m_ui->m_commandList->setModel(m_commandModel);

    connect(m_ui->m_assignCommand, &QPushButton::clicked, this, &CToolsConfigPage::OnAssignCommand);
    connect(m_ui->m_selectIcon, &QPushButton::clicked, this, &CToolsConfigPage::OnSelectMacroIcon);
    connect(m_ui->m_clearIcon, &QPushButton::clicked, this, &CToolsConfigPage::OnClearMacroIcon);
    connect(m_ui->m_console, &QRadioButton::clicked, this, &CToolsConfigPage::OnConsoleCmd);
    connect(m_ui->m_script, &QRadioButton::clicked, this, &CToolsConfigPage::OnScriptCmd);

    connect(m_ui->m_macroList->selectionModel(), &QItemSelectionModel::currentChanged, this, &CToolsConfigPage::OnSelchangeMacroList);
    connect(m_ui->m_buttonMacroNew, &QToolButton::clicked, this, &CToolsConfigPage::OnNewMacroItem);
    connect(m_ui->m_buttonMacroUp, &QToolButton::clicked, this, &CToolsConfigPage::OnMoveMacroItemUp);
    connect(m_ui->m_buttonMacroDown, &QToolButton::clicked, this, &CToolsConfigPage::OnMoveMacroItemDown);
    connect(m_ui->m_buttonMacroDelete, &QToolButton::clicked, this, &CToolsConfigPage::OnDeleteMacroItem);

    connect(m_ui->m_commandList->selectionModel(), &QItemSelectionModel::currentChanged, this, &CToolsConfigPage::OnSelchangeCommandList);
    connect(m_ui->m_buttonCommandNew, &QToolButton::clicked, this, &CToolsConfigPage::OnNewCommandItem);
    connect(m_ui->m_buttonCommandUp, &QToolButton::clicked, this, &CToolsConfigPage::OnMoveCommandItemUp);
    connect(m_ui->m_buttonCommandDown, &QToolButton::clicked, this, &CToolsConfigPage::OnMoveCommandItemDown);
    connect(m_ui->m_buttonCommandDelete, &QToolButton::clicked, this, &CToolsConfigPage::OnDeleteCommandItem);

    m_consoleOrScript = 1;  // To force the change in the next line
    OnConsoleCmd();

    // To ensure the proper disabling of controls
    OnSelchangeMacroList();
    OnSelchangeCommandList();
}
//////////////////////////////////////////////////////////////////////////
void CToolsConfigPage::OnOK()
{
    GetIEditor()->GetToolBoxManager()->Save();
}

//////////////////////////////////////////////////////////////////////////
void CToolsConfigPage::OnCancel()
{
    // Revert to the original.
    GetIEditor()->GetToolBoxManager()->Load();
}

//////////////////////////////////////////////////////////////////////////
void CToolsConfigPage::OnSelchangeMacroList()
{
    /// Update the command list.
    auto macro = m_ui->m_macroList->currentIndex().data(Qt::UserRole).value<CToolBoxMacro*>();
    m_commandModel->setMacroIndex(m_ui->m_macroList->currentIndex());

    if (m_ui->m_macroList->currentIndex().isValid())
    {
        /// Update the icon.
        const QPixmap icon(macro != nullptr ? macro->GetIconPath() : QString());
        m_ui->m_macroIcon->setPixmap(icon);

        m_ui->m_selectIcon->setEnabled(true);
        m_ui->m_clearIcon->setEnabled(true);
    }
    else
    {
        m_ui->m_selectIcon->setEnabled(false);
        m_ui->m_clearIcon->setEnabled(false);
    }

    m_ui->m_commandList->selectionModel()->clear();
    OnSelchangeCommandList();
}

//////////////////////////////////////////////////////////////////////////
void CToolsConfigPage::OnNewMacroItem()
{
    if (m_macroModel->addRow())
    {
        const QModelIndex index = m_macroModel->index(m_macroModel->rowCount() - 1, 0);
        m_ui->m_macroList->setCurrentIndex(index);
        m_ui->m_macroList->edit(index);
    }
}

//////////////////////////////////////////////////////////////////////////
void CToolsConfigPage::OnSelchangeCommandList()
{
    auto commandIndex = m_ui->m_commandList->currentIndex();

    if (!commandIndex.isValid())
    {
        m_ui->m_assignCommand->setEnabled(false);
        m_ui->m_macroCmd->setEnabled(false);
        m_ui->m_macroCmd->clear();
        m_ui->m_toggleVar->setEnabled(false);
        m_ui->m_toggleVar->setChecked(false);
        m_ui->m_console->setEnabled(false);
        m_ui->m_script->setEnabled(false);
    }
    else
    {
        m_ui->m_assignCommand->setEnabled(true);
        m_ui->m_macroCmd->setEnabled(true);
        m_ui->m_console->setEnabled(true);
        m_ui->m_script->setEnabled(true);
        CToolBoxCommand* pCommand = commandIndex.data(Qt::UserRole).value<CToolBoxCommand*>();
        if (pCommand->m_type == CToolBoxCommand::eT_SCRIPT_COMMAND)
        {
            m_ui->m_macroCmd->setText(pCommand->m_text);
            OnScriptCmd();
        }
        else if (pCommand->m_type == CToolBoxCommand::eT_CONSOLE_COMMAND)
        {
            m_ui->m_macroCmd->setText(pCommand->m_text);
            OnConsoleCmd();
            m_ui->m_toggleVar->setChecked(pCommand->m_bVariableToggle);
            m_ui->m_toggleVar->setEnabled(true);
        }
        else
        {
            m_ui->m_macroCmd->clear();
            OnConsoleCmd();
            m_ui->m_toggleVar->setChecked(false);
            m_ui->m_toggleVar->setEnabled(true);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CToolsConfigPage::OnNewCommandItem()
{
    if (m_commandModel->addRow())
    {
        m_ui->m_commandList->setCurrentIndex(m_commandModel->index(m_commandModel->rowCount() - 1));
    }
}

//////////////////////////////////////////////////////////////////////////
void CToolsConfigPage::OnAssignCommand()
{
    auto commandIndex = m_ui->m_commandList->currentIndex();
    if (commandIndex.isValid())
    {
        CToolBoxCommand* pCommand = commandIndex.data(Qt::UserRole).value<CToolBoxCommand*>();
        pCommand->m_type = m_consoleOrScript ? CToolBoxCommand::eT_SCRIPT_COMMAND
            : CToolBoxCommand::eT_CONSOLE_COMMAND;
        pCommand->m_text = m_ui->m_macroCmd->text();
        if (pCommand->m_type == CToolBoxCommand::eT_CONSOLE_COMMAND)
        {
            pCommand->m_bVariableToggle = m_ui->m_toggleVar->isChecked();
        }
        else
        {
            pCommand->m_bVariableToggle = false;
        }

        m_commandModel->setData(commandIndex, QVariant::fromValue(pCommand), Qt::UserRole);
    }
}

//////////////////////////////////////////////////////////////////////////
void CToolsConfigPage::OnMoveMacroItemUp()
{
    auto macroIndex = m_ui->m_macroList->currentIndex();
    m_macroModel->moveRow(macroIndex.row(), true);
}

//////////////////////////////////////////////////////////////////////////
void CToolsConfigPage::OnMoveMacroItemDown()
{
    auto macroIndex = m_ui->m_macroList->currentIndex();
    m_macroModel->moveRow(macroIndex.row(), false);
}

//////////////////////////////////////////////////////////////////////////
void CToolsConfigPage::OnMoveCommandItemUp()
{
    auto commandIndex = m_ui->m_commandList->currentIndex();
    m_commandModel->moveRow(commandIndex.row(), true);
}

//////////////////////////////////////////////////////////////////////////
void CToolsConfigPage::OnMoveCommandItemDown()
{
    auto commandIndex = m_ui->m_commandList->currentIndex();
    m_commandModel->moveRow(commandIndex.row(), false);
}

//////////////////////////////////////////////////////////////////////////
void CToolsConfigPage::OnDeleteMacroItem()
{
    m_macroModel->removeRow(m_ui->m_macroList->currentIndex().row());
}

//////////////////////////////////////////////////////////////////////////
void CToolsConfigPage::OnDeleteCommandItem()
{
    m_commandModel->removeRow(m_ui->m_commandList->currentIndex().row());
}

//////////////////////////////////////////////////////////////////////////
void CToolsConfigPage::OnSelectMacroIcon()
{
    auto macroIndex = m_ui->m_macroList->currentIndex();
    if (!macroIndex.isValid())
    {
        return;
    }

    CToolBoxMacro* pMacro = macroIndex.data(Qt::UserRole).value<CToolBoxMacro*>();

    CIconListDialog iconListDlg;
    if (iconListDlg.exec() == QDialog::Accepted)
    {
        QString iconPath;
        if (iconListDlg.GetSelectedIconPath(iconPath))
        {
            const QPixmap pixmap(iconPath);
            m_ui->m_macroIcon->setPixmap(pixmap);
            pMacro->SetIconPath(iconPath.toUtf8().data());
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CToolsConfigPage::OnClearMacroIcon()
{
    auto macroIndex = m_ui->m_macroList->currentIndex();
    if (!macroIndex.isValid())
    {
        return;
    }

    CToolBoxMacro* pMacro = macroIndex.data(Qt::UserRole).value<CToolBoxMacro*>();
    m_ui->m_macroIcon->setPixmap(QPixmap());
    pMacro->SetIconPath("");
}

//////////////////////////////////////////////////////////////////////////
void CToolsConfigPage::FillConsoleCmds()
{
    QStringList commands;
    IConsole* console = GetIEditor()->GetSystem()->GetIConsole();
    AZStd::vector<AZStd::string_view> cmds;
    cmds.resize(console->GetNumVars());
    size_t cmdCount = console->GetSortedVars(cmds);
    for (int i = 0; i < cmdCount; ++i)
    {
        commands.push_back(cmds[i].data());
    }
    m_completionModel->setStringList(commands);
}

//////////////////////////////////////////////////////////////////////////
void CToolsConfigPage::FillScriptCmds()
{
    // Add module names to the auto-completion list.
    QStringList commands;

    using namespace AzToolsFramework;
    EditorPythonConsoleInterface* editorPythonConsoleInterface = AZ::Interface<EditorPythonConsoleInterface>::Get();
    if (editorPythonConsoleInterface)
    {
        EditorPythonConsoleInterface::GlobalFunctionCollection globalFunctionCollection;
        editorPythonConsoleInterface->GetGlobalFunctionList(globalFunctionCollection);
        commands.reserve(static_cast<int>(globalFunctionCollection.size()));
        for (const EditorPythonConsoleInterface::GlobalFunction& globalFunction : globalFunctionCollection)
        {
            const QString fullCmd = QString("%1.%2()").arg(globalFunction.m_moduleName.data()).arg(globalFunction.m_functionName.data());
            commands.push_back(fullCmd);
        }
    }

    m_completionModel->setStringList(commands);
}

//////////////////////////////////////////////////////////////////////////
void CToolsConfigPage::OnConsoleCmd()
{
    m_ui->m_console->setChecked(true);
    if (m_consoleOrScript == 0)
    {
        return;
    }

    m_consoleOrScript = 0;
    FillConsoleCmds();
    m_ui->m_toggleVar->setEnabled(true);
    m_ui->m_toggleVar->setChecked(false);
}

//////////////////////////////////////////////////////////////////////////
void CToolsConfigPage::OnScriptCmd()
{
    m_ui->m_script->setChecked(true);
    if (m_consoleOrScript == 1)
    {
        return;
    }

    m_consoleOrScript = 1;
    FillScriptCmds();
    m_ui->m_toggleVar->setEnabled(false);
    m_ui->m_toggleVar->setChecked(false);
}

#include <moc_ToolsConfigPage.cpp>
