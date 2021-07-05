/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : For listing available script commands with their descriptions

#include "ScriptHelpDialog.h"

#include <array>

// Qt
#include <QClipboard>
#include <QApplication>
#include <QLineEdit>

// AzQtComponents
#include <AzQtComponents/Components/Widgets/LineEdit.h>

// AzToolsFramework
#include <AzToolsFramework/API/EditorPythonConsoleBus.h>    // for EditorPythonConsoleInterface

AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
#include <AzToolsFramework/PythonTerminal/ui_ScriptHelpDialog.h>
AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING

namespace AzToolsFramework
{
    HeaderView::HeaderView(QWidget* parent)
        : QHeaderView(Qt::Horizontal, parent)
        , m_commandFilter(new QLineEdit(this))
        , m_moduleFilter(new QLineEdit(this))
        , m_descriptionFilter(new QLineEdit(this))
        , m_exampleFilter(new QLineEdit(this))
    {
        // Allow the header sections to be clickable so we can change change the
        // sort order on click
        setSectionsClickable(true);

        connect(this, &QHeaderView::geometriesChanged, this, &HeaderView::repositionLineEdits);
        connect(this, &QHeaderView::sectionMoved, this, &HeaderView::repositionLineEdits);
        connect(this, &QHeaderView::sectionResized, this, &HeaderView::repositionLineEdits);
        connect(m_commandFilter, &QLineEdit::textChanged, this, &HeaderView::commandFilterChanged);
        connect(m_moduleFilter, &QLineEdit::textChanged, this, &HeaderView::moduleFilterChanged);
        connect(m_descriptionFilter, &QLineEdit::textChanged, this, &HeaderView::descriptionFilterChanged);
        connect(m_exampleFilter, &QLineEdit::textChanged, this, &HeaderView::exampleFilterChanged);

        AzQtComponents::LineEdit::applySearchStyle(m_commandFilter);
        AzQtComponents::LineEdit::applySearchStyle(m_moduleFilter);
        AzQtComponents::LineEdit::applySearchStyle(m_descriptionFilter);
        AzQtComponents::LineEdit::applySearchStyle(m_exampleFilter);

        // Calculate our height offset to embed our line edits in the header
        const int margins = frameWidth() * 2 + 1;
        m_lineEditHeightOffset = m_commandFilter->sizeHint().height() + margins;
    }

    QSize HeaderView::sizeHint() const
    {
        // Adjust our height to include the line edit offset
        QSize size = QHeaderView::sizeHint();
        size.setHeight(size.height() + m_lineEditHeightOffset);
        return size;
    }

    void HeaderView::resizeEvent(QResizeEvent* ev)
    {
        QHeaderView::resizeEvent(ev);
        repositionLineEdits();
    }

    void HeaderView::repositionLineEdits()
    {
        const int headerHeight = sizeHint().height();
        const int lineEditYPos = headerHeight - m_lineEditHeightOffset;
        const int col0Width = sectionSize(0);
        const int col1Width = sectionSize(1);
        const int col2Width = sectionSize(2);
        const int col3Width = sectionSize(3);
        const int adjustment = 2;

        if (col0Width <= adjustment || col1Width <= adjustment)
        {
            return;
        }

        m_commandFilter->setFixedWidth(col0Width - adjustment);
        m_moduleFilter->setFixedWidth(col1Width - adjustment);
        m_descriptionFilter->setFixedWidth(col2Width - adjustment);
        m_exampleFilter->setFixedWidth(col3Width - adjustment);

        m_commandFilter->move(1, lineEditYPos);
        m_moduleFilter->move(col0Width + 1, lineEditYPos);
        m_descriptionFilter->move(col0Width + col1Width + 1, lineEditYPos);
        m_exampleFilter->move(col0Width + col1Width + col2Width + 1, lineEditYPos);

        m_commandFilter->show();
        m_moduleFilter->show();
        m_descriptionFilter->show();
        // The example field is currently unused so will always be empty. Remove this when examples are added.
        m_exampleFilter->hide();
    }

    ScriptHelpProxyModel::ScriptHelpProxyModel(QObject* parent)
        : QSortFilterProxyModel(parent)
    {
    }

    bool ScriptHelpProxyModel::filterAcceptsRow(int source_row, [[maybe_unused]] const QModelIndex& source_parent) const
    {
        if (!sourceModel())
        {
            return false;
        }

        const QString command = sourceModel()->index(source_row, ScriptHelpModel::ColumnCommand).data(Qt::DisplayRole).toString().toLower();
        const QString module = sourceModel()->index(source_row, ScriptHelpModel::ColumnModule).data(Qt::DisplayRole).toString().toLower();
        const QString description = sourceModel()->index(source_row, ScriptHelpModel::ColumnDescription).data(Qt::DisplayRole).toString().toLower();

        return command.contains(m_commandFilter) && module.contains(m_moduleFilter) && description.contains(m_descriptionFilter);
    }

    void ScriptHelpProxyModel::setCommandFilter(const QString& text)
    {
        const QString lowerText = text.toLower();
        if (m_commandFilter != lowerText)
        {
            m_commandFilter = lowerText;
            invalidateFilter();
        }
    }

    void ScriptHelpProxyModel::setModuleFilter(const QString& text)
    {
        const QString lowerText = text.toLower();
        if (m_moduleFilter != lowerText)
        {
            m_moduleFilter = lowerText;
            invalidateFilter();
        }
    }

    void ScriptHelpProxyModel::setDescriptionFilter(const QString& text)
    {
        const QString lowerText = text.toLower();
        if (m_descriptionFilter != lowerText)
        {
            m_descriptionFilter = lowerText;
            invalidateFilter();
        }
    }

    void ScriptHelpProxyModel::setExampleFilter(const QString& text)
    {
        const QString lowerText = text.toLower();
        if (m_exampleFilter != lowerText)
        {
            m_exampleFilter = lowerText;
            invalidateFilter();
        }
    }

    ScriptHelpModel::ScriptHelpModel(QObject* parent)
        : QAbstractTableModel(parent)
    {
    }

    QVariant ScriptHelpModel::data(const QModelIndex& index, int role) const
    {
        if (index.row() < 0 || index.row() >= rowCount() || index.column() < 0 || index.column() >= ColumnCount)
        {
            return QVariant();
        }

        const int col = index.column();
        const Item& item = m_items[index.row()];

        if (role == Qt::DisplayRole)
        {
            if (col == ColumnCommand)
            {
                return item.command;
            }
            else if (col == ColumnModule)
            {
                return item.module;
            }
            else if (col == ColumnDescription)
            {
                return item.description;
            }
        }

        return QVariant();
    }

    int ScriptHelpModel::rowCount(const QModelIndex& parent) const
    {
        if (parent.isValid())
        {
            return 0;
        }
        return m_items.size();
    }

    int ScriptHelpModel::columnCount(const QModelIndex& parent) const
    {
        if (parent.isValid())
        {
            return 0;
        }
        return ColumnCount;
    }

    Qt::ItemFlags ScriptHelpModel::flags([[maybe_unused]] const QModelIndex& index) const
    {
        return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    }

    QVariant ScriptHelpModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (section < 0 || section >= ColumnCount || orientation == Qt::Vertical)
        {
            return QVariant();
        }

        if (role == Qt::DisplayRole)
        {
            static const QStringList headers = { tr("Command"), tr("Module"), tr("Description"), tr("Example") };
            return headers.at(section);
        }
        else if (role == Qt::TextAlignmentRole)
        {
            return Qt::AlignLeft;
        }

        return QAbstractTableModel::headerData(section, orientation, role);
    }

    void ScriptHelpModel::Reload()
    {
        beginResetModel();

        using namespace AzToolsFramework;
        EditorPythonConsoleInterface* editorPythonConsoleInterface = AZ::Interface<EditorPythonConsoleInterface>::Get();
        if (editorPythonConsoleInterface)
        {
            EditorPythonConsoleInterface::GlobalFunctionCollection globalFunctionCollection;
            editorPythonConsoleInterface->GetGlobalFunctionList(globalFunctionCollection);
            m_items.reserve(globalFunctionCollection.size());
            for (const EditorPythonConsoleInterface::GlobalFunction& globalFunction : globalFunctionCollection)
            {
                Item item;
                item.command = globalFunction.m_functionName.data();
                item.module = globalFunction.m_moduleName.data();
                item.description = globalFunction.m_description.data();
                m_items.push_back(item);
            }
        }

        endResetModel();
    }

    ScriptTableView::ScriptTableView(QWidget* parent)
        : QTableView(parent)
        , m_model(new ScriptHelpModel(this))
        , m_proxyModel(new ScriptHelpProxyModel(parent))
    {
        HeaderView* headerView = new HeaderView(this);
        setHorizontalHeader(headerView); // our header view embeds filter line edits
        QObject::connect(headerView, SIGNAL(sectionPressed(int)), this, SLOT(sortByColumn(int)));

        m_proxyModel->setSourceModel(m_model);
        setModel(m_proxyModel);
        m_model->Reload();
        setSortingEnabled(true);
        horizontalHeader()->setSortIndicatorShown(false);
        horizontalHeader()->setStretchLastSection(true);
        setSelectionBehavior(QAbstractItemView::SelectRows);
        setSelectionMode(QAbstractItemView::ContiguousSelection); // Not very useful for this dialog, but the MFC code allowed to select many rows

        static const std::array<int, ScriptHelpModel::ColumnCount> colWidths = { { 100, 60, 300 } };
        for (int col = 0; col < ScriptHelpModel::ColumnCount; ++col)
        {
            setColumnWidth(col, colWidths[col]);
        }

        setAlternatingRowColors(true);

        connect(headerView, &HeaderView::commandFilterChanged,
            m_proxyModel, &ScriptHelpProxyModel::setCommandFilter);
        connect(headerView, &HeaderView::moduleFilterChanged,
            m_proxyModel, &ScriptHelpProxyModel::setModuleFilter);
        connect(headerView, &HeaderView::descriptionFilterChanged,
            m_proxyModel, &ScriptHelpProxyModel::setDescriptionFilter);
        connect(headerView, &HeaderView::exampleFilterChanged,
            m_proxyModel, &ScriptHelpProxyModel::setExampleFilter);
    }

    CScriptHelpDialog::CScriptHelpDialog(QWidget* parent)
        : QDialog(parent)
    {
        ui.reset(new Ui::ScriptDialog);
        ui->setupUi(this);
        setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
        setWindowTitle(tr("Script Help"));
        setMinimumSize(QSize(480, 360));

        connect(ui->tableView, &ScriptTableView::doubleClicked, this, &CScriptHelpDialog::OnDoubleClick);
    }

    void CScriptHelpDialog::OnDoubleClick(const QModelIndex& index)
    {
        if (!index.isValid())
        {
            return;
        }
        const QString command = index.sibling(index.row(), ScriptHelpModel::ColumnCommand).data(Qt::DisplayRole).toString();
        const QString module = index.sibling(index.row(), ScriptHelpModel::ColumnModule).data(Qt::DisplayRole).toString();
        const QString textForClipboard = module + QLatin1Char('.') + command + "()";

        QApplication::clipboard()->setText(textForClipboard);
        setWindowTitle(QString("Script Help (Copied \"%1\" to clipboard)").arg(textForClipboard));
    }
} // namespace AzToolsFramework

#include <moc_ScriptHelpDialog.cpp>
