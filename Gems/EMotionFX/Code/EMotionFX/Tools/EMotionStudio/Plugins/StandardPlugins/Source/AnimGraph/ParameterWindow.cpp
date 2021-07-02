/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include required headers
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzQtComponents/Components/FilteredSearchWidget.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphGroupParameterCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphParameterCommands.h>
#include <EMotionFX/CommandSystem/Source/SelectionList.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/AnimGraphGameControllerSettings.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/BlendTreeParameterNode.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendTreeVisualNode.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/GameControllerWindow.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/GraphNode.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/NodeGraph.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/ParameterCreateEditDialog.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/ParameterEditor/ParameterEditorFactory.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/ParameterEditor/ValueParameterEditor.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/ParameterWindow.h>
#include <EMotionFX/Source/Parameter/GroupParameter.h>
#include <EMotionFX/Source/Parameter/ValueParameter.h>
#include <MCore/Source/LogManager.h>
#include <MCore/Source/ReflectionSerializer.h>
#include <QAction>
#include <QContextMenuEvent>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QTreeWidget>
#include <QToolBar>
#include <QVBoxLayout>
#include <QtWidgets/private/qabstractitemview_p.h>

namespace EMStudio
{
    int ParameterWindow::m_contextMenuWidth = 100;

    // constructor
    ParameterCreateRenameWindow::ParameterCreateRenameWindow(const char* windowTitle, const char* topText, const char* defaultName, const char* oldName, const AZStd::vector<AZStd::string>& invalidNames, QWidget* parent)
        : QDialog(parent)
    {
        setObjectName("EMFX.ParameterCreateRenameDialog");

        // store values
        mOldName = oldName;
        mInvalidNames = invalidNames;

        // update title of the about dialog
        setWindowTitle(windowTitle);

        // set the minimum width
        setMinimumWidth(300);

        // create the about dialog's layout
        QVBoxLayout* layout = new QVBoxLayout();

        // add the top text if valid
        if (topText)
        {
            layout->addWidget(new QLabel(topText));
        }

        // add the line edit
        mLineEdit = new QLineEdit(defaultName);
        connect(mLineEdit, &QLineEdit::textChanged, this, &ParameterCreateRenameWindow::NameEditChanged);
        mLineEdit->selectAll();

        // create the button layout
        QHBoxLayout* buttonLayout = new QHBoxLayout();
        mOKButton       = new QPushButton("OK");
        mCancelButton   = new QPushButton("Cancel");
        buttonLayout->addWidget(mOKButton);
        buttonLayout->addWidget(mCancelButton);

        // set the layout
        layout->addWidget(mLineEdit);
        layout->addLayout(buttonLayout);
        setLayout(layout);

        // connect the buttons
        connect(mOKButton, &QPushButton::clicked, this, &ParameterCreateRenameWindow::accept);
        connect(mCancelButton, &QPushButton::clicked, this, &ParameterCreateRenameWindow::reject);

        mOKButton->setDefault(true);
    }

    // check for duplicate names upon editing
    void ParameterCreateRenameWindow::NameEditChanged(const QString& text)
    {
        const AZStd::string convertedNewName = text.toUtf8().data();
        if (text.isEmpty())
        {
            mOKButton->setEnabled(false);
            GetManager()->SetWidgetAsInvalidInput(mLineEdit);
        }
        else if (mOldName == convertedNewName)
        {
            mOKButton->setEnabled(true);
            mLineEdit->setStyleSheet("");
        }
        else
        {
            // Check if the name has invalid characters.
            if (!EMotionFX::Parameter::IsNameValid(convertedNewName, nullptr))
            {
                mOKButton->setEnabled(false);
                GetManager()->SetWidgetAsInvalidInput(mLineEdit);
                return;
            }

            // Is there a parameter with the given name already?
            if (AZStd::find(mInvalidNames.begin(), mInvalidNames.end(), convertedNewName) != mInvalidNames.end())
            {
                mOKButton->setEnabled(false);
                GetManager()->SetWidgetAsInvalidInput(mLineEdit);
                return;
            }

            // no duplicate name found
            mOKButton->setEnabled(true);
            mLineEdit->setStyleSheet("");
        }
    }

    ParameterWindow::ParameterWindow(AnimGraphPlugin* plugin)
        : QWidget()
    {
        mPlugin = plugin;
        mEnsureVisibility = false;
        mLockSelection = false;

        // add the add button
        QToolBar* toolBar = new QToolBar(this);

        m_addAction = toolBar->addAction(
            QIcon(":/EMotionFX/Plus.svg"),
            tr("Add new parameter or group"));
        {
            QToolButton* toolButton = qobject_cast<QToolButton*>(toolBar->widgetForAction(m_addAction));
            AZ_Assert(toolButton, "The action widget must be a tool button.");
            toolButton->setPopupMode(QToolButton::InstantPopup);

            QMenu* contextMenu = new QMenu(toolBar);

            QAction* addParameterAction = contextMenu->addAction("Add parameter");
            connect(addParameterAction, &QAction::triggered, this, &ParameterWindow::OnAddParameter);

            QAction* addGroupAction = contextMenu->addAction("Add group");
            connect(addGroupAction, &QAction::triggered, this, &ParameterWindow::OnAddGroup);

            m_addAction->setMenu(contextMenu);
        }

        // add edit button
        m_editAction = toolBar->addAction(QIcon(":/EMotionFX/Edit.svg"),
            tr("Edit selected parameter"),
            this, &ParameterWindow::OnEditButton);

        // add spacer widget
        QWidget* spacerWidget = new QWidget();
        spacerWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
        toolBar->addWidget(spacerWidget);

        // add the search filter button
        m_searchWidget = new AzQtComponents::FilteredSearchWidget(this);
        connect(m_searchWidget, &AzQtComponents::FilteredSearchWidget::TextFilterChanged, this, &ParameterWindow::OnTextFilterChanged);
        m_searchWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
        toolBar->addWidget(m_searchWidget);

        // create the parameter tree widget
        mTreeWidget = new ParameterWindowTreeWidget();
        mTreeWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        mTreeWidget->setObjectName("AnimGraphParamWindow");
        mTreeWidget->header()->setVisible(false);

        // adjust selection mode and enable some other helpful things
        mTreeWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
        mTreeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
        mTreeWidget->setExpandsOnDoubleClick(true);
        mTreeWidget->setColumnCount(3);
        mTreeWidget->setUniformRowHeights(true);
        mTreeWidget->setIndentation(10);
        mTreeWidget->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        mTreeWidget->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        mTreeWidget->header()->setSectionResizeMode(2, QHeaderView::Stretch);

        // enable drag and drop
        mTreeWidget->setDragEnabled(true);
        mTreeWidget->setDragDropMode(QAbstractItemView::InternalMove);

        // connect the tree widget
        connect(mTreeWidget, &QTreeWidget::itemSelectionChanged, this, &ParameterWindow::OnSelectionChanged);
        connect(mTreeWidget, &QTreeWidget::itemCollapsed, this, &ParameterWindow::OnGroupCollapsed);
        connect(mTreeWidget, &QTreeWidget::itemExpanded, this, &ParameterWindow::OnGroupExpanded);
        connect(mTreeWidget, &ParameterWindowTreeWidget::ParameterMoved, this, &ParameterWindow::OnMoveParameterTo);
        connect(mTreeWidget, &ParameterWindowTreeWidget::DragEnded, this, [this]()
        {
            Reinit(/*forceReinit*/true);
        });

        // create and fill the vertical layout
        mVerticalLayout = new QVBoxLayout();
        mVerticalLayout->setObjectName("StyledWidget");
        mVerticalLayout->setSpacing(2);
        mVerticalLayout->setMargin(0);
        mVerticalLayout->setAlignment(Qt::AlignTop);
        mVerticalLayout->addWidget(toolBar);
        mVerticalLayout->addWidget(mTreeWidget);

        // set the object name
        setObjectName("StyledWidget");

        setLayout(mVerticalLayout);

        // set the focus policy
        setFocusPolicy(Qt::ClickFocus);

        // Force reinitialize in case e.g. a parameter got added or removed.
        connect(&mPlugin->GetAnimGraphModel(), &AnimGraphModel::ParametersChanged, [this](EMotionFX::AnimGraph* animGraph)
        {
            if (animGraph == m_animGraph)
            {
                Reinit(/*forceReinit*/true);
            }
        });

        connect(&mPlugin->GetAnimGraphModel(), &AnimGraphModel::FocusChanged, this, &ParameterWindow::OnFocusChanged);

        // Trigger actions are processed from EMotionFX worker threads, which
        // are not allowed to update the UI. Use a QueuedConnection to force
        // the update to happen on the main thread.
        connect(this, &ParameterWindow::OnParameterActionTriggered, this, &ParameterWindow::UpdateParameterValue, Qt::QueuedConnection);

        Reinit();
        EMotionFX::AnimGraphNotificationBus::Handler::BusConnect();
    }

    ParameterWindow::~ParameterWindow()
    {
        EMotionFX::AnimGraphNotificationBus::Handler::BusDisconnect();
    }

    // check if the gamepad control mode is enabled for the given parameter and if its actually being controlled or not
    void ParameterWindow::GetGamepadState(EMotionFX::AnimGraph* animGraph, const EMotionFX::Parameter* parameter, bool* outIsActuallyControlled, bool* outIsEnabled)
    {
        *outIsActuallyControlled = false;
        *outIsEnabled = false;

        // get the game controller settings and its active preset
        EMotionFX::AnimGraphGameControllerSettings& gameControllerSettings = animGraph->GetGameControllerSettings();
        EMotionFX::AnimGraphGameControllerSettings::Preset* preset = gameControllerSettings.GetActivePreset();

        // get access to the game controller and check if it is valid
        bool isGameControllerValid = false;
#if AZ_TRAIT_EMOTIONFX_HAS_GAME_CONTROLLER
        GameControllerWindow* gameControllerWindow = mPlugin->GetGameControllerWindow();
        if (gameControllerWindow)
        {
            isGameControllerValid = gameControllerWindow->GetIsGameControllerValid();
        }
#endif

        // only update in case a preset is selected and the game controller is valid
        if (preset && isGameControllerValid)
        {
            // check if the given parameter is controlled by a joystick
            EMotionFX::AnimGraphGameControllerSettings::ParameterInfo* controllerParameterInfo = preset->FindParameterInfo(parameter->GetName().c_str());
            if (controllerParameterInfo)
            {
                // set the gamepad controlled enable flag
                if (controllerParameterInfo->m_enabled)
                {
                    *outIsEnabled = true;
                }

                // when the axis is not set to "None"
                if (controllerParameterInfo->m_axis != MCORE_INVALIDINDEX8)
                {
                    *outIsActuallyControlled = true;
                }
            }

            // check if the given parameter is controlled by a gamepad button
            if (preset->CheckIfIsParameterButtonControlled(parameter->GetName().c_str()))
            {
                *outIsActuallyControlled = true;
            }
            if (preset->CheckIfIsButtonEnabled(parameter->GetName().c_str()))
            {
                *outIsEnabled = true;
            }
        }
    }


    // helper function to update all parameter and button infos
    void ParameterWindow::SetGamepadState(EMotionFX::AnimGraph* animGraph, const EMotionFX::Parameter* parameter, bool isEnabled)
    {
        // get the game controller settings and its active preset
        EMotionFX::AnimGraphGameControllerSettings& gameControllerSettings = animGraph->GetGameControllerSettings();
        EMotionFX::AnimGraphGameControllerSettings::Preset* preset = gameControllerSettings.GetActivePreset();

        // get access to the game controller and check if it is valid
        bool isGameControllerValid = false;

#if AZ_TRAIT_EMOTIONFX_HAS_GAME_CONTROLLER
        GameControllerWindow* gameControllerWindow = mPlugin->GetGameControllerWindow();
        if (gameControllerWindow)
        {
            isGameControllerValid = gameControllerWindow->GetIsGameControllerValid();
        }
#endif

        // only update in case a preset is selected and the game controller is valid
        if (preset && isGameControllerValid)
        {
            // check if the given parameter is controlled by a joystick and set its enabled state
            EMotionFX::AnimGraphGameControllerSettings::ParameterInfo* controllerParameterInfo = preset->FindParameterInfo(parameter->GetName().c_str());
            if (controllerParameterInfo)
            {
                controllerParameterInfo->m_enabled = isEnabled;
            }

            // do the same for the button infos
            preset->SetButtonEnabled(parameter->GetName().c_str(), isEnabled);
        }
    }

    void ParameterWindow::AddParameterToInterface(EMotionFX::AnimGraph* animGraph, const EMotionFX::Parameter* parameter, QTreeWidgetItem* parentWidgetItem)
    {
        // Only filter value parameters
        if (!mFilterString.empty()
            && azrtti_typeid(parameter) != azrtti_typeid<EMotionFX::GroupParameter>()
            && AzFramework::StringFunc::Find(parameter->GetName().c_str(), mFilterString.c_str()) == AZStd::string::npos)
        {
            return;
        }

        QTreeWidgetItem* widgetItem = new QTreeWidgetItem(parentWidgetItem);
        widgetItem->setText(0, parameter->GetName().c_str());
        widgetItem->setToolTip(0, parameter->GetDescription().c_str());
        widgetItem->setData(0, Qt::UserRole, QString(parameter->GetName().c_str()));
        parentWidgetItem->addChild(widgetItem);

        // check if the given parameter is selected
        if (GetIsParameterSelected(parameter->GetName()))
        {
            widgetItem->setSelected(true);
            if (mEnsureVisibility)
            {
                mTreeWidget->scrollToItem(widgetItem);
                mEnsureVisibility = false;
            }
        }

        if (azrtti_typeid(parameter) == azrtti_typeid<EMotionFX::GroupParameter>())
        {
            widgetItem->setExpanded(true);

            const EMotionFX::GroupParameter* groupParameter = static_cast<const EMotionFX::GroupParameter*>(parameter);

            const AZStd::string tooltip = AZStd::string::format("%zu Parameters", groupParameter->GetNumValueParameters());
            widgetItem->setToolTip(0, tooltip.c_str());
            widgetItem->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);

            // add all parameters that belong to the given group
            const EMotionFX::ParameterVector& childParameters = groupParameter->GetChildParameters();
            for (const EMotionFX::Parameter* childParameter : childParameters)
            {
                AddParameterToInterface(animGraph, childParameter, widgetItem);
            }
        }
        else
        {
            const EMotionFX::ValueParameter* valueParameter = static_cast<const EMotionFX::ValueParameter*>(parameter);
            const AZ::Outcome<size_t> parameterIndex = animGraph->FindValueParameterIndex(valueParameter);
            AZ_Assert(parameterIndex.IsSuccess(), "Expected a parameter belonging to the the anim graph");

            // check if the interface item needs to be read only or not
            bool isActuallyControlled, isEnabled;
            GetGamepadState(animGraph, parameter, &isActuallyControlled, &isEnabled);

            ParameterWidget parameterWidget;
            const AZStd::vector<MCore::Attribute*> attributes = GetAttributesForParameter(parameterIndex.GetValue());
            parameterWidget.m_valueParameterEditor.reset(ParameterEditorFactory::Create(animGraph, valueParameter, attributes));

            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            if (!serializeContext)
            {
                AZ_Error("EMotionFX", false, "Can't get serialize context from component application.");
                return;
            }
            parameterWidget.m_propertyEditor = aznew AzToolsFramework::ReflectedPropertyEditor(mTreeWidget);
            parameterWidget.m_propertyEditor->SetSizeHintOffset(QSize(0, 0));
            parameterWidget.m_propertyEditor->SetAutoResizeLabels(false);
            parameterWidget.m_propertyEditor->SetLeafIndentation(0);
            parameterWidget.m_propertyEditor->setStyleSheet("QFrame, .QWidget, QSlider, QCheckBox { background-color: transparent }");
            parameterWidget.m_propertyEditor->setFixedHeight(20);

            parameterWidget.m_propertyEditor->AddInstance(parameterWidget.m_valueParameterEditor.get(), azrtti_typeid(parameterWidget.m_valueParameterEditor.get()));
            parameterWidget.m_propertyEditor->Setup(serializeContext, this, false, 0);
            parameterWidget.m_propertyEditor->SetSelectionEnabled(true);
            parameterWidget.m_propertyEditor->show();
            parameterWidget.m_propertyEditor->ExpandAll();
            parameterWidget.m_propertyEditor->InvalidateAll();

            mTreeWidget->setItemWidget(widgetItem, 2, parameterWidget.m_propertyEditor);

            // create the gizmo widget in case the parameter is currently not being controlled by the gamepad
            QWidget* gizmoWidget = nullptr;
            if (isActuallyControlled)
            {
                QPushButton* gizmoButton = new QPushButton();
                gizmoButton->setCheckable(true);
                gizmoButton->setChecked(isEnabled);
                SetGamepadButtonTooltip(gizmoButton);
                gizmoButton->setProperty("attributeInfo", parameter->GetName().c_str());
                gizmoWidget = gizmoButton;
                connect(gizmoButton, &QPushButton::clicked, this, &ParameterWindow::OnGamepadControlToggle);
            }
            else
            {
                AzToolsFramework::ReflectedPropertyEditor* rpe = parameterWidget.m_propertyEditor.data();
                gizmoWidget = parameterWidget.m_valueParameterEditor->CreateGizmoWidget(
                        [rpe]()
                        {
                            rpe->InvalidateValues();
                        }
                        );
            }
            if (gizmoWidget)
            {
                mTreeWidget->setItemWidget(widgetItem, 1, gizmoWidget);
                mTreeWidget->setColumnWidth(1, 20);
            }

            auto insertIt = m_parameterWidgets.emplace(parameter, AZStd::move(parameterWidget));
            if (!insertIt.second)
            {
                insertIt.first->second = AZStd::move(parameterWidget);
            }
        }
    }

    // set the tooltip for a checkable gamepad gizmo button based on the state
    void ParameterWindow::SetGamepadButtonTooltip(QPushButton* button)
    {
        if (button->isChecked())
        {
            EMStudioManager::MakeTransparentButton(button, "Images/Icons/Gamepad.svg", "Parameter is currently being controlled by the gamepad", 20, 17);
        }
        else
        {
            EMStudioManager::MakeTransparentButton(button, "Images/Icons/Gamepad.svg", "Click button to enable gamepad control", 20, 17);
        }
    }

    void ParameterWindow::BeforePropertyModified(AzToolsFramework::InstanceDataNode*)
    {
    }

    void ParameterWindow::AfterPropertyModified(AzToolsFramework::InstanceDataNode*)
    {
    }

    void ParameterWindow::SetPropertyEditingActive(AzToolsFramework::InstanceDataNode*)
    {
    }

    void ParameterWindow::SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode*)
    {
    }

    void ParameterWindow::SealUndoStack()
    {
    }

    void ParameterWindow::RequestPropertyContextMenu(AzToolsFramework::InstanceDataNode*, const QPoint& point)
    {
        if (!m_animGraph || EMotionFX::GetRecorder().GetIsInPlayMode() || EMotionFX::GetRecorder().GetIsRecording())
        {
            return;
        }

        // create the context menu
        QMenu* menu = new QMenu(this);
        menu->setObjectName("EMFX.ParameterWindow.ContextMenu");

        // get the selected parameter index and make sure it is valid
        const EMotionFX::Parameter* parameter = GetSingleSelectedParameter();
        if (parameter)
        {
            // make the current value the default value for this parameter
            if (azrtti_typeid(parameter) != azrtti_typeid<EMotionFX::GroupParameter>())
            {
                EMotionFX::ActorInstance* actorInstance = GetCommandManager()->GetCurrentSelection().GetSingleActorInstance();
                if (actorInstance && actorInstance->GetAnimGraphInstance())
                {
                    QAction* makeDefaultAction = menu->addAction("Make default value");
                    connect(makeDefaultAction, &QAction::triggered, this, &ParameterWindow::OnMakeDefaultValue);
                }
            }

            // edit action
            QAction* editAction = menu->addAction("Edit");
            connect(editAction, &QAction::triggered, this, &ParameterWindow::OnEditButton);
        }
        if (!mSelectedParameterNames.empty())
        {
            menu->addSeparator();

            // select group parameter action
            QMenu* groupMenu = new QMenu("Assign to group", menu);
            QAction* noneGroupAction = groupMenu->addAction("Default");
            noneGroupAction->setCheckable(true);

            if (!parameter)
            {
                noneGroupAction->setChecked(true);
            }
            else
            {
                noneGroupAction->setChecked(false);
            }

            connect(noneGroupAction, &QAction::triggered, this, &ParameterWindow::OnGroupParameterSelected);

            // get the parent of the parameter
            const EMotionFX::GroupParameter* parentGroup = nullptr;
            if (parameter)
            {
                parentGroup = m_animGraph->FindParentGroupParameter(parameter);
            }

            // get the number of group parameters and iterate through them
            EMotionFX::GroupParameterVector groupParametersInCurrentParameter;
            if (azrtti_typeid(parameter) == azrtti_typeid<EMotionFX::GroupParameter>())
            {
                const EMotionFX::GroupParameter* groupParameter = static_cast<const EMotionFX::GroupParameter*>(parameter);
                groupParametersInCurrentParameter = groupParameter->RecursivelyGetChildGroupParameters();
            }
            const EMotionFX::GroupParameterVector allGroupParameters = m_animGraph->RecursivelyGetGroupParameters();
            for (const EMotionFX::GroupParameter* groupParameter : allGroupParameters)
            {
                if (groupParameter != parameter
                    && AZStd::find(groupParametersInCurrentParameter.begin(),
                        groupParametersInCurrentParameter.end(),
                        groupParameter) == groupParametersInCurrentParameter.end()
                    && AZStd::find(mSelectedParameterNames.begin(), mSelectedParameterNames.end(), groupParameter->GetName()) == mSelectedParameterNames.end())
                {
                    QAction* groupAction = groupMenu->addAction(groupParameter->GetName().c_str());
                    groupAction->setCheckable(true);
                    groupAction->setChecked(groupParameter == parentGroup);
                    connect(groupAction, &QAction::triggered, this, &ParameterWindow::OnGroupParameterSelected);
                }
            }

            menu->addMenu(groupMenu);
        }

        menu->addSeparator();

        // add parameter action
        QAction* addParameter = menu->addAction("Add parameter");
        connect(addParameter, &QAction::triggered, this, &ParameterWindow::OnAddParameter);

        // add group action
        QAction* addGroupAction = menu->addAction("Add group");
        connect(addGroupAction, &QAction::triggered, this, &ParameterWindow::OnAddGroup);

        menu->addSeparator();

        // remove action
        if (!mSelectedParameterNames.empty())
        {
            QAction* removeAction = menu->addAction("Remove");
            connect(removeAction, &QAction::triggered, this, &ParameterWindow::OnRemoveSelected);
        }

        // clear action
        if (m_animGraph->GetNumParameters() > 0)
        {
            QAction* clearAction = menu->addAction("Clear");
            connect(clearAction, &QAction::triggered, this, &ParameterWindow::OnClearButton);
        }

        // show the menu at the given position
        if (menu->isEmpty() == false)
        {
            menu->popup(point);
        }
        connect(menu, &QMenu::triggered, menu, &QMenu::deleteLater);

    }

    void ParameterWindow::PropertySelectionChanged(AzToolsFramework::InstanceDataNode*, bool)
    {
    }

    // triggered when pressing one of the gamepad gizmo buttons
    void ParameterWindow::OnGamepadControlToggle()
    {
        if (!m_animGraph)
        {
            return;
        }

        if (EMotionFX::GetRecorder().GetIsInPlayMode() && EMotionFX::GetRecorder().GetRecordTime() > MCore::Math::epsilon)
        {
            return;
        }

        MCORE_ASSERT(sender()->inherits("QPushButton"));
        QPushButton* button = qobject_cast<QPushButton*>(sender());
        SetGamepadButtonTooltip(button);

        const AZStd::string attributeInfoName = button->property("attributeInfo").toString().toUtf8().data();

        const EMotionFX::Parameter* parameter = m_animGraph->FindParameterByName(attributeInfoName);
        if (parameter)
        {
            // update the game controller settings
            SetGamepadState(m_animGraph, parameter, button->isChecked());

            // update the interface
            ParameterWidgetByParameter::const_iterator paramWidgetIt = m_parameterWidgets.find(parameter);
            if (paramWidgetIt != m_parameterWidgets.end())
            {
                paramWidgetIt->second.m_valueParameterEditor->setIsReadOnly(button->isChecked());
                paramWidgetIt->second.m_propertyEditor->InvalidateAll();
            }
        }
    }


    // enable/disable recording/playback mode
    void ParameterWindow::OnRecorderStateChanged()
    {
        const bool readOnly = (EMotionFX::GetRecorder().GetIsInPlayMode()); // disable when in playback mode, enable otherwise
        if (m_animGraph)
        {
            // update parameter values
            UpdateParameterValues();
        }

        // update the interface
        UpdateInterface();
    }


    // update the interface attribute widgets with current parameter values
    void ParameterWindow::UpdateParameterValues()
    {
        for (ParameterWidgetByParameter::value_type& param : m_parameterWidgets)
        {
            param.second.m_valueParameterEditor->UpdateValue();
            param.second.m_propertyEditor->InvalidateValues();
        }
    }


    void ParameterWindow::Reinit(bool forceReinit)
    {
        mLockSelection = true;

        // Early out in case we're already showing the parameters from the focused anim graph.
        if (!forceReinit && m_animGraph == mPlugin->GetAnimGraphModel().GetFocusedAnimGraph())
        {
            UpdateAttributesForParameterWidgets();
            UpdateInterface();
            mLockSelection = false;
            return;
        }

        m_animGraph = mPlugin->GetAnimGraphModel().GetFocusedAnimGraph();
        qobject_cast<ParameterWindowTreeWidget*>(mTreeWidget)->SetAnimGraph(m_animGraph);

        // First clear the parameter widgets array and then the actual tree widget.
        // Don't change the order here as the tree widget clear call calls an on selection changed which uses the parameter widget array.
        m_parameterWidgets.clear();
        mTreeWidget->clear();

        if (!m_animGraph)
        {
            UpdateInterface();
            mLockSelection = false;
            return;
        }

        // add all parameters, this will recursively add parameters to groups
        const EMotionFX::ParameterVector& childParameters = m_animGraph->GetChildParameters();
        for (const EMotionFX::Parameter* parameter : childParameters)
        {
            AddParameterToInterface(m_animGraph, parameter, mTreeWidget->invisibleRootItem());
        }

        mLockSelection = false;

        UpdateAttributesForParameterWidgets();
        UpdateInterface();
    }


    void ParameterWindow::SingleSelectGroupParameter(const char* groupName, bool ensureVisibility, bool updateInterface)
    {
        mSelectedParameterNames.clear();

        mSelectedParameterNames.push_back(groupName);

        mEnsureVisibility = ensureVisibility;

        if (updateInterface)
        {
            UpdateInterface();
        }
    }

    void ParameterWindow::SelectParameters(const AZStd::vector<AZStd::string>& parameterNames, bool updateInterface)
    {
        mTreeWidget->clearSelection();
        for (const AZStd::string& parameterName : parameterNames)
        {
            const QList<QTreeWidgetItem*> foundItems = mTreeWidget->findItems(parameterName.c_str(), Qt::MatchFixedString);
            for (QTreeWidgetItem* foundItem : foundItems)
            {
                foundItem->setSelected(true);
            }
        }
        UpdateSelectionArrays();

        if (updateInterface)
        {
            UpdateInterface();
        }
    }



    void ParameterWindow::OnTextFilterChanged(const QString& text)
    {
        mFilterString = text.toUtf8().data();
        Reinit(/*forceReinit=*/true);
    }


    void ParameterWindow::OnSelectionChanged()
    {
        // update the local arrays which store the selected group parameters and parameter names
        UpdateSelectionArrays();

        // update the interface
        UpdateInterface();
    }


    void ParameterWindow::CanMove(bool* outMoveUpPossible, bool* outMoveDownPossible)
    {
        // init the values
        *outMoveUpPossible = false;
        *outMoveDownPossible = false;

        if (!m_animGraph)
        {
            return;
        }

        const EMotionFX::Parameter* parameter = GetSingleSelectedParameter();
        if (!parameter)
        {
            return;
        }

        // To detect if we can move up or down, we are going to get a flat list of all the parameters belonging
        // to the same group (non-recursive) then find the parameter.
        // If the parameter is the first one, we can only move up if we are in a group (this will move the parameter
        // to the parent group, making it a sibling of the group)
        // If the parameter is the last of the list, then we can move down if are in a group
        const EMotionFX::GroupParameter* parentGroup = m_animGraph->FindParentGroupParameter(parameter);

        // If we have a parent group, we dont need to inspect the siblings, we can always move up/down
        if (parentGroup)
        {
            *outMoveUpPossible = true;
            *outMoveDownPossible = true;
            return;
        }

        const EMotionFX::ParameterVector& siblingParameters = parentGroup ? parentGroup->GetChildParameters() : m_animGraph->GetChildParameters();
        AZ_Assert(!siblingParameters.empty(), "Expected at least one parameter (the one we are analyzing)");

        if (*siblingParameters.begin() != parameter)
        {
            *outMoveUpPossible = true;
        }
        if (*siblingParameters.rbegin() != parameter)
        {
            *outMoveDownPossible = true;
        }
    }


    // update the interface
    void ParameterWindow::UpdateInterface()
    {
        if (!m_animGraph || EMotionFX::GetRecorder().GetIsInPlayMode() || EMotionFX::GetRecorder().GetIsRecording())
        {
            m_addAction->setEnabled(false);
            m_editAction->setEnabled(false);
            return;
        }

        // always allow to add a parameter when there is a anim graph selected
        m_addAction->setEnabled(true);

        // disable the remove and edit buttton if we dont have any parameter selected
        m_editAction->setEnabled(true);
        if (mSelectedParameterNames.empty())
        {
            m_editAction->setEnabled(false);
        }

        // check if we can move up/down the currently single selected item
        bool moveUpPossible, moveDownPossible;
        CanMove(&moveUpPossible, &moveDownPossible);

        bool isAnimGraphActive = mPlugin->IsAnimGraphActive(m_animGraph);

        // Make the parameter widgets read-only in case they are either controlled by the gamepad or the anim graph is not running on an actor instance.
        for (const auto& iterator : m_parameterWidgets)
        {
            const EMotionFX::Parameter* parameter = iterator.first;

            bool isActuallyControlled, isEnabled;
            GetGamepadState(m_animGraph, parameter, &isActuallyControlled, &isEnabled);
            const bool isGamepadConrolled = isActuallyControlled && isEnabled;

            const bool readOnly = isGamepadConrolled || !isAnimGraphActive;
            const bool oldIsReadOnly = iterator.second.m_valueParameterEditor->IsReadOnly();
            if (readOnly != oldIsReadOnly)
            {
                iterator.second.m_valueParameterEditor->setIsReadOnly(readOnly);
                iterator.second.m_propertyEditor->InvalidateAll();
            }
        }
    }


    AZStd::vector<MCore::Attribute*> ParameterWindow::GetAttributesForParameter(size_t parameterIndex) const
    {
        AZStd::vector<MCore::Attribute*> result;

        const CommandSystem::SelectionList& selectionList = GetCommandManager()->GetCurrentSelection();
        const uint32 numActorInstances = selectionList.GetNumSelectedActorInstances();
        for (uint32 i = 0; i < numActorInstances; ++i)
        {
            const EMotionFX::ActorInstance* actorInstance = selectionList.GetActorInstance(i);
            const EMotionFX::AnimGraphInstance* animGraphInstance = actorInstance->GetAnimGraphInstance();
            if (animGraphInstance && animGraphInstance->GetAnimGraph() == m_animGraph)
            {
                result.emplace_back(animGraphInstance->GetParameterValue(static_cast<uint32>(parameterIndex)));
            }
        }

        return result;
    }

    void ParameterWindow::UpdateAttributesForParameterWidgets()
    {
        if (!m_animGraph)
        {
            return;
        }

        for (const auto& iterator : m_parameterWidgets)
        {
            const EMotionFX::Parameter* parameter = iterator.first;

            const EMotionFX::ValueParameter* valueParameter = azdynamic_cast<const EMotionFX::ValueParameter*>(parameter);
            if (valueParameter)
            {
                const AZ::Outcome<size_t> parameterIndex = m_animGraph->FindValueParameterIndex(valueParameter);
                AZ_Assert(parameterIndex.IsSuccess(), "Expected a parameter belonging to the the anim graph");

                const AZStd::vector<MCore::Attribute*> attributes = GetAttributesForParameter(parameterIndex.GetValue());
                iterator.second.m_valueParameterEditor->SetAttributes(attributes);
                // Also update the parameter value after the attributes updated.
                iterator.second.m_valueParameterEditor->UpdateValue();
                iterator.second.m_propertyEditor->InvalidateValues();
            }
        }
    }

    void ParameterWindow::OnAddParameter()
    {
        if (!m_animGraph)
        {
            return;
        }

        ParameterCreateEditDialog* createEditParameterDialog = new ParameterCreateEditDialog(mPlugin, this);
        createEditParameterDialog->Init();

        EMStudio::ParameterCreateEditDialog::connect(createEditParameterDialog, &QDialog::finished, [=](int resultCode)
            {
                if (resultCode == QDialog::Rejected)
                {
                    delete createEditParameterDialog;
                    return;
                }

                AZStd::string commandResult;
                AZStd::string commandString;
                MCore::CommandGroup commandGroup("Add parameter");

                // Construct the create parameter command and add it to the command group.
                const AZStd::unique_ptr<EMotionFX::Parameter>& parameter = createEditParameterDialog->GetParameter();

                CommandSystem::ConstructCreateParameterCommand(commandString, m_animGraph, parameter.get(), MCORE_INVALIDINDEX32);
                commandGroup.AddCommandString(commandString);

                const EMotionFX::GroupParameter* parentGroup = nullptr;
                const EMotionFX::Parameter* selectedParameter = GetSingleSelectedParameter();
                // if we have a group selected add the new parameter to this group
                if (selectedParameter)
                {
                    if (azrtti_typeid(selectedParameter) == azrtti_typeid<EMotionFX::GroupParameter>())
                    {
                        parentGroup = static_cast<const EMotionFX::GroupParameter*>(selectedParameter);
                    }
                    else
                    {
                        // add it as sibling of the current selected parameter
                        parentGroup = m_animGraph->FindParentGroupParameter(selectedParameter);
                    }
                }

                if (parentGroup)
                {
                    commandString = AZStd::string::format("AnimGraphAdjustGroupParameter -animGraphID %d -name \"%s\" -parameterNames \"%s\" -action \"add\"",
                        m_animGraph->GetID(),
                        parentGroup->GetName().c_str(),
                        parameter->GetName().c_str());
                    commandGroup.AddCommandString(commandString);
                }

                AZStd::string result;
                if (!GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
                {
                    AZ_Error("EMotionFX", false, result.c_str());
                }
                delete createEditParameterDialog;
            });

        createEditParameterDialog->open();
    }


    // edit a new parameter
    void ParameterWindow::OnEditButton()
    {
        if (!m_animGraph)
        {
            return;
        }

        // get the selected parameter index and make sure it is valid
        const EMotionFX::Parameter* parameter = GetSingleSelectedParameter();
        if (!parameter)
        {
            return;
        }

        const AZStd::string oldName = parameter->GetName();

        // create and init the dialog
        ParameterCreateEditDialog* dialog = new ParameterCreateEditDialog(mPlugin, this, parameter);
        dialog->Init();
        // We cannot use exec here as we need to access it from the tests
        EMStudio::ParameterCreateEditDialog::connect(dialog, &QDialog::finished, [=](int resultCode)
            {
                dialog->deleteLater();
                if (resultCode == QDialog::Rejected)
                {
                    return;
                }

                // convert the interface type into a string
                const AZStd::unique_ptr<EMotionFX::Parameter>& editedParameter = dialog->GetParameter();
                const AZStd::string contents = MCore::ReflectionSerializer::Serialize(editedParameter.get()).GetValue();

                const bool isGroupParameter = (azrtti_typeid(parameter) == azrtti_typeid<EMotionFX::GroupParameter>());
                const AZStd::string commandGroupName = AZStd::string::format("%s parameter%s",
                    oldName == editedParameter->GetName() ? "Adjust" : "Rename",
                    isGroupParameter ? " group" : "");
                MCore::CommandGroup commandGroup(commandGroupName);

                if (!isGroupParameter)
                {
                    const AZ::TypeId oldTypeId = azrtti_typeid(parameter);
                    const AZ::TypeId newTypeId = azrtti_typeid(editedParameter.get());

                    if (oldTypeId != newTypeId)
                    {
                        // Add commands to remove connections from any existing port on a
                        // parameter node from this parameter

                        // Make a new port with the correct new type, to test the connection validity
                        EMotionFX::AnimGraphNode::Port newPort;
                        if (const EMotionFX::ValueParameter* valueParameter = azrtti_cast<EMotionFX::ValueParameter*>(editedParameter.get()))
                        {
                            newPort.mCompatibleTypes[0] = valueParameter->GetType();
                        }

                        // Get the list of all parameter nodes
                        AZStd::vector<EMotionFX::AnimGraphNode*> parameterNodes;
                        m_animGraph->RecursiveCollectNodesOfType(azrtti_typeid<EMotionFX::BlendTreeParameterNode>(), &parameterNodes);
                        for (const EMotionFX::AnimGraphNode* parameterNode : parameterNodes)
                        {
                            // Get the list of connections from the port whose type is
                            // being changed
                            const uint32 sourcePortIndex = parameterNode->FindOutputPortIndex(parameter->GetName().c_str());

                            AZStd::vector<AZStd::pair<EMotionFX::BlendTreeConnection*, EMotionFX::AnimGraphNode*>> outgoingConnectionsFromThisPort;
                            parameterNode->CollectOutgoingConnections(outgoingConnectionsFromThisPort, sourcePortIndex);

                            // Verify that the connection will still be valid with the new type
                            for (const auto& connection : outgoingConnectionsFromThisPort)
                            {
                                const EMotionFX::AnimGraphNode* targetNode = connection.second;
                                const EMotionFX::AnimGraphNode::Port& targetPort = targetNode->GetInputPort(connection.first->GetTargetPort());
                                bool isCompatible = newPort.CheckIfIsCompatibleWith(targetPort);

                                if (!isCompatible)
                                {
                                    // Delete the connection
                                    const AZStd::string removeConnectionCommand = AZStd::string::format(
                                        "AnimGraphRemoveConnection"
                                        " -animGraphID %d"
                                        " -sourceNode \"%s\""
                                        " -sourcePort %d"
                                        " -targetNode \"%s\""
                                        " -targetPort %d",
                                        m_animGraph->GetID(),
                                        parameterNode->GetName(),
                                        connection.first->GetSourcePort(),
                                        targetNode->GetName(),
                                        connection.first->GetTargetPort()
                                        );
                                    commandGroup.AddCommandString(removeConnectionCommand.c_str());
                                }
                            }
                        }
                    }

                    // Build the command string and execute it.
                    const AZStd::string commandString = AZStd::string::format("AnimGraphAdjustParameter -animGraphID %i -name \"%s\" -newName \"%s\" -type \"%s\" -contents {%s}",
                        m_animGraph->GetID(),
                        oldName.c_str(),
                        editedParameter->GetName().c_str(),
                        azrtti_typeid(editedParameter.get()).ToString<AZStd::string>().c_str(),
                        contents.c_str());
                    commandGroup.AddCommandString(commandString);
                }
                else
                {
                    AZStd::string commandString = AZStd::string::format("AnimGraphAdjustGroupParameter -animGraphID %d -name \"%s\" -description \"%s\"",
                        m_animGraph->GetID(),
                        oldName.c_str(),
                        editedParameter->GetDescription().c_str());

                    if (oldName != editedParameter->GetName())
                    {
                        commandString += AZStd::string::format(" -newName \"%s\"", editedParameter->GetName().c_str());
                    }

                    commandGroup.AddCommandString(commandString);
                }

                if (!commandGroup.IsEmpty())
                {
                    AZStd::string result;
                    if (!GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
                    {
                        AZ_Error("EMotionFX", false, result.c_str());
                    }
                }
            });
        dialog->open();
    }


    void ParameterWindow::UpdateSelectionArrays()
    {
        // only update the selection in case it is not locked
        if (mLockSelection)
        {
            return;
        }

        // clear the selection
        mSelectedParameterNames.clear();

        if (!m_animGraph)
        {
            return;
        }

        // make sure we only have exactly one selected item
        QList<QTreeWidgetItem*> selectedItems = mTreeWidget->selectedItems();
        int32 numSelectedItems = selectedItems.count();

        for (int32 i = 0; i < numSelectedItems; ++i)
        {
            // get the selected item
            QTreeWidgetItem* selectedItem = selectedItems[i];
            mSelectedParameterNames.emplace_back(selectedItem->data(0, Qt::UserRole).toString().toUtf8().data());
        }
    }


    // get the index of the selected parameter
    const EMotionFX::Parameter* ParameterWindow::GetSingleSelectedParameter() const
    {
        if (mSelectedParameterNames.size() != 1)
        {
            return nullptr;
        }

        if (!m_animGraph)
        {
            return nullptr;
        }

        // find and return the index of the parameter in the anim graph
        return m_animGraph->FindParameterByName(mSelectedParameterNames[0]);
    }


    // remove the selected parameters and groups
    void ParameterWindow::OnRemoveSelected()
    {
        if (MCore::GetLogManager().GetLogLevels() & MCore::LogCallback::LOGLEVEL_INFO)
        {
            // log the parameters and the group parameters
            const EMotionFX::ValueParameterVector& valueParameters = m_animGraph->RecursivelyGetValueParameters();
            const size_t logNumParams = valueParameters.size();
            MCore::LogInfo("=================================================");
            MCore::LogInfo("Parameters: (%i)", logNumParams);
            for (size_t p = 0; p < logNumParams; ++p)
            {
                MCore::LogInfo("Parameter #%i: Name='%s'", p, valueParameters[p]->GetName().c_str());
            }
            const EMotionFX::GroupParameterVector groupParameters = m_animGraph->RecursivelyGetGroupParameters();
            const size_t logNumGroups = groupParameters.size();
            MCore::LogInfo("Group parameters: (%i)", logNumGroups);
            for (uint32 g = 0; g < logNumGroups; ++g)
            {
                const EMotionFX::GroupParameter* groupParam = groupParameters[g];
                MCore::LogInfo("Group parameter #%i: Name='%s'", g, groupParam->GetName().c_str());
                const EMotionFX::ValueParameterVector childValueParams = groupParam->GetChildValueParameters();
                for (const EMotionFX::ValueParameter* childValueParam : childValueParams)
                {
                    MCore::LogInfo("   + Parameter: Name='%s'", childValueParam->GetName().c_str());
                }
            }
        }

        // check if the anim graph is valid
        if (!m_animGraph)
        {
            return;
        }

        MCore::CommandGroup commandGroup("Remove parameters/groups");

        AZStd::vector<AZStd::string> paramsOfSelectedGroup;
        AZStd::vector<AZStd::string> selectedValueParameters;

        // get the number of selected parameters and iterate through them
        for (const AZStd::string& selectedParameter : mSelectedParameterNames)
        {
            const EMotionFX::Parameter* parameter = m_animGraph->FindParameterByName(selectedParameter);
            if (!parameter)
            {
                continue;
            }
            if (azrtti_typeid(parameter) == azrtti_typeid<EMotionFX::GroupParameter>())
            {
                // remove the group parameter
                const EMotionFX::GroupParameter* groupParameter = static_cast<const EMotionFX::GroupParameter*>(parameter);
                CommandSystem::RemoveGroupParameter(m_animGraph, groupParameter, false, &commandGroup);

                // check if we have selected all parameters inside the group
                // if not we should ask if we want to remove them along with the group
                const EMotionFX::ParameterVector parametersInGroup = groupParameter->RecursivelyGetChildParameters();
                for (const EMotionFX::Parameter* parameter2 : parametersInGroup)
                {
                    const AZStd::string& parameterName = parameter2->GetName();
                    if (AZStd::find(mSelectedParameterNames.begin(), mSelectedParameterNames.end(), parameterName) == mSelectedParameterNames.end())
                    {
                        paramsOfSelectedGroup.push_back(parameterName);
                    }
                }
            }
            else
            {
                selectedValueParameters.push_back(selectedParameter);
            }
        }

        CommandSystem::BuildRemoveParametersCommandGroup(m_animGraph, selectedValueParameters, &commandGroup);

        if (!paramsOfSelectedGroup.empty())
        {
            const QMessageBox::StandardButton result = QMessageBox::question(this, "Remove parameters along with the groups?", "Would you also like to remove the parameters inside the group? Clicking no will move them into the root.",
                    QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (result == QMessageBox::Yes)
            {
                // Remove the contained parameters, since they can be groups or regular parameters, we
                // iterate over them moving the groups to a different vector to be deleted after
                AZStd::vector<const EMotionFX::GroupParameter*> groupParameters;

                AZStd::vector<AZStd::string>::const_iterator it = paramsOfSelectedGroup.begin();
                while (it != paramsOfSelectedGroup.end())
                {
                    const EMotionFX::GroupParameter* groupParameter = m_animGraph->FindGroupParameterByName(*it);
                    if (groupParameter)
                    {
                        groupParameters.push_back(groupParameter);
                        it = paramsOfSelectedGroup.erase(it);
                    }
                    else
                    {
                        ++it;
                    }
                }
                CommandSystem::BuildRemoveParametersCommandGroup(m_animGraph, paramsOfSelectedGroup, &commandGroup);
                for (const EMotionFX::GroupParameter* groupParameter : groupParameters)
                {
                    CommandSystem::RemoveGroupParameter(m_animGraph, groupParameter, false, &commandGroup);
                }
            }
        }

        // Execute the command group.
        AZStd::string result;
        if (!GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }

    void ParameterWindow::ClearParameters(bool showConfirmationDialog)
    {
        if (!m_animGraph)
        {
            return;
        }

        // ask the user if he really wants to remove all parameters
        if (showConfirmationDialog &&
            QMessageBox::question(this, "Remove all groups and parameters?", "Are you sure you want to remove all parameters and all group parameters from the anim graph?", QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
        {
            return;
        }

        MCore::CommandGroup commandGroup("Clear parameters/groups");

        // add the commands to remove all groups and parameters
        CommandSystem::ClearParametersCommand(m_animGraph, &commandGroup);
        CommandSystem::ClearGroupParameters(m_animGraph, &commandGroup);

        AZStd::string result;
        if (!GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }

    int ParameterWindow::GetTopLevelItemCount() const
    {
        return mTreeWidget->topLevelItemCount();
    }

    // move parameter under a specific parent, at a determined index
    void ParameterWindow::OnMoveParameterTo(int idx, const QString& parameter, const QString& parent)
    {
        if (!m_animGraph)
        {
            return;
        }

        // If index is less than zero, move the parameter to the top.
        if (idx < 0)
        {
            idx = 0;
        }

        AZStd::string commandString;
        commandString = AZStd::string::format("AnimGraphMoveParameter -animGraphID %d -name \"%s\" -index %d ",
                m_animGraph->GetID(),
                parameter.toUtf8().data(),
                idx);
        if (!parent.isEmpty())
        {
            commandString += AZStd::string::format("-parent \"%s\"", parent.toUtf8().data());
        }

        AZStd::string result;
        if (!GetCommandManager()->ExecuteCommand(commandString, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }

    void ParameterWindow::contextMenuEvent(QContextMenuEvent* event)
    {
        RequestPropertyContextMenu(nullptr, event->globalPos());
    }


    void ParameterWindow::OnGroupParameterSelected()
    {
        assert(sender()->inherits("QAction"));
        QAction* action = qobject_cast<QAction*>(sender());

        if (!m_animGraph)
        {
            return;
        }

        // get the number of selected parameters and return directly in case there aren't any selected
        const size_t numSelectedParameters = mSelectedParameterNames.size();
        if (numSelectedParameters == 0)
        {
            return;
        }

        // construct the name of the group parameter
        const AZStd::string commandGroupName = (numSelectedParameters == 1)
            ? "Assign parameter to group"
            : "Assign parameters to group";

        AZStd::string commandString;
        MCore::CommandGroup commandGroup(commandGroupName);

        // target group parameter
        const AZStd::string groupParameterName = action->text().toUtf8().data();
        const EMotionFX::GroupParameter* groupParameter = m_animGraph->FindGroupParameterByName(groupParameterName);

        AZStd::string parameterNames;
        AZ::StringFunc::Join(parameterNames, begin(mSelectedParameterNames), end(mSelectedParameterNames), ";");
        if (groupParameter)
        {
            commandString = AZStd::string::format(R"(AnimGraphAdjustGroupParameter -animGraphID %d -name "%s" -parameterNames "%s" -action "add")",
                    m_animGraph->GetID(),
                    groupParameter->GetName().c_str(),
                    parameterNames.c_str());
            commandGroup.AddCommandString(commandString);
        }
        else
        {
            commandString = AZStd::string::format(R"(AnimGraphAdjustGroupParameter -animGraphID %d -parameterNames "%s" -action "clear")",
                    m_animGraph->GetID(),
                    parameterNames.c_str());
            commandGroup.AddCommandString(commandString);
        }

        // Execute the command group.
        AZStd::string result;
        if (!GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }


    // Set the instance parameter value to the parameter's default value.
    void ParameterWindow::OnMakeDefaultValue()
    {
        EMotionFX::ActorInstance* actorInstance = GetCommandManager()->GetCurrentSelection().GetSingleActorInstance();
        if (!actorInstance)
        {
            QMessageBox::warning(this, "Selection Issue", "We cannot perform this operation while you have multiple actor instances selected!");
            return;
        }

        EMotionFX::Parameter* parameter = const_cast<EMotionFX::Parameter*>(GetSingleSelectedParameter());
        if (!parameter)
        {
            return;
        }

        EMotionFX::ValueParameter* valueParameter = azdynamic_cast<EMotionFX::ValueParameter*>(parameter);
        if (!valueParameter)
        {
            return;
        }

        EMotionFX::AnimGraphInstance* animGraphInstance = actorInstance->GetAnimGraphInstance();
        if (!animGraphInstance)
        {
            return;
        }

        if (m_animGraph != animGraphInstance->GetAnimGraph())
        {
            return;
        }

        const AZ::Outcome<size_t> valueParameterIndex = m_animGraph->FindValueParameterIndex(valueParameter);
        if (valueParameterIndex.IsSuccess())
        {
            MCore::Attribute* instanceValue = animGraphInstance->GetParameterValue(static_cast<uint32>(valueParameterIndex.GetValue()));
            valueParameter->SetDefaultValueFromAttribute(instanceValue);

            m_animGraph->SetDirtyFlag(true);
        }
    }


    void ParameterWindow::OnAddGroup()
    {
        if (!m_animGraph)
        {
            return;
        }

        // Fill in the invalid names array. A group parameter cannot have the same name as the other group, or other parameters.
        AZStd::vector<AZStd::string> invalidNames;
        const EMotionFX::GroupParameterVector groupParameters = m_animGraph->RecursivelyGetGroupParameters();
        for (const EMotionFX::GroupParameter* groupParameter : groupParameters)
        {
            invalidNames.push_back(groupParameter->GetName());
        }
        const EMotionFX::ValueParameterVector& valueParameters = m_animGraph->RecursivelyGetValueParameters();
        for (const EMotionFX::ValueParameter* valueParameter : valueParameters)
        {
            invalidNames.push_back(valueParameter->GetName());
        }

        // generate a unique group name
        const AZStd::string uniqueGroupName = MCore::GenerateUniqueString("Group", [&invalidNames](const AZStd::string& value)
                {
                    return AZStd::find(invalidNames.begin(), invalidNames.end(), value) == invalidNames.end();
                });

        // show the create window
        auto createWindow = new ParameterCreateRenameWindow("Create Group", "Please enter the group name:", uniqueGroupName.c_str(), "", invalidNames, this);
        connect(createWindow, &QDialog::finished, this, [this, createWindow]()
        {
            createWindow->deleteLater();

            AZStd::string command = AZStd::string::format("AnimGraphAddGroupParameter -animGraphID %i -name \"%s\"", m_animGraph->GetID(), createWindow->GetName().c_str());
            const EMotionFX::GroupParameter* parentGroup = nullptr;
            const EMotionFX::Parameter* selectedParameter = GetSingleSelectedParameter();

            // if we have a group selected add the new parameter to this group
            if (selectedParameter)
            {
                if (azrtti_typeid(selectedParameter) == azrtti_typeid<EMotionFX::GroupParameter>())
                {
                    parentGroup = static_cast<const EMotionFX::GroupParameter*>(selectedParameter);
                }
                else
                {
                    // add it as sibling of the current selected parameter
                    parentGroup = m_animGraph->FindParentGroupParameter(selectedParameter);
                }
            }
            if (parentGroup)
            {
                // create the group as a child of the current selected group parameter
                command += AZStd::string::format(" -parent \"%s\"", parentGroup->GetName().c_str());
            }

            // select our new group directly (this needs UpdateInterface() to be called, but the command does that internally)
            SingleSelectGroupParameter(createWindow->GetName().c_str(), true);

            // Execute command.
            AZStd::string result;
            if (!GetCommandManager()->ExecuteCommand(command, result))
            {
                AZ_Error("EMotionFX", false, result.c_str());
            }
        });

        createWindow->open();
    }


    void ParameterWindow::OnGroupExpanded(QTreeWidgetItem* /*item*/)
    {
        // Collapse/expanded state was being saved in the animgraph file. This can cause multiple users that are using the animgraph
        // to see the file dirty because of the collapsed state. This setting likely should be by user and stored in a separate file.
        // The RPE supports serializing this state.
        //// get the anim graph
        //if (!m_animGraph)
        //{
        //    return;
        //}

        //// find the group parameter
        //const EMotionFX::GroupParameter* groupParameter = animGraph->FindGroupParameterByName(FromQtString(item->text(0)));
        //if (groupParameter)
        //{
        //    groupParameter->SetIsCollapsed(false);
        //}
    }


    void ParameterWindow::OnGroupCollapsed(QTreeWidgetItem* /*item*/)
    {
        // Collapse/expanded state was being saved in the animgraph file. This can cause multiple users that are using the animgraph
        // to see the file dirty because of the collapsed state. This setting likely should be by user and stored in a separate file.
        // The RPE supports serializing this state.
        //// get the anim graph
        //if (!m_animGraph)
        //{
        //    return;
        //}

        //// find the group parameter
        //EMotionFX::GroupParameter* groupParameter = animGraph->FindGroupParameterByName(FromQtString(item->text(0)).c_str());
        //if (groupParameter)
        //{
        //    groupParameter->SetIsCollapsed(true);
        //}
    }


    void ParameterWindow::OnFocusChanged(const QModelIndex& newFocusIndex, const QModelIndex& newFocusParent, const QModelIndex& oldFocusIndex, const QModelIndex& oldFocusParent)
    {
        AZ_UNUSED(newFocusIndex);
        AZ_UNUSED(oldFocusIndex);

        if (newFocusParent.isValid() && newFocusParent == oldFocusParent)
        {
            // Still focusing on the same parent, no need to reinit.
            return;
        }

        Reinit();
    }


    void ParameterWindow::keyPressEvent(QKeyEvent* event)
    {
        switch (event->key())
        {
        case Qt::Key_Delete:
        {
            OnRemoveSelected();
            event->accept();
            break;
        }
        case Qt::Key_PageUp:
        {
            event->accept();
            break;
        }
        case Qt::Key_PageDown:
        {
            event->accept();
            break;
        }
        default:
            event->ignore();
        }
    }


    void ParameterWindow::keyReleaseEvent(QKeyEvent* event)
    {
        switch (event->key())
        {
        case Qt::Key_Delete:
        {
            event->accept();
            break;
        }
        case Qt::Key_PageUp:
        {
            event->accept();
            break;
        }
        case Qt::Key_PageDown:
        {
            event->accept();
            break;
        }
        default:
            event->ignore();
        }
    }


    // Find the parameter widget for a given parameter.
    void ParameterWindow::UpdateParameterValue(const EMotionFX::Parameter* parameter)
    {
        ParameterWidgetByParameter::const_iterator itWidget = m_parameterWidgets.find(parameter);
        if (itWidget != m_parameterWidgets.end())
        {
            itWidget->second.m_valueParameterEditor->UpdateValue();
            itWidget->second.m_propertyEditor->InvalidateValues();
        }
    }

    class ParameterWindowTreeWidgetPrivate
        : public QAbstractItemViewPrivate
    {
        Q_DECLARE_PUBLIC(ParameterWindowTreeWidget);
    };

    ParameterWindowTreeWidget::ParameterWindowTreeWidget(QWidget* parent)
        : QTreeWidget(parent)
    {
    }

    void ParameterWindowTreeWidget::SetAnimGraph(EMotionFX::AnimGraph* animGraph)
    {
        m_animGraph = animGraph;
    }

    void ParameterWindowTreeWidget::startDrag(Qt::DropActions supportedActions)
    {
        [[maybe_unused]] Q_D(ParameterWindowTreeWidget);

        QModelIndexList indexes = selectedIndexes();
        if (indexes.count() > 0)
        {
            m_draggedParam = itemFromIndex(indexes[0]);
            m_draggedParentParam = m_draggedParam ? m_draggedParam->parent() : nullptr;
        }

        // StartDrag is not synchronous and will only return when drag ends. If we allow application mode change, this widget could get destroy and
        // so does the qt drag object. Thus we need to prevent application mode from changing at all.
        const bool signalBlocked = GetMainWindow()->GetApplicationModeComboBox()->signalsBlocked();
        GetMainWindow()->GetApplicationModeComboBox()->blockSignals(true);

        QAbstractItemView::startDrag(supportedActions);

        GetMainWindow()->GetApplicationModeComboBox()->blockSignals(signalBlocked);

        // Why sending a drag ended signal?
        // We enabled the InternalMove mode on this widget to support moving item within this widget. But if any parameter is
        // dropped to another widget, it will remove the underlying item without calling drop event on this widget, and create a
        // desync between the parameters and the treeview item.
        // To solve this problem, we will force reinit the parameter window everytime a DND operation end.
        emit DragEnded();
    }

    void ParameterWindowTreeWidget::dropEvent(QDropEvent* event)
    {
        Q_D(ParameterWindowTreeWidget);

        QModelIndex topIndex;
        int col = -1;
        int row = -1;

        // Getting the target drop index from the private implementation
        // of QAbstractItemView
        //
        // Documentation from QAbstractItemViewPrivate::dropOn
        // if (row == -1 && col == -1)
        //     // append to this drop index
        // else
        //     // place at row, col in drop index

        if (d->dropOn(event, &row, &col, &topIndex))
        {
            QTreeWidgetItem* item = itemFromIndex(topIndex);
            item = item ? item : invisibleRootItem();

            const QString& dragParamName = m_draggedParam->data(0, Qt::UserRole).toString();
            const QString& dropTopParamName = item->data(0, Qt::UserRole).toString();

            // Attempting to group an element inside another element, need to check if
            // the drop is being made on a group
            if (row == -1 && col == -1)
            {
                const EMotionFX::Parameter* parameter = m_animGraph->FindParameterByName(dropTopParamName.toUtf8().data());
                if (azrtti_typeid(parameter) == azrtti_typeid<EMotionFX::GroupParameter>())
                {
                    QTreeWidget::dropEvent(event);
                    // row will be -1 if a parameter is dragged on a group, we need
                    // to place the parameter as the last child of the group
                    emit ParameterMoved(item->childCount() - 1, dragParamName.toUtf8().data(), dropTopParamName);
                    return;
                }
            }
            // Placing at col, row in as a child of topIndex, this is always allowed
            else
            {
                const QString& dragParentName = m_draggedParentParam ? m_draggedParentParam->data(0, Qt::UserRole).toString() : QStringLiteral("");
                QTreeWidget::dropEvent(event);
                emit ParameterMoved(row - ((dragParentName == dropTopParamName) ? 1 : 0), dragParamName.toUtf8().data(), dropTopParamName);
                // emit ParameterMoved(row, dragParamName.toUtf8().data(), dropTopParamName);
                return;
            }
        }
    }

} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/moc_ParameterWindow.cpp>
