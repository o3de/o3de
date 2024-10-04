/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QAction>
#include <QCompleter>
#include <QEvent>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QAction>
#include <QMenu>
#include <QMessageBox>
#include <QScopedValueRollback>
#include <QLineEdit>
#include <QTimer>
#include <QPushButton>
#include <QHeaderView>

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/UserSettings/UserSettings.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzToolsFramework/Entity/EditorEntityContextPickingBus.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

#include <GraphCanvas/Components/VisualBus.h>
#include <GraphCanvas/Components/SceneBus.h>
#include <GraphCanvas/Components/StyleBus.h>

#include <Editor/View/Widgets/VariablePanel/VariableDockWidget.h>
#include <Editor/View/Widgets/VariablePanel/ui_VariableDockWidget.h>

#include <Data/Data.h>

#include <Editor/GraphCanvas/GraphCanvasEditorNotificationBusId.h>
#include <Editor/QtMetaTypes.h>
#include <Editor/Settings.h>
#include <Editor/Translation/TranslationHelper.h>
#include <Editor/View/Widgets/DataTypePalette/DataTypePaletteModel.h>
#include <Editor/View/Widgets/NodePalette/VariableNodePaletteTreeItemTypes.h>
#include <Editor/View/Widgets/PropertyGridBus.h>
#include <Editor/Include/ScriptCanvas/Bus/EditorScriptCanvasBus.h>
#include <Editor/Include/ScriptCanvas/GraphCanvas/NodeDescriptorBus.h>

#include <ScriptCanvas/Core/NodeBus.h>
#include <ScriptCanvas/Data/DataRegistry.h>
#include <ScriptCanvas/GraphCanvas/NodeDescriptorBus.h>
#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include "GraphCanvas/Components/Slots/Data/DataSlotBus.h"

namespace ScriptCanvasEditor
{
    ////////////////////////////////
    // VariablePropertiesComponent
    ////////////////////////////////

    void VariablePropertiesComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {            
            serializeContext->Class<VariablePropertiesComponent, GraphCanvas::GraphCanvasPropertyComponent>()
                ->Version(1)
                ->Field("VariableName", &VariablePropertiesComponent::m_variableName)
                ->Field("VariableDatum", &VariablePropertiesComponent::m_variable)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<VariablePropertiesComponent>("Variable Properties", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "Properties")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->Attribute(AZ::Edit::Attributes::NameLabelOverride, &VariablePropertiesComponent::GetTitle)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &VariablePropertiesComponent::m_variableName, "Name", "")
                        ->Attribute(AZ::Edit::Attributes::StringLineEditingCompleteNotify, &VariablePropertiesComponent::OnNameChanged)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &VariablePropertiesComponent::m_variable, "Datum", "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }
        }
    }

    AZ::Entity* VariablePropertiesComponent::CreateVariablePropertiesEntity()
    {
        AZ::Entity* entity = aznew AZ::Entity("VariablePropertiesHelper");

        entity->CreateComponent<VariablePropertiesComponent>();

        return entity;
    }

    VariablePropertiesComponent::VariablePropertiesComponent()
        : m_variableName(AZStd::string())
        , m_variable(nullptr)
        , m_componentTitle("Variable")
    {
    }

    const char* VariablePropertiesComponent::GetTitle()
    {
        return m_componentTitle.c_str();
    }

    void VariablePropertiesComponent::SetVariable(ScriptCanvas::GraphVariable* variable)
    {
        if (variable)
        {
            ScriptCanvas::VariableNotificationBus::Handler::BusDisconnect();

            m_variable = variable;
            m_componentTitle.clear();
            m_variableName.clear();

            m_variableName = m_variable->GetVariableName();

            const AZStd::string variableTypeName = TranslationHelper::GetSafeTypeName(m_variable->GetDatum()->GetType());

            m_componentTitle = AZStd::string::format("%s Variable", variableTypeName.data());

            ScriptCanvas::VariableNotificationBus::Handler::BusConnect(m_variable->GetGraphScopedId());

            m_scriptCanvasGraphId = m_variable->GetGraphScopedId().m_scriptCanvasId;
        }
    }

    void VariablePropertiesComponent::OnNameChanged()
    {
        if (m_variable == nullptr)
        {
            return;
        }

        ScriptCanvas::VariableNotificationBus::Handler::BusDisconnect();

        AZ::Outcome<void, AZStd::string> outcome = AZ::Failure(AZStd::string());
        AZStd::string_view oldVariableName = m_variable->GetVariableName();

        if (oldVariableName != m_variableName)
        {            
            ScriptCanvas::VariableRequestBus::EventResult(outcome, m_variable->GetGraphScopedId(), &ScriptCanvas::VariableRequests::RenameVariable, m_variableName);

            AZ_Warning("VariablePropertiesComponent", outcome.IsSuccess(), "Could not rename variable: %s", outcome.GetError().c_str());
            if (!outcome.IsSuccess())
            {
                // Revert the variable name if we couldn't rename it (e.g. not unique)
                m_variableName = oldVariableName;
                PropertyGridRequestBus::Broadcast(&PropertyGridRequests::RefreshPropertyGrid);
            }
            else
            {
                GeneralRequestBus::Broadcast(&GeneralRequests::PostUndoPoint, m_scriptCanvasGraphId);
            }
        }

        ScriptCanvas::VariableNotificationBus::Handler::BusConnect(m_variable->GetGraphScopedId());
    }

    void VariablePropertiesComponent::OnVariableRemoved()
    {
        ScriptCanvas::VariableNotificationBus::Handler::BusDisconnect();        

        m_variableName = AZStd::string();
        m_variable = nullptr;
    }

    void VariablePropertiesComponent::OnVariableValueChanged()
    {
        GeneralRequestBus::Broadcast(&GeneralRequests::PostUndoPoint, m_scriptCanvasGraphId);
        PropertyGridRequestBus::Broadcast(&PropertyGridRequests::RefreshPropertyGrid);
        AzToolsFramework::ToolsApplicationEvents::Bus::Broadcast(&AzToolsFramework::ToolsApplicationEvents::InvalidatePropertyDisplay, AzToolsFramework::Refresh_AttributesAndValues);
    }

    void VariablePropertiesComponent::OnVariableRenamed(AZStd::string_view variableName)
    {
        m_variableName = variableName;
        PropertyGridRequestBus::Broadcast(&PropertyGridRequests::RefreshPropertyGrid);
    }

    void VariablePropertiesComponent::OnVariableScopeChanged()
    {
        GeneralRequestBus::Broadcast(&GeneralRequests::PostUndoPoint, m_scriptCanvasGraphId);
        PropertyGridRequestBus::Broadcast(&PropertyGridRequests::RefreshPropertyGrid);
    }

    /////////////////////////////
    // VariablePanelContextMenu
    /////////////////////////////

    VariablePanelContextMenu::VariablePanelContextMenu(VariableDockWidget* dockWidget, const ScriptCanvas::ScriptCanvasId& scriptCanvasId
        , ScriptCanvas::VariableId varId, QPoint position)
        : QMenu()
    {
        AZ::EntityId graphCanvasGraphId;
        GeneralRequestBus::BroadcastResult(graphCanvasGraphId, &GeneralRequests::GetGraphCanvasGraphId, scriptCanvasId);

        AZStd::string variableName;
        ScriptCanvas::GraphVariableManagerRequestBus::EventResult(variableName, scriptCanvasId, &ScriptCanvas::GraphVariableManagerRequests::GetVariableName, varId);

        QAction* getAction = new QAction(QObject::tr("Get %1").arg(variableName.c_str()), this);
        getAction->setToolTip(QObject::tr("Adds a Get %1 variable node onto the active graph.").arg(variableName.c_str()));
        getAction->setStatusTip(QObject::tr("Adds a Get %1 variable node onto the active graph.").arg(variableName.c_str()));
        
        QObject::connect(getAction,
                &QAction::triggered,
            [graphCanvasGraphId, varId](bool)
        {
            CreateGetVariableNodeMimeEvent mimeEvent(varId);

            AZ::EntityId viewId;
            GraphCanvas::SceneRequestBus::EventResult(viewId, graphCanvasGraphId, &GraphCanvas::SceneRequests::GetViewId);

            AZ::Vector2 viewCenter;
            GraphCanvas::ViewRequestBus::EventResult(viewCenter, viewId, &GraphCanvas::ViewRequests::GetViewSceneCenter);

            mimeEvent.ExecuteEvent(viewCenter, viewCenter, graphCanvasGraphId);
        });
        
        QAction* setAction = new QAction(QObject::tr("Set %1").arg(variableName.c_str()), this);
        setAction->setToolTip(QObject::tr("Adds a Set %1 variable node onto the active graph.").arg(variableName.c_str()));
        setAction->setStatusTip(QObject::tr("Adds a Set %1 variable node onto the active graph.").arg(variableName.c_str()));

        QObject::connect(setAction,
                &QAction::triggered,
            [graphCanvasGraphId, varId](bool)
        {
            CreateSetVariableNodeMimeEvent mimeEvent(varId);

            AZ::EntityId viewId;
            GraphCanvas::SceneRequestBus::EventResult(viewId, graphCanvasGraphId, &GraphCanvas::SceneRequests::GetViewId);

            AZ::Vector2 viewCenter;
            GraphCanvas::ViewRequestBus::EventResult(viewCenter, viewId, &GraphCanvas::ViewRequests::GetViewSceneCenter);

            mimeEvent.ExecuteEvent(viewCenter, viewCenter, graphCanvasGraphId);
        });

        QAction* copyAction = new QAction(QObject::tr("Copy %1").arg(variableName.c_str()), this);        
        copyAction->setToolTip(QObject::tr("Copies the variable called - %1").arg(variableName.c_str()));
        copyAction->setStatusTip(QObject::tr("Copies the variable called - %1").arg(variableName.c_str()));

        QObject::connect(copyAction,
            &QAction::triggered,
            [dockWidget, varId](bool)
        {
            GraphVariablesTableView::CopyVariableToClipboard(dockWidget->GetActiveScriptCanvasId(), varId);
        });

        QAction* pasteAction = new QAction(QObject::tr("Paste %1").arg(variableName.c_str()), this);
        pasteAction->setToolTip(QObject::tr("Pastes the variable %1 currently on the clipboard").arg(variableName.c_str()));
        pasteAction->setStatusTip(QObject::tr("Pastes the variable %1 currently on the clipboard").arg(variableName.c_str()));

        pasteAction->setEnabled(GraphVariablesTableView::HasCopyVariableData());

        QObject::connect(pasteAction,
            &QAction::triggered,
            [dockWidget](bool)
        {
            GraphVariablesTableView::HandleVariablePaste(dockWidget->GetActiveScriptCanvasId());
        });

        QAction* duplicateAction = new QAction(QObject::tr("Duplicate %1").arg(variableName.c_str()), this);
        duplicateAction->setToolTip(QObject::tr("Duplicates the variable called - %1").arg(variableName.c_str()));
        duplicateAction->setStatusTip(QObject::tr("Duplicates the variable called - %1").arg(variableName.c_str()));

        QObject::connect(duplicateAction,
            &QAction::triggered,
            [dockWidget, varId](bool)
        {
            dockWidget->OnDuplicateVariable(varId);
        });

        QAction* deleteAction = new QAction(QObject::tr("Delete %1").arg(variableName.c_str()), this);
        deleteAction->setToolTip(QObject::tr("Deletes the variable called - %1").arg(variableName.c_str()));
        deleteAction->setStatusTip(QObject::tr("Deletes the variable called - %1").arg(variableName.c_str()));

        QObject::connect(deleteAction,
            &QAction::triggered,
            [dockWidget, varId](bool)
        {
            AZStd::unordered_set< ScriptCanvas::VariableId > variableIds = { varId };
            dockWidget->OnDeleteVariables(variableIds);
        });

        QAction* configureAction = new QAction(QObject::tr("Configure %1").arg(variableName.c_str()), this);
        configureAction->setToolTip(QObject::tr("Sets the name/type the variable called - %1").arg(variableName.c_str()));
        configureAction->setStatusTip(QObject::tr("Sets the name/type the variable called - %1").arg(variableName.c_str()));

        QObject::connect(configureAction,
            &QAction::triggered,
            [dockWidget, varId, position](bool)
        {
            dockWidget->OnConfigureVariable(varId, position);
        });


        addAction(getAction);
        addAction(setAction);
        addSeparator();
        addAction(copyAction);
        addAction(pasteAction);
        addAction(duplicateAction);
        addAction(deleteAction);
        addAction(configureAction);
    }

    ///////////////////////
    // VariableDockWidget
    ///////////////////////

    AZStd::string VariableDockWidget::ConstructDefaultVariableName(AZ::u32 variableCounter)
    {
        return AZStd::string::format("Variable %u", variableCounter);
    }

    AZStd::string VariableDockWidget::FindDefaultVariableName(const ScriptCanvas::ScriptCanvasId& scriptCanvasExecutionId)
    {
        ScriptCanvas::VariableValidationOutcome nameAvailable = AZ::Failure(ScriptCanvas::GraphVariableValidationErrorCode::Unknown);
        AZStd::string varName;

        do
        {
            AZ::u32 varCounter = 0;
            SceneCounterRequestBus::EventResult(varCounter, scriptCanvasExecutionId, &SceneCounterRequests::GetNewVariableCounter);

            varName = ConstructDefaultVariableName(varCounter);

            ScriptCanvas::GraphVariableManagerRequestBus::EventResult(nameAvailable, scriptCanvasExecutionId, &ScriptCanvas::GraphVariableManagerRequests::IsNameValid, varName);
        } while (!nameAvailable);

        return varName;
    }

    VariableDockWidget::VariableDockWidget(QWidget* parent /*= nullptr*/)
        : AzQtComponents::StyledDockWidget(parent)
        , ui(new Ui::VariableDockWidget())
    {
        ui->setupUi(this);

        ui->graphVariables->setContentsMargins(0, 0, 0, 0);
        ui->graphVariables->setContextMenuPolicy(Qt::CustomContextMenu);

        QObject::connect(ui->graphVariables, &GraphVariablesTableView::SelectionChanged, this, &VariableDockWidget::OnSelectionChanged);
        QObject::connect(ui->graphVariables, &QWidget::customContextMenuRequested, this, &VariableDockWidget::OnContextMenuRequested);

        ui->searchFilter->setClearButtonEnabled(true);
        QObject::connect(ui->searchFilter, &QLineEdit::textChanged, this, &VariableDockWidget::OnQuickFilterChanged);
        QObject::connect(ui->searchFilter, &QLineEdit::returnPressed, this, &VariableDockWidget::OnReturnPressed);

        // Tell the widget to auto create our context menu, for now
        setContextMenuPolicy(Qt::ActionsContextMenu);

        // Add button is disabled by default, since we don't want to switch panels until we have an active scene.
        connect(ui->addButton, &QPushButton::clicked, this, &VariableDockWidget::OnAddVariableButton);
        
        ui->addButton->setEnabled(false);
        ui->searchFilter->setEnabled(false);

        QObject::connect(ui->variablePalette, &VariablePaletteTableView::CreateVariable, this, &VariableDockWidget::OnCreateVariable);
        QObject::connect(ui->variablePalette, &VariablePaletteTableView::CreateNamedVariable, this, &VariableDockWidget::OnCreateNamedVariable);
        QObject::connect(ui->graphVariables, &GraphVariablesTableView::DeleteVariables, this, &VariableDockWidget::OnDeleteVariables);

        m_filterTimer.setInterval(250);
        m_filterTimer.setSingleShot(true);
        m_filterTimer.stop();

        QObject::connect(&m_filterTimer, &QTimer::timeout, this, &VariableDockWidget::UpdateFilter);

        GraphCanvas::AssetEditorNotificationBus::Handler::BusConnect(ScriptCanvasEditor::AssetEditorId);

        ShowGraphVariables();

        VariableAutomationRequestBus::Handler::BusConnect();
    }

    VariableDockWidget::~VariableDockWidget()
    {
        GraphCanvas::AssetEditorNotificationBus::Handler::BusDisconnect();
        VariableAutomationRequestBus::Handler::BusDisconnect();
    }

    void VariableDockWidget::PopulateVariablePalette(const AZStd::unordered_set< AZ::Uuid >& objectTypes)
    {
        ui->variablePalette->PopulateVariablePalette(objectTypes);
    }

    void VariableDockWidget::OnActiveGraphChanged(const GraphCanvas::GraphId& graphCanvasGraphId)
    {
        ClearFilter();

        m_graphCanvasGraphId = graphCanvasGraphId;

        m_scriptCanvasId.SetInvalid();
        GeneralRequestBus::BroadcastResult(m_scriptCanvasId, &GeneralRequests::GetScriptCanvasId, graphCanvasGraphId);

        ui->graphVariables->SetActiveScene(m_scriptCanvasId);
        ui->variablePalette->SetActiveScene(m_scriptCanvasId);

        ui->addButton->setEnabled(m_scriptCanvasId.IsValid());
        ui->searchFilter->setEnabled(m_scriptCanvasId.IsValid());

        ShowGraphVariables();
    }

    AZStd::vector< ScriptCanvas::Data::Type > VariableDockWidget::GetPrimitiveTypes() const
    {
        AZStd::vector< ScriptCanvas::Data::Type > primitiveTypes;

        auto variableTypes = ui->variablePalette->GetVariableTypePaletteModel()->GetVariableTypes();

        for (auto variableType : variableTypes)
        {
            ScriptCanvas::Data::Type dataType = ScriptCanvas::Data::FromAZType(variableType);
            if (ScriptCanvas::Data::IsValueType(dataType))
            {
                primitiveTypes.push_back(dataType);
            }
        }

        return primitiveTypes;
    }

    AZStd::vector< ScriptCanvas::Data::Type > VariableDockWidget::GetBehaviorContextObjectTypes() const
    {
        AZStd::vector< ScriptCanvas::Data::Type > bcoTypes;

        auto variableTypes = ui->variablePalette->GetVariableTypePaletteModel()->GetVariableTypes();

        for (auto variableType : variableTypes)
        {
            ScriptCanvas::Data::Type dataType = ScriptCanvas::Data::FromAZType(variableType);
            if (!ScriptCanvas::Data::IsValueType(dataType))
            {
                if (!ScriptCanvas::Data::IsContainerType(dataType))
                {
                    bcoTypes.emplace_back(dataType);
                }
            }
        }
        
        return bcoTypes;
    }

    AZStd::vector< ScriptCanvas::Data::Type > VariableDockWidget::GetMapTypes() const
    {
        AZStd::vector< ScriptCanvas::Data::Type > variableDataTypes;

        auto mapTypes = ui->variablePalette->GetMapTypes();

        for (auto mapType : mapTypes)
        {
            ScriptCanvas::Data::Type dataType = ScriptCanvas::Data::FromAZType(mapType);
            variableDataTypes.emplace_back(dataType);
        }

        return variableDataTypes;
    }

    AZStd::vector< ScriptCanvas::Data::Type > VariableDockWidget::GetArrayTypes() const
    {
        AZStd::vector< ScriptCanvas::Data::Type > variableDataTypes;

        auto arrayTypes = ui->variablePalette->GetArrayTypes();

        for (auto arrayType : arrayTypes)
        {
            ScriptCanvas::Data::Type dataType = ScriptCanvas::Data::FromAZType(arrayType);
            variableDataTypes.emplace_back(dataType);
        }

        return variableDataTypes;
    }

    bool VariableDockWidget::IsShowingVariablePalette() const
    {
        return ui->stackedWidget->currentIndex() == ui->stackedWidget->indexOf(ui->variablePalettePage);
    }

    bool VariableDockWidget::IsShowingGraphVariables() const
    {
        return ui->stackedWidget->currentIndex() == ui->stackedWidget->indexOf(ui->graphVariablesPage);
    }

    QPushButton* VariableDockWidget::GetCreateVariableButton() const
    {
        return ui->addButton;
    }

    QTableView* VariableDockWidget::GetGraphPaletteTableView() const
    {
        return ui->graphVariables;
    }

    QTableView* VariableDockWidget::GetVariablePaletteTableView() const
    {
        return ui->variablePalette;
    }

    QLineEdit* VariableDockWidget::GetVariablePaletteFilter() const
    {
        return ui->searchFilter;
    }

    QLineEdit* VariableDockWidget::GetGraphVariablesFilter() const
    {
        return ui->searchFilter;
    }

    void VariableDockWidget::OnEscape()
    {
        ShowGraphVariables();
    }

    void VariableDockWidget::focusOutEvent(QFocusEvent* focusEvent)
    {
        AzQtComponents::StyledDockWidget::focusOutEvent(focusEvent);

        if (ui->stackedWidget->currentIndex() == ui->stackedWidget->indexOf(ui->variablePalettePage))
        {
            ShowGraphVariables();
        }
    }

    const ScriptCanvas::ScriptCanvasId& VariableDockWidget::GetActiveScriptCanvasId() const
    {
        return m_scriptCanvasId;
    }

    bool VariableDockWidget::IsValidVariableType(const ScriptCanvas::Data::Type& dataType) const
    {
        bool isValid = false;

        AZ::Uuid azType = ScriptCanvas::Data::ToAZType(dataType);

        if (ScriptCanvas::Data::IsMapContainerType(dataType))
        {
            auto mapTypes = ui->variablePalette->GetMapTypes();

            auto findIter = AZStd::find(mapTypes.begin(), mapTypes.end(), azType);

            isValid = findIter != mapTypes.end();
        }
        else if (ScriptCanvas::Data::IsVectorContainerType(dataType))
        {
            auto arrayTypes = ui->variablePalette->GetArrayTypes();

            auto findIter = AZStd::find(arrayTypes.begin(), arrayTypes.end(), azType);

            isValid = findIter != arrayTypes.end();
        }
        else
        {
            const auto& variableTypes = ui->variablePalette->GetVariableTypePaletteModel()->GetVariableTypes();

            auto findIter = AZStd::find(variableTypes.begin(), variableTypes.end(), azType);

            isValid = findIter != variableTypes.end();
        }

        return isValid;
    }

    void VariableDockWidget::ShowVariablePalette()
    {
        ui->stackedWidget->setCurrentIndex(ui->stackedWidget->indexOf(ui->variablePalettePage));
        ClearFilter();

        ui->searchFilter->setPlaceholderText("Variable Type...");
        FocusOnSearchFilter();

        ui->searchFilter->setCompleter(ui->variablePalette->GetVariableCompleter());

        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
    }

    void VariableDockWidget::ShowGraphVariables()
    {
        ui->stackedWidget->setCurrentIndex(ui->stackedWidget->indexOf(ui->graphVariablesPage));
        ClearFilter();

        ui->variablePalette->clearSelection();

        ui->searchFilter->setPlaceholderText("Search...");

        ui->searchFilter->setCompleter(nullptr);

        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
    }

    void VariableDockWidget::FocusOnSearchFilter()
    {
        ui->searchFilter->setFocus(Qt::FocusReason::MouseFocusReason);
    }

    void VariableDockWidget::ClearFilter()
    {
        {
            QSignalBlocker blocker(ui->searchFilter);
            ui->searchFilter->setText("");
        }

        UpdateFilter();
    }

    void VariableDockWidget::UpdateFilter()
    {
        if (IsShowingGraphVariables())
        {
            ui->graphVariables->SetFilter(ui->searchFilter->text());
        }
        else if (IsShowingVariablePalette())
        {
            ui->variablePalette->SetFilter(ui->searchFilter->userInputText());
        }
    }

    void VariableDockWidget::OnReturnPressed()
    {
        if (IsShowingVariablePalette())
        {
            ui->variablePalette->TryCreateVariableByTypeName(ui->searchFilter->text().toStdString().c_str());
        }
        else if (IsShowingGraphVariables())
        {
            UpdateFilter();
        }
    }

    void VariableDockWidget::OnQuickFilterChanged(const QString& text)
    {
        if(text.isEmpty())
        {
            //If field was cleared, update immediately
            UpdateFilter();
            return;
        }
        m_filterTimer.stop();
        m_filterTimer.start();
    }

    void VariableDockWidget::RefreshModel()
    {
        ui->graphVariables->SetActiveScene(m_scriptCanvasId);
    }

    void VariableDockWidget::OnAddVariableButton()
    {
        int index = ui->stackedWidget->currentIndex();

        // Switch between pages
        if (index == ui->stackedWidget->indexOf(ui->graphVariablesPage))
        {
            ShowVariablePalette();
        }
        else if(index == ui->stackedWidget->indexOf(ui->variablePalettePage))
        {
            ShowGraphVariables();
        }
    }

    void VariableDockWidget::OnContextMenuRequested(const QPoint& pos)
    {
        AzToolsFramework::EditorPickModeRequestBus::Broadcast(&AzToolsFramework::EditorPickModeRequests::StopEntityPickMode);

        QModelIndex index = ui->graphVariables->indexAt(pos);

        QActionGroup actionGroup(this);
        actionGroup.setExclusive(true);

        QAction* sortByName = actionGroup.addAction("Sort by name");
        sortByName->setCheckable(true);

        QAction* sortByType = actionGroup.addAction("Sort by type");
        sortByType->setCheckable(true);

        AZStd::intrusive_ptr<EditorSettings::ScriptCanvasEditorSettings> settings = AZ::UserSettings::CreateFind<EditorSettings::ScriptCanvasEditorSettings>(AZ_CRC_CE("ScriptCanvasPreviewSettings"), AZ::UserSettings::CT_LOCAL);

        if (settings->m_variablePanelSorting == GraphVariablesModel::ColumnIndex::Name)
        {
            sortByName->setChecked(true);
        }
        else
        {
            sortByType->setChecked(true);
        }

        QAction* cleanupAction = new QAction(QObject::tr("Remove unused variables"), this);
        QAction* actionResult = nullptr;


        ScriptCanvas::VariableId varId;

        // Bring up the context menu if the item is valid
        if (index.row() > -1)
        {
            varId = index.data(GraphVariablesModel::VarIdRole).value<ScriptCanvas::VariableId>();

            VariablePanelContextMenu menu(this, m_scriptCanvasId, varId, pos);

            menu.addSeparator();
            menu.addAction(cleanupAction);
            menu.addSeparator();
            menu.addAction(sortByName);
            menu.addAction(sortByType);
            

            actionResult = menu.exec(ui->graphVariables->mapToGlobal(pos));
        }
        else
        {
            QMenu menu;
            
            menu.addAction(cleanupAction);
            menu.addSeparator();
            menu.addAction(sortByName);
            menu.addAction(sortByType);

            actionResult = menu.exec(ui->graphVariables->mapToGlobal(pos));
        }

        // Very likely the actions are dangling pointers here. Do not dereference them.
        if (actionResult == sortByName)
        {
            settings->m_variablePanelSorting = GraphVariablesModel::ColumnIndex::Name;
            ui->graphVariables->ApplyPreferenceSort();
        }
        else if (actionResult == sortByType)
        {
            settings->m_variablePanelSorting = GraphVariablesModel::ColumnIndex::Type;
            ui->graphVariables->ApplyPreferenceSort();
        }
        else if (actionResult == cleanupAction)
        {
            OnRemoveUnusedVariables();
        }
    }

    void VariableDockWidget::OnSelectionChanged(const AZStd::unordered_set< ScriptCanvas::VariableId>& variableIds)
    {
        if (!variableIds.empty())
        {
            GraphCanvas::SceneRequestBus::Event(m_graphCanvasGraphId, &GraphCanvas::SceneRequests::ClearSelection);

            AZStd::vector<ScriptCanvas::VariableId> deselectedVariableIds;
            for (auto pair : m_usedElements)
            {
                if (variableIds.count(pair.first) == 0)
                {
                    deselectedVariableIds.emplace_back(pair.first);
                    m_unusedPool.emplace_back(pair.second);
                }
            }

            for (auto variableId : deselectedVariableIds)
            {
                ReleaseComponent(variableId);
            }
        }
        else
        {
            ResetPool();
        }

        AZStd::vector<AZ::EntityId> selection;

        auto owningGraph = ScriptCanvas::GraphRequestBus::FindFirstHandler(m_scriptCanvasId);

        if (owningGraph == nullptr)
        {
            return;
        }

        for (const ScriptCanvas::VariableId& varId : variableIds)
        {
            VariablePropertiesComponent* propertiesComponent = AllocateComponent(varId);

            if (propertiesComponent)
            {
                ScriptCanvas::GraphVariable* graphVariable = owningGraph->FindVariableById(varId);;
                propertiesComponent->SetVariable(graphVariable);

                selection.push_back(propertiesComponent->GetEntityId());
            }
        }

        OnHighlightVariables(variableIds);
        Q_EMIT OnVariableSelectionChanged(selection);
    }

    void VariableDockWidget::OnDuplicateVariable(const ScriptCanvas::VariableId& variableId)
    {
        ScriptCanvas::GraphVariable* graphVariable = nullptr;
        ScriptCanvas::GraphVariableManagerRequestBus::EventResult(graphVariable, m_scriptCanvasId, &ScriptCanvas::GraphVariableManagerRequests::FindVariableById, variableId);

        if (graphVariable == nullptr)
        {
            return;
        }

        ScriptCanvas::GraphVariableManagerRequestBus::Event(m_scriptCanvasId, &ScriptCanvas::GraphVariableManagerRequests::CloneVariable, (*graphVariable));
        GeneralRequestBus::Broadcast(&GeneralRequests::PostUndoPoint, m_scriptCanvasId);
    }

    void VariableDockWidget::OnCreateVariable(ScriptCanvas::Data::Type varType)
    {
        AZStd::string varName = FindDefaultVariableName(m_scriptCanvasId);
        OnCreateNamedVariable(varName, varType);
    }

    void VariableDockWidget::OnCreateNamedVariable(const AZStd::string& variableName, ScriptCanvas::Data::Type varType)
    {
        ShowGraphVariables();
        ScriptCanvas::Datum datum(varType, ScriptCanvas::Datum::eOriginality::Original);

        AZ::Outcome<ScriptCanvas::VariableId, AZStd::string> outcome = AZ::Failure(AZStd::string());
        ScriptCanvas::GraphVariableManagerRequestBus::EventResult(outcome, m_scriptCanvasId, &ScriptCanvas::GraphVariableManagerRequests::AddVariable, variableName, datum, false);

        AZ_Warning("VariablePanel", outcome.IsSuccess(), "Could not create new variable: %s", outcome.GetError().c_str());
        GeneralRequestBus::Broadcast(&GeneralRequests::PostUndoPoint, m_scriptCanvasId);

        // We already provide a naming hook for container types so we don't need to do re-force them into it.
        if (outcome.IsSuccess() && !ScriptCanvas::Data::IsContainerType(varType))
        {
            ui->graphVariables->EditVariableName(outcome.GetValue());
        }
    }

    void VariableDockWidget::OnDeleteVariables(const AZStd::unordered_set< ScriptCanvas::VariableId>& variableIds)
    {
        PropertyGridRequestBus::Broadcast(&PropertyGridRequests::ClearSelection);

        GeneralRequestBus::Broadcast(&GeneralRequests::PushPreventUndoStateUpdate);

        bool result = false;
        for (const ScriptCanvas::VariableId& variableId : variableIds)
        {
            if (CanDeleteVariable(variableId))
            {
                ScriptCanvas::GraphVariableManagerRequestBus::EventResult(result, m_scriptCanvasId, &ScriptCanvas::GraphVariableManagerRequests::RemoveVariable, variableId);
                AZ_Warning("VariablePanel", result, "Could not delete Variable Id (%s).", variableId.ToString().data());

                if (result)
                {
                    ReleaseComponent(variableId);
                }
            }
        }

        ui->graphVariables->ResizeColumns();
        GeneralRequestBus::Broadcast(&GeneralRequests::PopPreventUndoStateUpdate);

        if (result)
        {
            GeneralRequestBus::Broadcast(&GeneralRequests::PostUndoPoint, m_scriptCanvasId);
        }
    }

    void VariableDockWidget::OnHighlightVariables(const AZStd::unordered_set< ScriptCanvas::VariableId>& variableIds)
    {
        EditorGraphRequestBus::Event(m_scriptCanvasId, &EditorGraphRequests::HighlightVariables, variableIds);
    }

    void VariableDockWidget::OnConfigureVariable([[maybe_unused]] const ScriptCanvas::VariableId& variableId, [[maybe_unused]] QPoint position)
    {
         ScriptCanvas::GraphVariable* graphVariable = nullptr;
         ScriptCanvas::GraphVariableManagerRequestBus::EventResult(graphVariable, m_scriptCanvasId, &ScriptCanvas::GraphVariableManagerRequests::FindVariableById, variableId);
 
         if (graphVariable)
         {
             VariablePaletteRequests::VariableConfigurationInput input;
             input.m_graphVariable = graphVariable;
             input.m_changeVariableName = true;
             input.m_changeVariableType = true;

             VariablePaletteRequests::VariableConfigurationOutput output;
             VariablePaletteRequestBus::BroadcastResult(output, &VariablePaletteRequests::ShowVariableConfigurationWidget, input, position);

             if (output.m_actionIsValid)
             {
                 if ((output.m_nameChanged && !output.m_name.empty()) || (output.m_typeChanged && output.m_type.IsValid()))
                 {
                     GeneralRequestBus::Broadcast(&GeneralRequests::PostUndoPoint, m_scriptCanvasId);
                     GraphCanvas::ScopedGraphUndoBlocker undoBlocker(m_graphCanvasGraphId);

                     if ((output.m_nameChanged && !output.m_name.empty()))
                     {
                         graphVariable->SetVariableName(output.m_name);
                     }

                     if (output.m_typeChanged && output.m_type.IsValid())
                     {
                         graphVariable->ModDatum().SetType(output.m_type, ScriptCanvas::Datum::TypeChange::Forced);
                         ScriptCanvas::GraphRequestBus::Event(m_scriptCanvasId, &ScriptCanvas::GraphRequests::RefreshVariableReferences
                             , graphVariable->GetVariableId());
                     }
                 }
             }
         }
    }

    void VariableDockWidget::OnRemoveUnusedVariables()
    {
        EditorGraphRequestBus::Event(m_scriptCanvasId, &EditorGraphRequests::RemoveUnusedVariables);
    }

    bool VariableDockWidget::CanDeleteVariable(const ScriptCanvas::VariableId& variableId)
    {
        bool canDeleteVariable = false;

        AZStd::vector< NodeIdPair > nodeIds;
        EditorGraphRequestBus::EventResult(nodeIds, m_scriptCanvasId, &EditorGraphRequests::GetVariableNodes, variableId);

        if (!nodeIds.empty())
        {
            AZStd::string variableName;
            ScriptCanvas::VariableRequestBus::EventResult(variableName, ScriptCanvas::GraphScopedVariableId(m_scriptCanvasId, variableId), &ScriptCanvas::VariableRequests::GetName);

            int result = QMessageBox::warning(this, QString("Delete %1 and References").arg(variableName.c_str()), QString("The variable \"%1\" has %2 active references.\nAre you sure you want to delete the variable and its references from the graph?").arg(variableName.c_str()).arg(nodeIds.size()), QMessageBox::StandardButton::Yes, QMessageBox::StandardButton::Cancel);

            if (result == QMessageBox::StandardButton::Yes)
            {
                canDeleteVariable = true;

                AZStd::unordered_set< AZ::EntityId > memberIds;
                memberIds.reserve(nodeIds.size());

                AZStd::unordered_set< ScriptCanvas::VariableId > variableIds = { variableId };

                for (auto memberPair : nodeIds)
                {
                    bool removedReferences = false;

                    ScriptCanvas::NodeRequestBus::EventResult(removedReferences, memberPair.m_scriptCanvasId, &ScriptCanvas::NodeRequests::RemoveVariableReferences, variableIds);


                    // If we didn't remove the references. Just delete the node.
                    if (!removedReferences)
                    {
                        memberIds.insert(memberPair.m_graphCanvasId);
                    }
                }

                GraphCanvas::SceneRequestBus::Event(m_graphCanvasGraphId, &GraphCanvas::SceneRequests::Delete, memberIds);
            }
        }
        else
        {
            canDeleteVariable = true;
        }

        return canDeleteVariable;
    }

    VariablePropertiesComponent* VariableDockWidget::AllocateComponent(const ScriptCanvas::VariableId& variableId)
    {
        auto elementIter = m_usedElements.find(variableId);

        if (elementIter != m_usedElements.end())
        {
            return elementIter->second;
        }

        if (!m_unusedPool.empty())
        {
            VariablePropertiesComponent* component = m_unusedPool.back();
            m_unusedPool.pop_back();

            m_usedElements[variableId] = component;

            return component;
        }
        else
        {
            m_propertyHelpers.emplace_back(AZStd::unique_ptr<AZ::Entity>(VariablePropertiesComponent::CreateVariablePropertiesEntity()));

            AZ::Entity* entity = m_propertyHelpers.back().get();

            entity->Init();
            entity->Activate();

            VariablePropertiesComponent* component = AZ::EntityUtils::FindFirstDerivedComponent<VariablePropertiesComponent>(entity);

            if (component)
            {
                m_usedElements[variableId] = component;
                return component;
            }
        }

        return nullptr;
    }

    void VariableDockWidget::ReleaseComponent(const ScriptCanvas::VariableId& variableId)
    {
        auto mapIter = m_usedElements.find(variableId);

        if (mapIter != m_usedElements.end())
        {
            m_unusedPool.emplace_back(mapIter->second);
            m_usedElements.erase(mapIter);
        }
    }

    void VariableDockWidget::ResetPool()
    {
        for (auto pair : m_usedElements)
        {
            m_unusedPool.emplace_back(pair.second);
        }

        m_usedElements.clear();
    }

#include <Editor/View/Widgets/VariablePanel/moc_VariableDockWidget.cpp>
}

