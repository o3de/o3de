/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Tests/UI/UIFixture.h>
#include <Tests/UI/ModalPopupHandler.h>
#include <Tests/Mocks/AtomRenderPlugin.h>
#include <Tests/Mocks/PhysicsSystem.h>
#include <Tests/D6JointLimitConfiguration.h>
#include <Integration/System/SystemCommon.h>

#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionStudio/EMStudioSDK/Source/PluginManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/MotionSetsWindow/MotionSetsWindowPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/ParameterWindow.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/ParameterCreateEditWidget.h>

#include <AzCore/IO/Path/Path.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyRowWidget.hxx>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <AzQtComponents/Components/Titlebar.h>
#include <AzQtComponents/Components/DockBarButton.h>
#include <AzQtComponents/Components/WindowDecorationWrapper.h>
#include <AzQtComponents/Components/StyleManager.h>

#include <GraphCanvas/Widgets/NodePalette/NodePaletteTreeView.h>

#include <Editor/Plugins/ColliderWidgets/SimulatedObjectColliderWidget.h>
#include <Editor/Plugins/SimulatedObject/SimulatedObjectWidget.h>
#include <Editor/Plugins/SimulatedObject/SimulatedJointWidget.h>
#include <Editor/Plugins/SkeletonOutliner/SkeletonOutlinerPlugin.h>



#include <Editor/ReselectingTreeView.h>

#include <QtTest>
#include <QAbstractItemModel>
#include <QApplication>
#include <QWidget>
#include <QToolBar>
#include <QPushButton>

namespace EMotionFX
{
    void MakeQtApplicationBase::SetUp()
    {
        m_uiApp = new QApplication(s_argc, nullptr);

        AzToolsFramework::EditorEvents::Bus::Broadcast(&AzToolsFramework::EditorEvents::NotifyRegisterViews);

        AZ::IO::FixedMaxPath engineRootPath;
        if (auto settingsRegistry = AZ::SettingsRegistry::Get())
        {
            settingsRegistry->Get(engineRootPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
        }
        (new AzQtComponents::StyleManager(m_uiApp))->initialize(m_uiApp, engineRootPath);
    }

    MakeQtApplicationBase::~MakeQtApplicationBase()
    {
        delete m_uiApp;
    }

    void UIFixture::SetupQtAndFixtureBase()
    {
        UIFixtureBase::SetUp();
        MakeQtApplicationBase::SetUp();
        // Set ignore visibilty so that the visibility check can be ignored in plugins
        EMStudio::GetManager()->SetIgnoreVisibility(true);
    }

    void UIFixture::SetupPluginWindows()
    {
        // Plugins have to be created after both the QApplication object and after the SystemComponent
        const EMStudio::PluginManager::PluginVector& registeredPlugins = EMStudio::GetPluginManager()->GetRegisteredPlugins();
        for (EMStudio::EMStudioPlugin* plugin : registeredPlugins)
        {
            EMStudio::GetPluginManager()->CreateWindowOfType(plugin->GetName());
        }
        m_skeletonOutlinerPlugin = EMStudio::GetPluginManager()->FindActivePlugin<EMotionFX::SkeletonOutlinerPlugin>();
        m_simulatedObjectPlugin = EMStudio::GetPluginManager()->FindActivePlugin<EMotionFX::SimulatedObjectWidget>();
        m_animGraphPlugin = EMStudio::GetPluginManager()->FindActivePlugin<EMStudio::AnimGraphPlugin>();
    }

    void UIFixture::ReflectMockedSystems()
    {
        if (ShouldReflectPhysicSystem())
        {
            AZ::SerializeContext* serializeContext = GetSerializeContext();

            Physics::MockPhysicsSystem::Reflect(serializeContext); // Required by Ragdoll plugin to fake PhysX Gem is available
            D6JointLimitConfiguration::Reflect(serializeContext);
        }
    }

    void UIFixture::OnRegisterPlugin()
    {
        EMStudio::PluginManager* pluginManager = EMStudio::EMStudioManager::GetInstance()->GetPluginManager();
        pluginManager->RegisterPlugin(new EMStudio::MockAtomRenderPlugin());
    }

    void UIFixture::SetUp()
    {
        Integration::SystemNotificationBus::Handler::BusConnect();

        using namespace testing;
        SetupQtAndFixtureBase();
        ReflectMockedSystems();
        SetupPluginWindows();

        ON_CALL(m_assetSystemRequestMock, GetFullSourcePathFromRelativeProductPath(_, _))
            .WillByDefault(Return(true));
        m_assetSystemRequestMock.BusConnect();
    }

    void UIFixture::TearDown()
    {
        m_assetSystemRequestMock.BusDisconnect();
        Integration::SystemNotificationBus::Handler::BusDisconnect();
        CloseAllNotificationWindows();

        DeselectAllAnimGraphNodes();

        // Restore visibility
        EMStudio::GetManager()->SetIgnoreVisibility(false);

        UIFixtureBase::TearDown();
    }

    QWidget* UIFixture::FindTopLevelWidget(const QString& objectName)
    {
        //const QWidgetList topLevelWidgets = QApplication::topLevelWidgets(); // TODO: Check why QDialogs are no windows anymore and thus the topLevelWidgets() does not include them.
        const QWidgetList topLevelWidgets = QApplication::allWidgets();
        auto iterator = AZStd::find_if(topLevelWidgets.begin(), topLevelWidgets.end(),
            [objectName](const QWidget* widget)
            {
                return (widget->objectName() == objectName);
            });

        if (iterator != topLevelWidgets.end())
        {
            return *iterator;
        }

        return nullptr;
    }

    QWidget* UIFixture::GetWidgetFromToolbar(const QToolBar* toolbar, const QString &widgetText)
    {
        /*
        Searches a Toolbar for an action whose text exactly matches the widgetText parameter.
        Returns the widget by pointer if found, nullptr otherwise.
        */
        for (QAction* action : toolbar->actions())
        {
            if (action->text() == widgetText)
            {
                return toolbar->widgetForAction(action);
            }
        }
        return nullptr;
    }

    QWidget* UIFixture::GetWidgetFromToolbarWithObjectName(const QToolBar* toolbar, const QString &objectName)
    {
        for (QAction* action : toolbar->actions())
        {
            if (action->objectName() == objectName)
            {
                return toolbar->widgetForAction(action);
            }
        }
        return nullptr;
    }

    QWidget* UIFixture::GetWidgetWithNameFromNamedToolbar(const QWidget* widget, const QString &toolBarName, const QString &objectName)
    {
        auto toolBar = widget->findChild<QToolBar*>(toolBarName);
        if (!toolBar)
        {
            return nullptr;
        }

        return UIFixture::GetWidgetFromToolbarWithObjectName(toolBar, objectName);
    }

    QAction* UIFixture::GetNamedAction(const QWidget* widget, const QString& actionText)
    {
        const QList<QAction*> actions = widget->findChildren<QAction*>();

        for (QAction* action : actions)
        {
            if (action->text() == actionText)
            {
                return action;
            }
        }

        return nullptr;
    }

    QModelIndex UIFixture::GetIndexFromName(const GraphCanvas::NodePaletteTreeView* tree, const QString& name)
    {
        const QAbstractItemModel* model = tree->model();
        const QModelIndexList matches = model->match(model->index(0,0), Qt::DisplayRole, name, 1, Qt::MatchRecursive);

        if (!matches.empty())
        {
            return matches[0];
        }
        return {};
    }

    void UIFixture::ExecuteCommands(std::vector<std::string> commands)
    {
        AZStd::string result;
        for (const auto& commandStr : commands)
        {
            if (commandStr == "UNDO")
            {
                EXPECT_TRUE(CommandSystem::GetCommandManager()->Undo(result)) << "Undo: " << result.c_str();
            }
            else if (commandStr == "REDO")
            {
                EXPECT_TRUE(CommandSystem::GetCommandManager()->Redo(result)) << "Redo: " << result.c_str();
            }
            else
            {
                EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand(commandStr.c_str(), result)) << commandStr.c_str() << ": " << result.c_str();
            }
        }
    }

    bool UIFixture::GetActionFromContextMenu(QAction*& action, const QMenu* contextMenu, const QString& actionName)
    {
        const auto contextMenuActions = contextMenu->actions();
        auto contextAction = AZStd::find_if(contextMenuActions.begin(), contextMenuActions.end(), [actionName](const QAction* action) {
            return action->text() == actionName;
            });
        action = *contextAction;
        return contextAction != contextMenuActions.end();
    }

    void UIFixture::CloseAllPlugins()
    {
        m_skeletonOutlinerPlugin = nullptr;
        const EMStudio::PluginManager::PluginVector plugins = EMStudio::GetPluginManager()->GetActivePlugins();
        for (EMStudio::EMStudioPlugin* plugin : plugins)
        {
            EMStudio::GetPluginManager()->RemoveActivePlugin(plugin);
        }
    }

    void UIFixture::CloseAllNotificationWindows()
    {
        while (EMStudio::GetManager()->GetNotificationWindowManager()->GetNumNotificationWindow() > 0)
        {
            EMStudio::NotificationWindow* window = EMStudio::GetManager()->GetNotificationWindowManager()->GetNotificationWindow(0);
            delete window;
        }
    }

    void UIFixture::DeselectAllAnimGraphNodes()
    {
        // Unselectany selected anim graph nodes.
        EMStudio::AnimGraphPlugin*animGraphPlugin = static_cast<EMStudio::AnimGraphPlugin*>(EMStudio::GetPluginManager()->FindActivePlugin(EMStudio::AnimGraphPlugin::CLASS_ID));
        if (!animGraphPlugin)
        {
            return;
        }

        EMStudio::BlendGraphWidget* graphWidget = animGraphPlugin->GetGraphWidget();
        if (!graphWidget)
        {
            return;
        }

        EMStudio::NodeGraph* nodeGraph = graphWidget->GetActiveGraph();
        if (!nodeGraph)
        {
            return;
        }

        nodeGraph->UnselectAllNodes();
        ASSERT_EQ(nodeGraph->GetSelectedAnimGraphNodes().size(), 0) << "No node is selected";
    }

    void UIFixture::BringUpContextMenu(QObject* widget, const QPoint& pos, const QPoint& globalPos)
    {
        QContextMenuEvent cme(QContextMenuEvent::Mouse, pos, globalPos);
        QSpontaneKeyEvent::setSpontaneous(&cme);
        QApplication::instance()->notify(
            widget,
            &cme
        );
    }

    void UIFixture::BringUpContextMenu(const QTreeView* treeView, const QRect& rect)
    {
        BringUpContextMenu(treeView->viewport(), rect.center(), treeView->viewport()->mapTo(treeView->window(), rect.center()));
    }

     void UIFixture::BringUpContextMenu(const QTreeWidget* treeWidget, const QRect& rect)
    {
        QContextMenuEvent cme(QContextMenuEvent::Mouse, rect.center(), treeWidget->viewport()->mapTo(treeWidget->window(), rect.center()));
        QSpontaneKeyEvent::setSpontaneous(&cme);
        QApplication::instance()->notify(
            treeWidget->viewport(),
            &cme
        );
    }

    void UIFixture::SelectIndexes(const QModelIndexList& indexList, QTreeView* treeView, const int start, const int end)
    {
        QItemSelection selection;
        for (int i = start; i <= end; ++i)
        {
            const QModelIndex index = indexList[i];
            EXPECT_TRUE(index.isValid()) << "Unable to find a model index for the joint of the actor";

            selection.select(index, index);
        }
        treeView->selectionModel()->select(selection, QItemSelectionModel::Select | QItemSelectionModel::Rows);
        treeView->scrollTo(indexList[end]);
    }

    AzToolsFramework::PropertyRowWidget* UIFixture::GetNamedPropertyRowWidgetFromReflectedPropertyEditor(AzToolsFramework::ReflectedPropertyEditor* rpe, const QString& name)
    {
        // Search through the RPE's widgets to find the matching widget
        const AzToolsFramework::ReflectedPropertyEditor::WidgetList& widgets = rpe->GetWidgets();
        const auto foundWidget = AZStd::find_if(widgets.begin(), widgets.end(), [name](const auto& widgetIter)
        {
            return name == widgetIter.second->label();
        });
        return foundWidget != widgets.end() ? foundWidget->second : nullptr;
    }


    void UIFixture::TriggerContextMenuAction(QWidget* widget, const QString& actionname)
    {
        BringUpContextMenu(widget, QPoint(10, 10), widget->mapToGlobal(QPoint(10, 10)));

        QMenu* menu = widget->findChild<QMenu*>();
        ASSERT_TRUE(menu) << "Unable to find context menu.";

        QAction* action = menu->findChild<QAction*>(actionname);
        ASSERT_TRUE(action) << "Unable to find context menu action " << actionname.toUtf8().data();
        action->trigger();

        menu->close();
    }

    void UIFixture::TriggerModalContextMenuAction(QWidget* widget, const QString& actionname)
    {
        ModalPopupHandler modalPopupHandler;

        bool actionComplete = false;

        // Set up an action to be called when the menu is either triggered or a timeout occurs.
        ActionCompletionCallback completionCallback = [&actionComplete, actionname](const QString& menu)
        {
            ASSERT_STREQ(menu.toUtf8().constData(), actionname.toUtf8().constData());

            actionComplete = true;
        };

        modalPopupHandler.ShowContextMenuAndTriggerAction(widget, actionname, 3000, completionCallback);

        // Shouldn't get here without the action being complete, but just to be safe...
        // Cast to void to avoid nodiscard compiler warning.
        static_cast<void>(QTest::qWaitFor([actionComplete]() {
            return actionComplete;
        }, 10000));
    }

    AzQtComponents::WindowDecorationWrapper* UIFixture::GetDecorationWrapperForMainWindow() const
    {
        return static_cast<AzQtComponents::WindowDecorationWrapper*>(EMStudio::GetMainWindow()->parent());
    }

    AzQtComponents::TitleBar* UIFixture::GetTitleBarForMainWindow() const
    {
        return GetDecorationWrapperForMainWindow()->findChild<AzQtComponents::TitleBar*>(QString(), Qt::FindDirectChildrenOnly);
    }

    AzQtComponents::DockBarButton* UIFixture::GetDockBarButtonForMainWindow(AzQtComponents::DockBarButton::WindowDecorationButton buttonType) const
    {
        const QList<AzQtComponents::DockBarButton*>buttons =  GetTitleBarForMainWindow()->findChildren<AzQtComponents::DockBarButton*>();

        for (AzQtComponents::DockBarButton* button : buttons)
        {
            if (button->buttonType() == buttonType)
            {
                return button;
            }
        }

        return nullptr;
    }

    void UIFixture::SelectActor(Actor* actor)
    {
        AZStd::string result;
        AZStd::string cmd;
        cmd = AZStd::string::format("Select actor %d", actor->GetID());
        EXPECT_TRUE(CommandSystem::GetCommandManager()->ExecuteCommand(cmd.c_str(), result)) << result.c_str();
    }

    void UIFixture::SelectActorInstance(ActorInstance* actorInstance)
    {
        SelectActor(actorInstance->GetActor());
    }

    EMStudio::MotionSetsWindowPlugin* UIFixture::GetMotionSetsWindowPlugin()
    {
        return static_cast<EMStudio::MotionSetsWindowPlugin*>(EMStudio::GetPluginManager()->FindActivePlugin(EMStudio::MotionSetsWindowPlugin::CLASS_ID));
    }

    EMStudio::MotionSetManagementWindow* UIFixture::GetMotionSetManagementWindow()
    {
        EMStudio::MotionSetsWindowPlugin* plugin = GetMotionSetsWindowPlugin();
        EXPECT_TRUE(plugin);

        return plugin->GetManagementWindow();
    }

    void UIFixture::CreateAnimGraphParameter(const AZStd::string& name)
    {
        EMStudio::ParameterWindow* parameterWindow = m_animGraphPlugin->GetParameterWindow();
        parameterWindow->OnAddParameter();

        auto paramWidget = qobject_cast<EMStudio::ParameterCreateEditWidget*>(FindTopLevelWidget("ParameterCreateEditWidget"));
        ASSERT_TRUE(paramWidget);

        const AZStd::unique_ptr<EMotionFX::Parameter>& param = paramWidget->GetParameter();
        param->SetName(name);
        const size_t numParams = m_animGraphPlugin->GetActiveAnimGraph()->GetNumParameters();

        QPushButton* createButton = paramWidget->findChild<QPushButton*>("EMFX.ParameterCreateEditWidget.CreateApplyButton");
        QTest::mouseClick(createButton, Qt::LeftButton);

        ASSERT_EQ(m_animGraphPlugin->GetActiveAnimGraph()->GetNumParameters(), numParams + 1);
    }

    SimulatedObjectColliderWidget* UIFixture::GetSimulatedObjectColliderWidget() const
    {
        const EMotionFX::SimulatedObjectWidget* simulatedObjectWidget = static_cast<EMotionFX::SimulatedObjectWidget*>(EMStudio::GetPluginManager()->FindActivePlugin(EMotionFX::SimulatedObjectWidget::CLASS_ID));
        EXPECT_TRUE(simulatedObjectWidget) << "Simulated Object plugin not found!";
        if (!simulatedObjectWidget)
        {
            return nullptr;
        }

        const SimulatedJointWidget* simulatedJointWidget = simulatedObjectWidget->GetSimulatedJointWidget();
        EXPECT_TRUE(simulatedJointWidget) << "SimulatedJointWidget not found.";
        if (!simulatedJointWidget)
        {
            return nullptr;
        }

        SimulatedObjectColliderWidget* simulatedObjectColliderWidget = simulatedJointWidget->findChild<SimulatedObjectColliderWidget*>();
        EXPECT_TRUE(simulatedObjectColliderWidget) << "SimulatedJointWidget not found.";

        return simulatedObjectColliderWidget;
    }
} // namespace EMotionFX
