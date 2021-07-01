/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "precompiled.h"
#include "CommandLine.h"

#include <QString>
#include <QKeyEvent>
#include <QCompleter>
#include <QGraphicsItem>
#include <QGraphicsView>

#include "Editor/View/Widgets/ui_CommandLine.h"

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

#include <Core/Attributes.h>
#include <Core/Node.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <Editor/Nodes/NodeCreateUtils.h>
#include <Editor/QtMetaTypes.h>

#include <ScriptCanvas/Bus/RequestBus.h>

#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/ViewBus.h>
#include <GraphCanvas/Components/GeometryBus.h>

namespace
{
    using namespace ScriptCanvasEditor;
    using namespace ScriptCanvasEditor::Widget;

    void CreateSelectedNodes(CommandLine* commandLine,
        AZStd::unique_ptr<Ui::CommandLine>& ui,
        AZ::SerializeContext* serializeContext)
    {
        if (!ui->commandList->selectionModel())
        {
            return;
        }

        if (ui->commandList->selectionModel()->selectedIndexes().empty())
        {
            // Nothing selected.
            return;
        }

        CommandListDataProxyModel* dataModel = qobject_cast<CommandListDataProxyModel*>(ui->commandList->model());
        if (!dataModel)
        {
            return;
        }

        ScriptCanvas::ScriptCanvasId scriptCanvasId;
        ScriptCanvasEditor::GeneralRequestBus::BroadcastResult(scriptCanvasId, &ScriptCanvasEditor::GeneralRequests::GetActiveScriptCanvasId);

        AZ::EntityId graphCanvasGraphId;
        ScriptCanvasEditor::GeneralRequestBus::BroadcastResult(graphCanvasGraphId, &ScriptCanvasEditor::GeneralRequests::GetActiveGraphCanvasGraphId);

        if (!(scriptCanvasId.IsValid() &&
            graphCanvasGraphId.IsValid()))
        {
            // Nothing active.
            return;
        }


        // Create the nodes in a horizontal list at the top of the canvas.
        AZ::Vector2 pos(20.0f, 20.0f);
        for (const auto& index : ui->commandList->selectionModel()->selectedIndexes())
        {
            if (index.column() != CommandListDataModel::ColumnIndex::CommandIndex)
            {
                continue;
            }

            AZ::Uuid type = dataModel->data(index, CommandListDataModel::CustomRole::Types).value<AZ::Uuid>();
            if (type.IsNull())
            {
                continue;
            }

            [[maybe_unused]] const AZ::SerializeContext::ClassData* classData = serializeContext->FindClassData(type);
            AZ_Assert(classData, "Failed to find ClassData for ID: %s", type.ToString<AZStd::string>().data());

            ScriptCanvasEditor::Nodes::StyleConfiguration styleConfiguration;

            NodeIdPair nodePair = ScriptCanvasEditor::Nodes::CreateNode(type, scriptCanvasId, styleConfiguration);
            GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::AddNode, nodePair.m_graphCanvasId, pos, false);

            // The next position to create a node at.
            // IMPORTANT: This SHOULD be using GraphCanvas::GeometryRequests::GetWidth, but
            // GetWidth is currently returning zero, so we'll TEMPORARILY use a fixed value.
            pos += AZ::Vector2(125.0f, 0.0f);
        }

        commandLine->hide();
    }
} // anonymous namespace.

namespace ScriptCanvasEditor
{
    namespace Widget
    {
        // CommandListDataModel
        /////////////////////////////////////////////////////////////////////////////////////////////
        CommandListDataModel::CommandListDataModel([[maybe_unused]] QWidget* parent /*= nullptr*/)
        {
            ScriptCanvasCommandLineRequestBus::Handler::BusConnect();

            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

            serializeContext->EnumerateDerived<ScriptCanvas::Node>(
                [this](const AZ::SerializeContext::ClassData* classData, [[maybe_unused]] const AZ::Uuid& classUuid) -> bool
                {
                    if (classData && classData->m_editData)
                    {
                        bool add = true;
                        if (auto editorElementData = classData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData))
                        {
                            if (auto excludeAttribute = editorElementData->FindAttribute(AZ::Script::Attributes::ExcludeFrom))
                            {
                                auto excludeAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<bool>*>(excludeAttribute);
                                if (excludeAttributeData)
                                {
                                    add = false;
                                }
                            }
                        }

                        if (add)
                        {
                            Entry entry;
                            entry.m_type = classData->m_typeId;
                            m_entries.emplace_back(entry);
                        }
                    }
                    return true;
                }
            );

            ScriptCanvasCommandLineRequestBus::Broadcast(&ScriptCanvasCommandLineRequests::AddCommand, "add_node", "Adds the specified node to the graph",
                [serializeContext](const AZStd::vector<AZStd::string>& nodes)
                {
                    AZ::Uuid nodeTypeToAdd = AZ::Uuid::CreateNull();
                    if (nodes.size() > 0)
                    {
                        const AZStd::string& nodeName = *(nodes.begin());

                        serializeContext->EnumerateDerived<ScriptCanvas::Node>(
                            [&nodeName, &nodeTypeToAdd](const AZ::SerializeContext::ClassData* classData, [[maybe_unused]] const AZ::Uuid& classUuid) -> bool
                            {
                                if (classData && classData->m_editData)
                                {
                                    if (nodeName.compare(classData->m_name) == 0)
                                    {
                                        nodeTypeToAdd = classData->m_typeId;
                                    }
                                }
                                return true;
                            }
                        );

                        if (!nodeTypeToAdd.IsNull())
                        {
                            ScriptCanvas::ScriptCanvasId scriptCanvasId;
                            ScriptCanvasEditor::GeneralRequestBus::BroadcastResult(scriptCanvasId, &ScriptCanvasEditor::GeneralRequests::GetActiveScriptCanvasId);

                            AZ::EntityId graphCanvasGraphId;
                            ScriptCanvasEditor::GeneralRequestBus::BroadcastResult(graphCanvasGraphId, &ScriptCanvasEditor::GeneralRequests::GetActiveGraphCanvasGraphId);

                            if (scriptCanvasId.IsValid() && graphCanvasGraphId.IsValid())
                            {
                                ScriptCanvasEditor::Nodes::StyleConfiguration styleConfiguration;

                                AZ::Vector2 pos(100.0f, 20.0f);
                                NodeIdPair nodePair = ScriptCanvasEditor::Nodes::CreateNode(nodeTypeToAdd, scriptCanvasId, styleConfiguration);
                                GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::AddNode, nodePair.m_graphCanvasId, pos, false);
                            }
                        }
                    }
                }
            );
        }

        CommandListDataModel::~CommandListDataModel()
        {
            ScriptCanvasCommandLineRequestBus::Handler::BusDisconnect();
        }

        QModelIndex CommandListDataModel::index(int row, int column, const QModelIndex& parent /*= QModelIndex()*/) const
        {
            if (row >= rowCount(parent) || column >= columnCount(parent))
            {
                return QModelIndex();
            }
            return createIndex(row, column);
        }

        QModelIndex CommandListDataModel::parent([[maybe_unused]] const QModelIndex& child) const
        {
            return QModelIndex();
        }

        int CommandListDataModel::rowCount([[maybe_unused]] const QModelIndex& parent /*= QModelIndex()*/) const
        {
            return static_cast<int>(m_entries.size());
        }

        int CommandListDataModel::columnCount([[maybe_unused]] const QModelIndex& parent /*= QModelIndex()*/) const
        {
            return ColumnIndex::Count;
        }

        QVariant CommandListDataModel::data(const QModelIndex& index, int role /*= Qt::DisplayRole*/) const
        {
            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

            if (role == Qt::DisplayRole)
            {
                if (index.row() == 0)
                {
                    if (index.column() == 0)
                    {
                        return QVariant(QString(tr("No results found.")));
                    }
                    else
                    if (index.column() > 0)
                    {
                        return QVariant();
                    }
                }

                AZ::Uuid nodeType = m_entries[index.row()].m_type;
                if (nodeType.IsNull())
                {
                    if (index.column() == ColumnIndex::CommandIndex)
                    {
                        return QVariant(QString(m_entries[index.row()].m_command.c_str()));
                    }
                    if (index.column() == ColumnIndex::DescriptionIndex)
                    {
                        AZStd::string command = m_entries[index.row()].m_command;
                        const auto& entry = m_commands.find(command);
                        if (entry != m_commands.end())
                        {
                            return QVariant(QString(entry->second->GetDescription().c_str()));
                        }
                    }
                }
                else
                {
                    if (const AZ::SerializeContext::ClassData* classData = serializeContext->FindClassData(nodeType))
                    {
                        if (index.column() == ColumnIndex::CommandIndex)
                        {
                            return QVariant(QString(classData->m_name));
                        }
                        if (index.column() == ColumnIndex::DescriptionIndex)
                        {
                            return QVariant(QString(classData->m_editData ? classData->m_editData->m_description : tr("No description provided.")));
                        }
                        if (index.column() == ColumnIndex::TrailIndex)
                        {
                            return QVariant(QString(""));
                        }
                    }
                }
            }

            switch (role)
            {
            case CustomRole::Types:
            {
                AZ::Uuid nodeType = m_entries[index.row()].m_type;
                return QVariant::fromValue<AZ::Uuid>(nodeType);
            }
            break;
            case CustomRole::Node:
            {
                AZ::Uuid nodeType = m_entries[index.row()].m_type;
                if (nodeType.IsNull())
                {
                    return QVariant(QString(m_entries[index.row()].m_command.c_str()));
                }
                else
                {
                    if (const AZ::SerializeContext::ClassData* classData = serializeContext->FindClassData(nodeType))
                    {
                        if (index.column() == ColumnIndex::CommandIndex)
                        {
                            return QVariant(QString(classData->m_name));
                        }
                        if (index.column() == ColumnIndex::DescriptionIndex)
                        {
                            return QVariant(QString(classData->m_editData ? classData->m_editData->m_description : tr("No description provided.")));
                        }
                        if (index.column() == ColumnIndex::TrailIndex)
                        {
                            return QVariant(QString(""));
                        }
                    }
                }
            }
            break;
            case CustomRole::Commands:
            {
                if (index.column() == ColumnIndex::CommandIndex)
                {
                    return QVariant(QString(m_entries[index.row()].m_command.c_str()));
                }
            }
            break;

            default:
                break;
            }

            return QVariant();
        }

        Qt::ItemFlags CommandListDataModel::flags(const QModelIndex& index) const
        {
            return QAbstractTableModel::flags(index);
        }

        bool CommandListDataModel::HasMatches(const AZStd::string& input)
        {
            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

            for (const auto& entry : m_entries)
            {
                if (!entry.m_type.IsNull())
                {
                    if (const AZ::SerializeContext::ClassData* classData = serializeContext->FindClassData(entry.m_type))
                    {
                        QString name = QString(classData->m_name);
                        if (name.startsWith(input.c_str(), Qt::CaseSensitivity::CaseInsensitive))
                        {
                            return true;
                        }
                    }
                }
                else
                {
                    QString commandName = entry.m_command.c_str();
                    return (commandName.startsWith(input.c_str(), Qt::CaseSensitivity::CaseInsensitive));
                }
            }

            return false;
        }

        ScriptCanvasEditor::Widget::CommandRegistry CommandListDataModel::m_commands;

        // CommandLineEdit
        /////////////////////////////////////////////////////////////////////////////////////////////

        namespace
        {
            QString s_defaultText = QStringLiteral("Press ? for help");
        }

        CommandLineEdit::CommandLineEdit(QWidget* parent /*= nullptr*/)
            : QLineEdit(parent)
            , m_empty(true)
            , m_defaultText(s_defaultText)
        {
            ResetState();

            connect(this, &QLineEdit::textChanged, this, &CommandLineEdit::onTextChanged);
            connect(this, &QLineEdit::textEdited, this, &CommandLineEdit::onTextEdited);
            connect(this, &QLineEdit::returnPressed, this, &CommandLineEdit::onReturnPressed);
        }

        void CommandLineEdit::onReturnPressed()
        {
        }

        void CommandLineEdit::onTextChanged([[maybe_unused]] const QString& text)
        {
        }

        void CommandLineEdit::onTextEdited(const QString& text)
        {
            // Execute the command
            (void)text;
            if (text.isEmpty())
            {
                ResetState();
            }
        }


        void CommandLineEdit::focusInEvent(QFocusEvent* e)
        {
            QLineEdit::focusInEvent(e);
            Q_EMIT(onFocusChange(true));
        }

        void CommandLineEdit::focusOutEvent(QFocusEvent* e)
        {
            QLineEdit::focusInEvent(e);
            Q_EMIT(onFocusChange(true));
        }

        void CommandLineEdit::ResetState()
        {
            m_empty = true;
            setText(m_defaultText);
        }

        void CommandLineEdit::keyReleaseEvent(QKeyEvent* event)
        {
            Q_EMIT(onKeyReleased(event));
        }

        void CommandLineEdit::keyPressEvent(QKeyEvent* event)
        {
            switch (event->key())
            {
            case Qt::Key_Enter:
            case Qt::Key_Return:
            {
                // Invoke the command
                AZStd::string commandText = text().toStdString().c_str();
                AZStd::vector<AZStd::string> tokens;
                AZ::StringFunc::Tokenize(commandText, tokens, " ");
                if (tokens.size() == 1)
                {
                    ScriptCanvasCommandLineRequestBus::Broadcast(&ScriptCanvasCommandLineRequests::Invoke, tokens.begin()->c_str());
                }
                else if (tokens.size() > 1)
                {
                    AZStd::string command = *(tokens.begin());
                    AZStd::vector<AZStd::string> args;
                    for (auto it = tokens.begin() + 1; it != tokens.end(); ++it)
                    {
                        args.push_back(*it);
                    }
                    ScriptCanvasCommandLineRequestBus::Broadcast(&ScriptCanvasCommandLineRequests::InvokeWithArguments, command.c_str(), args);
                }


                ResetState();
                qobject_cast<QWidget*>(parent())->hide();
            }
            break;

            case Qt::Key_Backspace:
                if (m_empty)
                {
                    break;
                }
                QLineEdit::keyPressEvent(event);
                break;

            case Qt::Key_Escape:
                ResetState();
                qobject_cast<QWidget*>(parent())->hide();
                return;

            default:
                if (m_empty)
                {
                    setText("");
                    m_empty = false;
                }
                QLineEdit::keyPressEvent(event);
                break;
            }
        }

        // CommandLineList
        /////////////////////////////////////////////////////////////////////////////////////////////
        CommandLineList::CommandLineList(QWidget* parent /*= nullptr*/)
            : QTableView(parent)
        {
        }


        // CommandListDataProxyModel
        /////////////////////////////////////////////////////////////////////////////////////////////
        CommandListDataProxyModel::CommandListDataProxyModel(CommandListDataModel* commandListData, QObject* parent /*= nullptr*/)
            : QSortFilterProxyModel(parent)
        {
            setSourceModel(commandListData);

            QStringList commandList;

            for (int i = 0; i < commandListData->rowCount(); ++i)
            {
                QModelIndex index = commandListData->index(i, CommandListDataModel::ColumnIndex::CommandIndex);
                QString command = commandListData->data(index, CommandListDataModel::CustomRole::Node).toString();
                commandList.push_back(command);
            }

            ScriptCanvasCommandLineRequests::CommandNameList commands;
            ScriptCanvasCommandLineRequestBus::BroadcastResult(commands, &ScriptCanvasCommandLineRequests::GetCommands);
            for (auto& command : commands)
            {
                QString commandName = command.first.c_str();
                commandList.push_back(commandName);
            }

            m_completer = new QCompleter(commandList);
            m_completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
            m_completer->setCaseSensitivity(Qt::CaseInsensitive);
        }


        bool CommandListDataProxyModel::filterAcceptsRow(int sourceRow, [[maybe_unused]] const QModelIndex& sourceParent) const
        {
            CommandListDataModel* dataModel = qobject_cast<CommandListDataModel*>(sourceModel());

            if (sourceRow < 0 || sourceRow >= dataModel->rowCount())
            {
                return false;
            }

            if (m_input.empty() || m_input.compare(s_defaultText.toStdString().c_str()) == 0)
            {
                return false;
            }

            if (AzFramework::StringFunc::FirstCharacter(m_input.c_str()) == '?')
            {
                if (sourceRow > 0)
                {
                    return true;
                }
                else
                {
                    return false;
                }
            }

            QModelIndex index = dataModel->index(sourceRow, CommandListDataModel::ColumnIndex::CommandIndex);

            QString sourceStr = dataModel->data(index).toString();
            if (sourceRow > 0 && sourceStr.startsWith(m_input.c_str(), Qt::CaseSensitivity::CaseInsensitive))
            {
                return true;
            }
            else
            if (sourceRow == 0)
            {
                if (!dataModel->HasMatches(m_input))
                {
                    return true;
                }
            }

            return false;
        }

        // CommandLine
        /////////////////////////////////////////////////////////////////////////////////////////////

        CommandLine::CommandLine(QWidget* parent /*=     nullptr*/)
            : QWidget(parent)
            , ui(new Ui::CommandLine())
        {
            ui->setupUi(this);

            CommandListDataModel* commandListDataModel = new CommandListDataModel();
            CommandListDataProxyModel* commandListDataProxyModel = new CommandListDataProxyModel(commandListDataModel);

            ui->commandList->setModel(commandListDataProxyModel);

            connect(ui->commandText, &QLineEdit::textChanged, this, &CommandLine::onTextChanged);

            connect(ui->commandText, &CommandLineEdit::onKeyReleased, this, &CommandLine::onEditKeyReleaseEvent);
            connect(ui->commandList, &CommandLineList::onKeyReleased, this, &CommandLine::onListKeyReleaseEvent);

            ui->commandList->setColumnWidth(CommandListDataModel::ColumnIndex::CommandIndex, 250);
            ui->commandList->setColumnWidth(CommandListDataModel::ColumnIndex::DescriptionIndex, 1000);
        }

        void CommandLine::onTextChanged(const QString& text)
        {
            CommandListDataProxyModel* model = qobject_cast<CommandListDataProxyModel*>(ui->commandList->model());
            if (model)
            {
                model->SetInput(text.toStdString().c_str());
            }
        }


        void CommandLine::onListKeyReleaseEvent(QKeyEvent* event)
        {
            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
            AZ_Assert(serializeContext, "Failed to acquire application serialize context.");

            switch (event->key())
            {
            case Qt::Key_Up:
                if (ui->commandList->selectionModel())
                {
                    if (ui->commandList->selectionModel()->selectedIndexes().empty() || ui->commandList->selectionModel()->selectedIndexes().at(0).row() == 0)
                    {
                        ui->commandText->setFocus();
                    }
                }
                break;

            case Qt::Key_Escape:
                hide();
                break;

            case Qt::Key_Enter:
            case Qt::Key_Return:
                CreateSelectedNodes(this, ui, serializeContext);
                break;
            }
        }

        void CommandLine::onEditKeyReleaseEvent(QKeyEvent* event)
        {
            if (event->key() == Qt::Key_Down)
            {
                ui->commandList->setFocus();
                ui->commandList->selectRow(0);
            }
        }

        void CommandLine::showEvent(QShowEvent* event)
        {
            QWidget::showEvent(event);

            ui->commandText->ResetState();
            ui->commandText->setFocus(Qt::PopupFocusReason);
        }

        #include <Editor/View/Widgets/moc_CommandLine.cpp>
    }
}
