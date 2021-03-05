/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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

#include <Editor/Nodes/NodeUtils.h>
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
        AZ::Vector2 pos(20.0f, -100.0f);
        for (const auto& index : ui->commandList->selectionModel()->selectedIndexes())
        {
            if (index.column() != CommandListDataModel::ColumnIndex::Command)
            {
                continue;
            }

            AZ::Uuid type = dataModel->data(index, CommandListDataModel::CustomRole::Types).value<AZ::Uuid>();

            const AZ::SerializeContext::ClassData* classData = serializeContext->FindClassData(type);
            AZ_Assert(classData, "Failed to find ClassData for ID: %s", type.ToString<AZStd::string>().data());

            ScriptCanvasEditor::Nodes::StyleConfiguration styleConfiguration;

            NodeIdPair nodePair = ScriptCanvasEditor::Nodes::CreateNode(type, scriptCanvasId, styleConfiguration);
            GraphCanvas::SceneRequestBus::Event(graphCanvasGraphId, &GraphCanvas::SceneRequests::AddNode, nodePair.m_graphCanvasId, pos);

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
                            m_nodeTypes.push_back(classData->m_typeId);
                        }
                    }
                    return true;
                }
                );
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
            return static_cast<int>(m_nodeTypes.size());
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

                AZ::Uuid nodeType = m_nodeTypes[index.row()];
                const AZ::SerializeContext::ClassData* classData = serializeContext->FindClassData(nodeType);
                if (index.column() == ColumnIndex::Command)
                {
                    return QVariant(QString(classData->m_name));
                }
                if (index.column() == ColumnIndex::Description)
                {
                    return QVariant(QString(classData->m_editData ? classData->m_editData->m_description : tr("No description provided.")));
                }
                if (index.column() == ColumnIndex::Trail)
                {
                    return QVariant(QString(""));
                }
            }

            switch (role)
            {
            case CustomRole::Types:
            {
                AZ::Uuid nodeType = m_nodeTypes[index.row()];
                return QVariant::fromValue<AZ::Uuid>(nodeType);
            }
            break;
            case CustomRole::Node:
            {
                AZ::Uuid nodeType = m_nodeTypes[index.row()];
                const AZ::SerializeContext::ClassData* classData = serializeContext->FindClassData(nodeType);
                if (index.column() == ColumnIndex::Command)
                {
                    return QVariant(QString(classData->m_name));
                }
                if (index.column() == ColumnIndex::Description)
                {
                    return QVariant(QString(classData->m_editData ? classData->m_editData->m_description : tr("No description provided.")));
                }
                if (index.column() == ColumnIndex::Trail)
                {
                    return QVariant(QString(""));
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

            for (const auto& entry : m_nodeTypes)
            {
                const AZ::SerializeContext::ClassData* classData = serializeContext->FindClassData(entry);
                if (classData)
                {
                    QString name = QString(classData->m_name);
                    if (name.startsWith(input.c_str(), Qt::CaseSensitivity::CaseInsensitive))
                    {
                        return true;
                    }
                }
            }
            return false;
        }

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
                // TODO: trigger invoke
                // CommandRequestBus::Broadcast(&CommandRequest::Invoke, text().toStdString().c_str());
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
        CommandListDataProxyModel::CommandListDataProxyModel(QObject* parent /*= nullptr*/)
            : QSortFilterProxyModel(parent)
        {
            QStringList commands;

            CommandListDataModel* commandListData = new CommandListDataModel();
            for (int i = 0; i < commandListData->rowCount(); ++i)
            {
                QModelIndex index = commandListData->index(i, CommandListDataModel::ColumnIndex::Command);
                QString command = commandListData->data(index, CommandListDataModel::CustomRole::Node).toString();
                commands.push_back(command);
            }

            m_completer = new QCompleter(commands);
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

            QModelIndex index = dataModel->index(sourceRow, CommandListDataModel::ColumnIndex::Command);

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
            CommandListDataProxyModel* commandListDataProxyModel = new CommandListDataProxyModel();
            commandListDataProxyModel->setSourceModel(commandListDataModel);

            ui->commandList->setModel(commandListDataProxyModel);

            connect(ui->commandText, &QLineEdit::textChanged, this, &CommandLine::onTextChanged);

            connect(ui->commandText, &CommandLineEdit::onKeyReleased, this, &CommandLine::onEditKeyReleaseEvent);
            connect(ui->commandList, &CommandLineList::onKeyReleased, this, &CommandLine::onListKeyReleaseEvent);

            ui->commandList->setColumnWidth(CommandListDataModel::ColumnIndex::Command, 250);
            ui->commandList->setColumnWidth(CommandListDataModel::ColumnIndex::Description, 1000);
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
