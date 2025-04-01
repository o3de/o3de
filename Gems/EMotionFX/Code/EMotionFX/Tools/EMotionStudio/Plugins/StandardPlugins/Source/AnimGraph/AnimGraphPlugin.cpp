/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AnimGraphPlugin.h"
#include "BlendGraphWidget.h"
#include "NodeGraph.h"
#include "NodePaletteWidget.h"
#include "BlendGraphViewWidget.h"
#include "GraphNode.h"
#include "NavigateWidget.h"
#include "AttributesWindow.h"
#include "BlendTreeVisualNode.h"
#include "StateGraphNode.h"
#include "ParameterWindow.h"
#include "GraphNodeFactory.h"
#include <QDockWidget>
#include <QMainWindow>
#include <QTreeWidgetItem>
#include <QScrollArea>
#include <QFileDialog>
#include <QApplication>
#include <QMessageBox>
#include <QSettings>
#include <QMenu>
#include <MCore/Source/LogManager.h>
#include <MCore/Source/ReflectionSerializer.h>
#include <MCore/Source/StringIdPool.h>
#include <EMotionFX/Source/BlendTreeParameterNode.h>
#include <Editor/AnimGraphEditorBus.h>
#include <Editor/InspectorBus.h>
#include <Editor/SaveDirtyFilesCallbacks.h>
#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionStudio/EMStudioSDK/Source/FileManager.h>
#include <EMotionStudio/EMStudioSDK/Source/MainWindow.h>
#include <EMotionStudio/EMStudioSDK/Source/NotificationWindow.h>
#include <EMotionStudio/EMStudioSDK/Source/SaveChangedFilesManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphActionManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphModel.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphWidget.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/NavigationHistory.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/ParameterEditor/ParameterEditorFactory.h>
#include <MysticQt/Source/DialogStack.h>
#include <MysticQt/Source/KeyboardShortcutManager.h>

#include "../TimeView/TimeViewPlugin.h"
#include "../TimeView/TimeInfoWidget.h"

#include <QMessageBox>

#include <EMotionFX/Source/Allocators.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/AnimGraphObjectFactory.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphAttributeTypes.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphObjectFactory.h>
#include <EMotionFX/Source/AnimGraphStateTransition.h>
#include <EMotionFX/Source/AnimGraphTransitionCondition.h>
#include <EMotionFX/Source/BlendTreeConnection.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>

#include <EMotionFX/CommandSystem/Source/AnimGraphConnectionCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphParameterCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphNodeCommands.h>

#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/FileIO.h>

#include <AzQtComponents/Components/FancyDocking.h>

#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>


namespace EMStudio
{
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphEventHandler, EMotionFX::EventHandlerAllocator)

    AnimGraphPlugin::AnimGraphPlugin()
        : EMStudio::DockWidgetPlugin()
        , m_eventHandler(this)
    {
        m_graphWidget                    = nullptr;
        m_navigateWidget                 = nullptr;
        m_paletteWidget                  = nullptr;
        m_nodePaletteDock                = nullptr;
        m_parameterDock                  = nullptr;
        m_parameterWindow                = nullptr;

        m_attributesWindow               = nullptr;
        m_activeAnimGraph                = nullptr;
        m_animGraphObjectFactory        = nullptr;
        m_graphNodeFactory               = nullptr;
        m_viewWidget                     = nullptr;
        m_navigationHistory             = nullptr;

        m_displayFlags                   = 0;
        m_disableRendering               = false;
        m_lastPlayTime                   = -1;
        m_totalTime                      = FLT_MAX;
        m_animGraphModel                = nullptr;
        m_actionManager                 = nullptr;
    }

    AnimGraphPlugin::~AnimGraphPlugin()
    {
        // destroy the event handler
        EMotionFX::GetEventManager().RemoveEventHandler(&m_eventHandler);

        // unregister the command callbacks and get rid of the memory
        for (MCore::Command::Callback* callback : m_commandCallbacks)
        {
            GetCommandManager()->RemoveCommandCallback(callback, true);
        }

        delete m_animGraphObjectFactory;

        // delete the graph node factory
        delete m_graphNodeFactory;

        // remove the attribute dock widget
        if (m_parameterDock)
        {
            EMStudio::GetMainWindow()->removeDockWidget(m_parameterDock);
            delete m_parameterDock;
        }

        if (m_attributesWindow)
        {
            delete m_attributesWindow;
            m_attributesWindow = nullptr;
        }

        // remove the blend node palette
        if (m_nodePaletteDock)
        {
            EMStudio::GetMainWindow()->removeDockWidget(m_nodePaletteDock);
            delete m_nodePaletteDock;
        }

        if (m_navigationHistory)
        {
            delete m_navigationHistory;
        }
        if (m_actionManager)
        {
            delete m_actionManager;
        }
        if (m_animGraphModel)
        {
            delete m_animGraphModel;
        }
    }

    // get the name
    const char* AnimGraphPlugin::GetName() const
    {
        return "Anim Graph";
    }

    // get the plugin type id
    uint32 AnimGraphPlugin::GetClassID() const
    {
        return AnimGraphPlugin::CLASS_ID;
    }

    void AnimGraphPlugin::AddWindowMenuEntries(QMenu* parent)
    {
        // Only create menu items if this plugin has been initialized
        // During startup, plugins can be constructed more than once, so don't add connections for those items
        if (GetParameterDock() != nullptr)
        {
            m_dockWindowActions[WINDOWS_PARAMETERWINDOW] = parent->addAction("Parameter Window");
            m_dockWindowActions[WINDOWS_PARAMETERWINDOW]->setCheckable(true);
            m_dockWindowActions[WINDOWS_PALETTEWINDOW] = parent->addAction("Node Palette");
            m_dockWindowActions[WINDOWS_PALETTEWINDOW]->setCheckable(true);

            connect(m_dockWindowActions[WINDOWS_PARAMETERWINDOW], &QAction::triggered, this, [this](bool checked) {
                UpdateWindowVisibility(WINDOWS_PARAMETERWINDOW, checked);
            });
            connect(m_dockWindowActions[WINDOWS_PALETTEWINDOW], &QAction::triggered, this, [this](bool checked) {
                UpdateWindowVisibility(WINDOWS_PALETTEWINDOW, checked);
            });

            // Keep our action checked state in sync by updating whenever we are about to show the menu,
            // since the user could've switched the active tab/closed tabs
            UpdateWindowActionsCheckState();
            QObject::connect(parent, &QMenu::aboutToShow, this, &AnimGraphPlugin::UpdateWindowActionsCheckState);
        }
    }

    void AnimGraphPlugin::UpdateWindowVisibility(EDockWindowOptionFlag option, bool checked)
    {
        QDockWidget* dockWidget = nullptr;
        switch (option)
        {
        case WINDOWS_PARAMETERWINDOW:
            dockWidget = GetParameterDock();
            break;
        case WINDOWS_PALETTEWINDOW:
            dockWidget = GetNodePaletteDock();
            break;
        }

        if (dockWidget)
        {
            if (checked)
            {
                // If the dock widgets wasn't visible and it wasn't tabbed, then it had been closed
                // so we need to restore its layout state
                if (!AzQtComponents::DockTabWidget::IsTabbed(dockWidget))
                {
                    GetMainWindow()->GetFancyDockingManager()->restoreDockWidget(dockWidget);
                }

                // If it's in a tab (or was restored to being in a tab), then set it as the new active tab
                AzQtComponents::DockTabWidget* tabWidget = AzQtComponents::DockTabWidget::ParentTabWidget(dockWidget);
                if (tabWidget)
                {
                    int index = tabWidget->indexOf(dockWidget);
                    tabWidget->setCurrentIndex(index);
                }
                // Otherwise just show the widget
                else
                {
                    dockWidget->show();
                }
            }
            else
            {
                dockWidget->close();
            }
        }
    }

    void AnimGraphPlugin::UpdateWindowActionsCheckState()
    {
        SetOptionFlag(WINDOWS_PARAMETERWINDOW, GetParameterDock()->isVisible());
        SetOptionFlag(WINDOWS_PALETTEWINDOW, GetNodePaletteDock()->isVisible());

    }

    void AnimGraphPlugin::SetOptionFlag(EDockWindowOptionFlag option, bool isEnabled)
    {
        if (m_dockWindowActions[option])
        {
            m_dockWindowActions[option]->setChecked(isEnabled);
        }
    }

    void AnimGraphPlugin::SetOptionEnabled(EDockWindowOptionFlag option, bool isEnabled)
    {
        if (m_dockWindowActions[option])
        {
            m_dockWindowActions[option]->setEnabled(isEnabled);
        }
    }

    void AnimGraphPlugin::SetActionFilter(const AnimGraphActionFilter& actionFilter)
    {
        m_actionFilter = actionFilter;

        emit ActionFilterChanged();
    }

    const AnimGraphActionFilter& AnimGraphPlugin::GetActionFilter() const
    {
        return m_actionFilter;
    }

    TimeViewPlugin* AnimGraphPlugin::FindTimeViewPlugin() const
    {
        EMStudioPlugin* timeViewBasePlugin = EMStudio::GetPluginManager()->FindActivePlugin(TimeViewPlugin::CLASS_ID);
        if (timeViewBasePlugin)
        {
            return static_cast<TimeViewPlugin*>(timeViewBasePlugin);
        }

        return nullptr;
    }

    void AnimGraphPlugin::RegisterPerFrameCallback(AnimGraphPerFrameCallback* callback)
    {
        if (AZStd::find(m_perFrameCallbacks.begin(), m_perFrameCallbacks.end(), callback) == m_perFrameCallbacks.end())
        {
            m_perFrameCallbacks.push_back(callback);
        }
    }

    void AnimGraphPlugin::UnregisterPerFrameCallback(AnimGraphPerFrameCallback* callback)
    {
        auto it = AZStd::find(m_perFrameCallbacks.begin(), m_perFrameCallbacks.end(), callback);
        if (it != m_perFrameCallbacks.end())
        {
            m_perFrameCallbacks.erase(it);
        }
    }

    void AnimGraphPlugin::OnMainWindowClosed()
    {
        // If the recorder is on, turn it off
        if (EMotionFX::GetEMotionFX().GetRecorder()
            && EMotionFX::GetEMotionFX().GetRecorder()->GetIsRecording())
        {
            EMotionFX::GetEMotionFX().GetRecorder()->Clear();
        }

        EMStudio::DockWidgetPlugin::OnMainWindowClosed();
    }

    void AnimGraphPlugin::Reflect(AZ::ReflectContext* context)
    {
        AnimGraphOptions::Reflect(context);
        BlendGraphMimeEvent::Reflect(context);
        ParameterEditorFactory::ReflectParameterEditorTypes(context);
    }

    // init after the parent dock window has been created
    bool AnimGraphPlugin::Init()
    {
        m_animGraphModel = new AnimGraphModel();

        m_actionManager = new AnimGraphActionManager(this);
        m_navigationHistory = new NavigationHistory(GetAnimGraphModel());

        // create the command callbacks
        m_commandCallbacks.emplace_back(new CommandActivateAnimGraphCallback(true));
        GetCommandManager()->RegisterCommandCallback("ActivateAnimGraph", m_commandCallbacks.back());

        m_commandCallbacks.emplace_back(new CommandSelectCallback(true));
        GetCommandManager()->RegisterCommandCallback("Select", m_commandCallbacks.back());

        m_commandCallbacks.emplace_back(new CommandUnselectCallback(true));
        GetCommandManager()->RegisterCommandCallback("Unselect", m_commandCallbacks.back());

        m_commandCallbacks.emplace_back(new CommandClearSelectionCallback(true));
        GetCommandManager()->RegisterCommandCallback("ClearSelection", m_commandCallbacks.back());

        m_commandCallbacks.emplace_back(new CommandRecorderClearCallback(false));
        GetCommandManager()->RegisterCommandCallback("RecorderClear", m_commandCallbacks.back());

        m_commandCallbacks.emplace_back(new CommandPlayMotionCallback(false));
        GetCommandManager()->RegisterCommandCallback("PlayMotion", m_commandCallbacks.back());

        m_animGraphObjectFactory = aznew EMotionFX::AnimGraphObjectFactory();

        // create the graph node factory
        m_graphNodeFactory = new GraphNodeFactory();

        // create the corresponding widget that holds the menu and the toolbar
        m_viewWidget = new BlendGraphViewWidget(this, m_dock);
        m_dock->setWidget(m_viewWidget);

        // create the graph widget
        m_graphWidget = new BlendGraphWidget(this, m_viewWidget);

        // get the main window
        QMainWindow* mainWindow = GetMainWindow();

        // Create the attribute window used as content widget for the inspector.
        m_attributesWindow = new AttributesWindow(this);
        m_attributesWindow->hide();

        QDockWidget::DockWidgetFeatures features;

        // create the node palette dock.
        // By default, it's hidden in AnimGraph.layout. Users should mostly use
        // the context menu to add nodes, but we let them show the palette dock
        // if needed
        m_nodePaletteDock = new AzQtComponents::StyledDockWidget("Node Palette", mainWindow);
        mainWindow->addDockWidget(Qt::RightDockWidgetArea, m_nodePaletteDock);
        features = QDockWidget::NoDockWidgetFeatures;
        //features |= QDockWidget::DockWidgetClosable;
        features |= QDockWidget::DockWidgetFloatable;
        features |= QDockWidget::DockWidgetMovable;
        m_nodePaletteDock->setFeatures(features);
        m_nodePaletteDock->setObjectName("AnimGraphPlugin::m_paletteDock");
        m_paletteWidget = new NodePaletteWidget(this);
        m_nodePaletteDock->setWidget(m_paletteWidget);

        // create the parameter dock
        QScrollArea* scrollArea = new QScrollArea();
        m_parameterDock = new AzQtComponents::StyledDockWidget("Parameters", mainWindow);
        mainWindow->addDockWidget(Qt::RightDockWidgetArea, m_parameterDock);
        features = QDockWidget::NoDockWidgetFeatures;
        //features |= QDockWidget::DockWidgetClosable;
        features |= QDockWidget::DockWidgetFloatable;
        features |= QDockWidget::DockWidgetMovable;
        m_parameterDock->setFeatures(features);
        m_parameterDock->setObjectName("AnimGraphPlugin::m_parameterDock");
        m_parameterWindow = new ParameterWindow(this);
        m_parameterDock->setWidget(scrollArea);
        scrollArea->setWidget(m_parameterWindow);
        scrollArea->setWidgetResizable(true);

        // Create Navigation Widget (embedded into BlendGraphViewWidget)
        m_navigateWidget = new NavigateWidget(this);

        // init the display flags
        m_displayFlags = 0;

        // init the view widget
        // it must be init after navigate widget is created because actions are linked to it
        m_viewWidget->Init(m_graphWidget);

        // load options
        LoadOptions();

        // construct the event handler
        EMotionFX::GetEventManager().AddEventHandler(&m_eventHandler);

        // connect to the timeline recorder data
        TimeViewPlugin* timeViewPlugin = FindTimeViewPlugin();
        if (timeViewPlugin)
        {
            connect(timeViewPlugin, &TimeViewPlugin::DoubleClickedRecorderNodeHistoryItem, this, &AnimGraphPlugin::OnDoubleClickedRecorderNodeHistoryItem);
            connect(timeViewPlugin, &TimeViewPlugin::ClickedRecorderNodeHistoryItem, this, &AnimGraphPlugin::OnClickedRecorderNodeHistoryItem);
            // detect changes in the recorder
            connect(timeViewPlugin, &TimeViewPlugin::RecorderStateChanged, m_parameterWindow, &ParameterWindow::OnRecorderStateChanged);
        }

        EMotionFX::AnimGraph* firstSelectedAnimGraph = CommandSystem::GetCommandManager()->GetCurrentSelection().GetFirstAnimGraph();
        SetActiveAnimGraph(firstSelectedAnimGraph);
        return true;
    }


    // load the options
    void AnimGraphPlugin::LoadOptions()
    {
        QSettings settings(AZStd::string(GetManager()->GetAppDataFolder() + "EMStudioRenderOptions.cfg").c_str(), QSettings::IniFormat, this);
        m_options = AnimGraphOptions::Load(&settings);
    }

    // save the options
    void AnimGraphPlugin::SaveOptions()
    {
        QSettings settings(AZStd::string(GetManager()->GetAppDataFolder() + "EMStudioRenderOptions.cfg").c_str(), QSettings::IniFormat, this);
        m_options.Save(&settings);
    }


    // triggered after loading a new layout
    void AnimGraphPlugin::OnAfterLoadLayout()
    {
        // fit graph on screen
        if (m_graphWidget->GetActiveGraph())
        {
            m_graphWidget->GetActiveGraph()->FitGraphOnScreen(m_graphWidget->geometry().width(), m_graphWidget->geometry().height(), m_graphWidget->GetMousePos(), false);
        }

        // connect to the timeline recorder data
        TimeViewPlugin* timeViewPlugin = FindTimeViewPlugin();
        if (timeViewPlugin)
        {
            connect(timeViewPlugin, &TimeViewPlugin::DoubleClickedRecorderNodeHistoryItem, this, &AnimGraphPlugin::OnDoubleClickedRecorderNodeHistoryItem);
            connect(timeViewPlugin, &TimeViewPlugin::ClickedRecorderNodeHistoryItem, this, &AnimGraphPlugin::OnClickedRecorderNodeHistoryItem);
        }

        SetOptionFlag(WINDOWS_PARAMETERWINDOW, GetParameterDock()->isVisible());
        SetOptionFlag(WINDOWS_PALETTEWINDOW, GetNodePaletteDock()->isVisible());
    }


    // init for a given anim graph
    void AnimGraphPlugin::InitForAnimGraph(EMotionFX::AnimGraph* setup)
    {
        AZ_UNUSED(setup);
        m_attributesWindow->Unlock();
        m_attributesWindow->Init(QModelIndex(), true); // Force update
        EMStudio::InspectorRequestBus::Broadcast(&EMStudio::InspectorRequestBus::Events::Update, m_attributesWindow);

        m_parameterWindow->Reinit();
        m_viewWidget->UpdateAnimGraphOptions();
    }


    // constructor
    AnimGraphEventHandler::AnimGraphEventHandler(AnimGraphPlugin* plugin)
        : EMotionFX::EventHandler()
    {
        m_plugin = plugin;
    }


    bool AnimGraphEventHandler::OnRayIntersectionTest(const AZ::Vector3& start, const AZ::Vector3& end, EMotionFX::IntersectionInfo* outIntersectInfo)
    {
        outIntersectInfo->m_isValid = true;

        AZ::Vector3 pos;
        AZ::Vector3 normal;
        AZ::Vector2 uv(0.0f, 0.0f);
        float baryU;
        float baryV;
        uint32 startIndex;
        bool first = true;
        bool result = false;
        float closestDist = FLT_MAX;

        MCore::Ray ray(start, end);

        const size_t numActorInstances = EMotionFX::GetActorManager().GetNumActorInstances();
        for (size_t i = 0; i < numActorInstances; ++i)
        {
            EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().GetActorInstance(i);

            if (actorInstance->GetIsOwnedByRuntime())
            {
                continue;
            }

            if (actorInstance == outIntersectInfo->m_ignoreActorInstance)
            {
                continue;
            }

            if (actorInstance->IntersectsMesh(0, ray, &pos, &normal, &uv, &baryU, &baryV, &startIndex) == nullptr)
            {
                continue;
            }

            if (first)
            {
                outIntersectInfo->m_position     = pos;
                outIntersectInfo->m_normal       = normal;
                outIntersectInfo->m_uv           = uv;
                outIntersectInfo->m_baryCentricU = baryU;
                outIntersectInfo->m_baryCentricV = baryU;
                closestDist = MCore::SafeLength(start - pos);
            }
            else
            {
                float dist = MCore::SafeLength(start - pos);
                if (dist < closestDist)
                {
                    outIntersectInfo->m_position     = pos;
                    outIntersectInfo->m_normal       = normal;
                    outIntersectInfo->m_uv           = uv;
                    outIntersectInfo->m_baryCentricU = baryU;
                    outIntersectInfo->m_baryCentricV = baryU;
                    closestDist = MCore::SafeLength(start - pos);
                    closestDist = dist;
                }
            }

            first   = false;
            result  = true;
        }

        return result;
    }

    // set the gizmo offsets
    void AnimGraphEventHandler::OnSetVisualManipulatorOffset(
        [[maybe_unused]] EMotionFX::AnimGraphInstance* animGraphInstance,
        [[maybe_unused]] size_t paramIndex,
        [[maybe_unused]] const AZ::Vector3& offset)
    {
    }

    void AnimGraphEventHandler::OnInputPortsChanged(EMotionFX::AnimGraphNode* node, const AZStd::vector<AZStd::string>& newInputPorts, const AZStd::string& memberName, const AZStd::vector<AZStd::string>& memberValue)
    {
        MCore::CommandGroup commandGroup("Adjust node input ports");
        const size_t newInputPortsCount = newInputPorts.size();

        /////////////////////////////////////////////////////////////////////////////////////////////
        // PHASE 1: Remember the incomming connections and build the new ones
        /////////////////////////////////////////////////////////////////////////////////////////////
        AZStd::vector<EMotionFX::BlendTreeConnection*> oldConnections;
        AZStd::vector<AZStd::unique_ptr<EMotionFX::BlendTreeConnection> > newConnections;

        // get the number of incoming connections and iterate through them
        const size_t numConnections = node->GetNumConnections();
        for (size_t c = 0; c < numConnections; ++c)
        {
            // get the connection and check if it is plugged into the node
            EMotionFX::BlendTreeConnection* connection = node->GetConnection(c);

            // Figure out how that new connection will look like. To do so, we need to find the new port index based
            // on the name of the old port
            const uint16 targetPort = connection->GetTargetPort();
            const AZStd::string& targetPortName = node->GetInputPort(targetPort).GetNameString();

            // Now search for that port in newInputPorts and create the new connection
            bool foundConnection = false;
            for (size_t newPort = 0; newPort < newInputPortsCount; ++newPort)
            {
                if (newInputPorts[newPort] == targetPortName)
                {
                    if (targetPort != static_cast<AZ::u16>(newPort))
                    {
                        // Needs rewiring
                        oldConnections.emplace_back(connection);
                        newConnections.emplace_back(AZStd::make_unique<EMotionFX::BlendTreeConnection>(connection->GetSourceNode(), connection->GetSourcePort(), static_cast<AZ::u16>(newPort)));
                    }
                    foundConnection = true;
                    break;
                }
            }
            if (!foundConnection)
            {
                oldConnections.emplace_back(connection);
            }
        }

        /////////////////////////////////////////////////////////////////////////////////////////////
        // PHASE 2: Removing all connections that need rewiring
        /////////////////////////////////////////////////////////////////////////////////////////////
        for (EMotionFX::BlendTreeConnection* oldConnection : oldConnections)
        {
            CommandSystem::DeleteNodeConnection(&commandGroup, node, oldConnection);
        }

        /////////////////////////////////////////////////////////////////////////////////////////////
        // PHASE 3: Set the member value through  command
        /////////////////////////////////////////////////////////////////////////////////////////////
        if (!memberName.empty())
        {
            AZ::Outcome<AZStd::string> result = MCore::ReflectionSerializer::SerializeValue(&memberValue);
            if (result)
            {
                const AZStd::string attributesString = "-" + memberName + " {" + result.GetValue() + "}";
                const AZStd::string commandString = AZStd::string::format("AnimGraphAdjustNode -animGraphID %i -name \"%s\" -attributesString \"%s\"",
                    node->GetAnimGraph()->GetID(),
                    node->GetName(),
                    attributesString.c_str());
                commandGroup.AddCommandString(commandString);
            }
        }

        /////////////////////////////////////////////////////////////////////////////////////////////
        // PHASE 4: Recreate the connections at the new ports
        /////////////////////////////////////////////////////////////////////////////////////////////
        for (const AZStd::unique_ptr<EMotionFX::BlendTreeConnection>& newConnection : newConnections)
        {
            CommandSystem::CreateNodeConnection(&commandGroup, node, newConnection.get());
        }

        // Execute the command group
        AZStd::string commandResult;
        // Typically determine saving history based on if we're already inside an executing cmd, but in this case we also don't want it 
        // in the action history either, because while undo in the action history will undo this command, it doesn't undo the UI changes.
        if (!GetCommandManager()->ExecuteCommandGroup(commandGroup, commandResult, false/*shouldAddToHistory*/))
        {
            if (commandResult.empty() == false)
            {
                MCore::LogError(commandResult.c_str());
            }
        }
    }

    void AnimGraphEventHandler::OnOutputPortsChanged(EMotionFX::AnimGraphNode* node, const AZStd::vector<AZStd::string>& newOutputPorts, const AZStd::string& memberName, const AZStd::vector<AZStd::string>& memberValue)
    {
        MCore::CommandGroup commandGroup("Adjust node output ports");
        EMotionFX::AnimGraphNode* parentNode = node->GetParentNode();
        const size_t newOutputPortsCount = newOutputPorts.size();

        /////////////////////////////////////////////////////////////////////////////////////////////
        // PHASE 1: Remember the outgoing connections and build the new ones
        /////////////////////////////////////////////////////////////////////////////////////////////
        AZStd::vector<AZStd::pair<EMotionFX::BlendTreeConnection*, EMotionFX::AnimGraphNode*> > oldConnections;
        AZStd::vector<AZStd::pair<AZStd::unique_ptr<EMotionFX::BlendTreeConnection>, EMotionFX::AnimGraphNode*> > newConnections;

        // iterate through all nodes in the parent and check if any of these has a connection from our node
        const size_t numNodes = parentNode->GetNumChildNodes();
        for (size_t i = 0; i < numNodes; ++i)
        {
            // get the child node and skip it in case it is the parameter node itself
            EMotionFX::AnimGraphNode* childNode = parentNode->GetChildNode(i);
            if (childNode == node)
            {
                continue;
            }

            // get the number of outgoing connections and iterate through them
            const size_t numConnections = childNode->GetNumConnections();
            for (size_t c = 0; c < numConnections; ++c)
            {
                // get the connection and check if it is plugged into the parameter node
                EMotionFX::BlendTreeConnection* connection = childNode->GetConnection(c);
                if (connection->GetSourceNode() == node)
                {
                    // Figure out how that new connection will look like. To do so, we need to find the new port index based
                    // on the name of the old port
                    const uint16 sourcePort = connection->GetSourcePort();
                    const AZStd::string& sourcePortName = node->GetOutputPort(sourcePort).GetName();

                    // Now search for that port in newOutputPorts and create the new connection
                    bool foundConnection = false;
                    for (size_t newPort = 0; newPort < newOutputPortsCount; ++newPort)
                    {
                        if (newOutputPorts[newPort] == sourcePortName)
                        {
                            if (sourcePort != static_cast<AZ::u16>(newPort))
                            {
                                // Needs rewiring
                                oldConnections.emplace_back(connection, childNode);
                                newConnections.emplace_back(AZStd::make_unique<EMotionFX::BlendTreeConnection>(connection->GetSourceNode(), static_cast<AZ::u16>(newPort), connection->GetTargetPort()), childNode);
                            }
                            foundConnection = true;
                            break;
                        }
                    }
                    if (!foundConnection)
                    {
                        oldConnections.emplace_back(connection, childNode);
                    }
                }
            }
        }

        /////////////////////////////////////////////////////////////////////////////////////////////
        // PHASE 2: Removing all connections that need rewiring
        /////////////////////////////////////////////////////////////////////////////////////////////
        for (const auto& oldConnection : oldConnections)
        {
            CommandSystem::DeleteNodeConnection(&commandGroup, oldConnection.second, oldConnection.first);
        }

        /////////////////////////////////////////////////////////////////////////////////////////////
        // PHASE 3: Set the member value through  command
        /////////////////////////////////////////////////////////////////////////////////////////////
        if (!memberName.empty())
        {
            AZ::Outcome<AZStd::string> result = MCore::ReflectionSerializer::SerializeValue(&memberValue);
            if (result)
            {
                const AZStd::string attributesString = "-" + memberName + " {" + result.GetValue() + "}";
                const AZStd::string commandString = AZStd::string::format("AnimGraphAdjustNode -animGraphID %i -name \"%s\" -attributesString \"%s\"",
                    node->GetAnimGraph()->GetID(),
                    node->GetName(),
                    attributesString.c_str());
                commandGroup.AddCommandString(commandString);
            }
        }

        /////////////////////////////////////////////////////////////////////////////////////////////
        // PHASE 4: Recreate the connections at the new ports
        /////////////////////////////////////////////////////////////////////////////////////////////
        for (const auto& newConnection : newConnections)
        {
            CommandSystem::CreateNodeConnection(&commandGroup, newConnection.second, newConnection.first.get());
        }

        // Execute the command group
        AZStd::string commandResult;
        // Typically determine saving history based on if we're already inside an executing cmd, but in this case we also don't want it 
        // in the action history either, because while undo in the action history will undo this command, it doesn't undo the UI changes.
        if (!GetCommandManager()->ExecuteCommandGroup(commandGroup, commandResult, false/*shouldAddToHistory*/))
        {
            if (commandResult.empty() == false)
            {
                MCore::LogError(commandResult.c_str());
            }
        }
    }


    void AnimGraphEventHandler::OnDeleteAnimGraph(EMotionFX::AnimGraph* animGraph)
    {
        if (m_plugin->GetActiveAnimGraph() == animGraph)
        {
            m_plugin->SetActiveAnimGraph(nullptr);
        }
    }

    void AnimGraphEventHandler::OnDeleteAnimGraphInstance(EMotionFX::AnimGraphInstance* animGraphInstance)
    {
        m_plugin->GetAnimGraphModel().SetAnimGraphInstance(animGraphInstance->GetAnimGraph(), animGraphInstance, nullptr);
    }


    // activate a given anim graph
    void AnimGraphPlugin::SetActiveAnimGraph(EMotionFX::AnimGraph* animGraph)
    {
        if (m_activeAnimGraph != animGraph)
        {
            m_activeAnimGraph = animGraph;
            InitForAnimGraph(animGraph);

            // Focus on the newly actived anim graph if it has already been added to the anim graph model.
            if (animGraph)
            {
                QModelIndex rootModelIndex = m_animGraphModel->FindFirstModelIndex(animGraph->GetRootStateMachine());
                if (rootModelIndex.isValid())
                {
                    m_animGraphModel->Focus(rootModelIndex);
                }
            }
        }
    }


    bool AnimGraphPlugin::IsAnimGraphActive(EMotionFX::AnimGraph* animGraph) const
    {
        const CommandSystem::SelectionList& selectionList = GetCommandManager()->GetCurrentSelection();
        const size_t numActorInstances = selectionList.GetNumSelectedActorInstances();
        for (size_t i = 0; i < numActorInstances; ++i)
        {
            const EMotionFX::ActorInstance* actorInstance = selectionList.GetActorInstance(i);
            const EMotionFX::AnimGraphInstance* animGraphInstance = actorInstance->GetAnimGraphInstance();
            if (animGraphInstance && animGraphInstance->GetAnimGraph() == animGraph)
            {
                return true;
            }
        }

        return false;
    }

    void AnimGraphPlugin::OnFileOpen()
    {
        AZStd::string filename = GetMainWindow()->GetFileManager()->LoadAnimGraphFileDialog(m_viewWidget);
        GetMainWindow()->activateWindow();
        if (filename.empty())
        {
            return;
        }
        FileOpen(filename);
    }

    void AnimGraphPlugin::FileOpen(AZStd::string filename)
    {
        GetMainWindow()->activateWindow();
        // Auto-relocate to asset source folder.

        if (!GetMainWindow()->GetFileManager()->RelocateToAssetSourceFolder(filename))
        {
            const AZStd::string errorString = AZStd::string::format("Unable to find Anim Graph -filename \"%s\"", filename.c_str());
            AZ_Error("EMotionFX", false, errorString.c_str());
            return;
        }

        const CommandSystem::SelectionList& selectionList = GetCommandManager()->GetCurrentSelection();
        const size_t numActorInstances = selectionList.GetNumSelectedActorInstances();

        MCore::CommandGroup commandGroup("Load anim graph");
        AZStd::string command;

        command = AZStd::string::format("LoadAnimGraph -filename \"%s\"", filename.c_str());
        commandGroup.AddCommandString(command);

        // activate it too
        // a command group is needed if actor instances are selected to activate the anim graph
        if (numActorInstances > 0)
        {
            // get the correct motion set
            // nullptr can only be <no motion set> because it's the first anim graph so no one is activated
            // if no motion set selected but one is possible, use the first possible
            // if no motion set selected and no one created, use no motion set
            // if one already selected, use the already selected
            uint32 motionSetId = MCORE_INVALIDINDEX32;
            EMotionFX::MotionSet* motionSet = nullptr;
            EMotionFX::AnimGraphEditorRequestBus::BroadcastResult(motionSet, &EMotionFX::AnimGraphEditorRequests::GetSelectedMotionSet);
            if (motionSet)
            {
                motionSetId = motionSet->GetID();
            }
            else
            {
                const size_t numMotionSets = EMotionFX::GetMotionManager().GetNumMotionSets();
                if (numMotionSets > 0)
                {
                    for (size_t i = 0; i < numMotionSets; ++i)
                    {
                        EMotionFX::MotionSet* candidate = EMotionFX::GetMotionManager().GetMotionSet(i);
                        if (candidate->GetIsOwnedByRuntime())
                        {
                            continue;
                        }

                        motionSet = candidate;
                        motionSetId = motionSet->GetID();
                        break;
                    }
                }
            }

            if (motionSet)
            {
                for (size_t i = 0; i < numActorInstances; ++i)
                {
                    EMotionFX::ActorInstance* actorInstance = selectionList.GetActorInstance(i);
                    if (actorInstance->GetIsOwnedByRuntime())
                    {
                        continue;
                    }

                    command = AZStd::string::format("ActivateAnimGraph -actorInstanceID %d -animGraphID %%LASTRESULT%% -motionSetID %d", actorInstance->GetID(), motionSetId);
                    commandGroup.AddCommandString(command);
                }
            }
        }

        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }

        GetCommandManager()->ClearHistory();
    }

    void AnimGraphPlugin::OnFileSave()
    {
        EMotionFX::AnimGraph* animGraph = m_animGraphModel->GetFocusedAnimGraph();
        if (!animGraph)
        {
            return;
        }

        const size_t animGraphIndex = EMotionFX::GetAnimGraphManager().FindAnimGraphIndex(animGraph);
        assert(animGraphIndex != InvalidIndex);

        const AZStd::string filename = animGraph->GetFileName();
        if (filename.empty())
        {
            OnFileSaveAs();
        }
        else
        {
            GetMainWindow()->GetFileManager()->SaveAnimGraph(filename.c_str(), animGraphIndex);
        }
    }

    void AnimGraphPlugin::OnFileSaveAs()
    {
        EMotionFX::AnimGraph* animGraph = m_animGraphModel->GetFocusedAnimGraph();
        if (!animGraph)
        {
            return;
        }

        const EMotionFX::AnimGraph* focusedAnimGraph = m_animGraphModel->GetFocusedAnimGraph();
        GetMainWindow()->GetFileManager()->SaveAnimGraphAs(m_viewWidget, animGraph, focusedAnimGraph);
    }


    // timer event
    void AnimGraphPlugin::ProcessFrame(float timePassedInSeconds)
    {
        if (GetManager()->GetAvoidRendering() || !m_graphWidget || m_graphWidget->visibleRegion().isEmpty())
        {
            return;
        }

        m_totalTime += timePassedInSeconds;

        for (AnimGraphPerFrameCallback* callback : m_perFrameCallbacks)
        {
            callback->ProcessFrame(timePassedInSeconds);
        }

        bool redraw = false;
    #ifdef MCORE_DEBUG
        if (m_totalTime > 1.0f / 30.0f)
    #else
        if (m_totalTime > 1.0f / 60.0f)
    #endif
        {
            redraw = true;
            m_totalTime = 0.0f;
        }

        if (EMotionFX::GetRecorder().GetIsInPlayMode())
        {
            if (MCore::Compare<float>::CheckIfIsClose(EMotionFX::GetRecorder().GetCurrentPlayTime(), m_lastPlayTime, 0.001f) == false)
            {
                m_parameterWindow->UpdateParameterValues();
                m_lastPlayTime = EMotionFX::GetRecorder().GetCurrentPlayTime();
            }
        }

        m_graphWidget->ProcessFrame(redraw);
    }

    int AnimGraphPlugin::OnSaveDirtyAnimGraphs()
    {
        return GetMainWindow()->GetDirtyFileManager()->SaveDirtyFiles(SaveDirtyAnimGraphFilesCallback::TYPE_ID);
    }


    // double clicked a node history item in the timeview plugin
    void AnimGraphPlugin::OnDoubleClickedRecorderNodeHistoryItem(EMotionFX::Recorder::ActorInstanceData* actorInstanceData, EMotionFX::Recorder::NodeHistoryItem* historyItem)
    {
        MCORE_UNUSED(actorInstanceData);

        // try to locate the node based on its unique ID
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(historyItem->m_animGraphId);
        if (animGraph == nullptr)
        {
            QMessageBox::warning(m_dock, "Cannot Find Anim Graph", "The anim graph used by this node cannot be located anymore, did you delete it?", QMessageBox::Ok);
            return;
        }

        EMotionFX::AnimGraphNode* foundNode = animGraph->RecursiveFindNodeById(historyItem->m_nodeId);
        if (foundNode == nullptr)
        {
            QMessageBox::warning(m_dock, "Cannot Find Node", "The anim graph node cannot be found. Did you perhaps delete the node or change animgraph?", QMessageBox::Ok);
            return;
        }

        EMotionFX::AnimGraphNode* nodeToShow = foundNode->GetParentNode();
        if (nodeToShow)
        {
            // show the graph and notify about the selection change
            const QModelIndex modelIndex = m_animGraphModel->FindFirstModelIndex(nodeToShow);
            m_animGraphModel->Focus(modelIndex);
        }
    }


    // clicked a node history item in the timeview plugin
    void AnimGraphPlugin::OnClickedRecorderNodeHistoryItem(EMotionFX::Recorder::ActorInstanceData* actorInstanceData, EMotionFX::Recorder::NodeHistoryItem* historyItem)
    {
        MCORE_UNUSED(actorInstanceData);

        // try to locate the node based on its unique ID
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(historyItem->m_animGraphId);
        if (animGraph == nullptr)
        {
            QMessageBox::warning(m_dock, "Cannot Find Anim Graph", "The anim graph used by this node cannot be located anymore, did you delete it?", QMessageBox::Ok);
            return;
        }

        EMotionFX::AnimGraphNode* foundNode = animGraph->RecursiveFindNodeById(historyItem->m_nodeId);
        if (foundNode == nullptr)
        {
            QMessageBox::warning(m_dock, "Cannot Find Node", "The anim graph node cannot be found. Did you perhaps delete the node or change animgraph?", QMessageBox::Ok);
            return;
        }

        EMotionFX::AnimGraphNode* nodeToShow = foundNode->GetParentNode();
        if (nodeToShow)
        {
            const QModelIndex foundNodeIndex = m_animGraphModel->FindModelIndex(nodeToShow, historyItem->m_animGraphInstance);
            if (foundNodeIndex.isValid())
            {
                m_animGraphModel->Focus(foundNodeIndex);
            }
        }
    }


    bool AnimGraphPlugin::CheckIfCanCreateObject(EMotionFX::AnimGraphObject* parentObject, const EMotionFX::AnimGraphObject* object, EMotionFX::AnimGraphObject::ECategory category) const
    {
        if (!object)
        {
            return false;
        }

        // Are we viewing a state machine right now?
        const bool isStateMachine = parentObject ? (azrtti_typeid(parentObject) == azrtti_typeid<EMotionFX::AnimGraphStateMachine>()) : false;

        EMotionFX::AnimGraphNode* parentNode = nullptr;
        if (parentObject && azdynamic_cast<EMotionFX::AnimGraphNode*>(parentObject))
        {
            parentNode = static_cast<EMotionFX::AnimGraphNode*>(parentObject);
        }

        bool isSubStateMachine = false;
        if (parentNode &&
            (azrtti_typeid(parentNode) == azrtti_typeid<EMotionFX::AnimGraphStateMachine>()) &&
            parentNode != parentNode->GetAnimGraph()->GetRootStateMachine())
        {
            isSubStateMachine = true;
        }

        // Skip the final node as special case.
        if (azrtti_typeid(object) == azrtti_typeid<EMotionFX::BlendTreeFinalNode>())
        {
            return false;
        }

        // Only load icons in the category we want.
        if (object->GetPaletteCategory() != category)
        {
            return false;
        }

        // If we are at the root, we can only create state machines.
        if (!parentNode)
        {
            if (azrtti_typeid(object) != azrtti_typeid<EMotionFX::AnimGraphStateMachine>())
            {
                return false;
            }
        }

        // Ignore objects other than nodes.
        const EMotionFX::AnimGraphNode* curNode = azdynamic_cast<const EMotionFX::AnimGraphNode*>(object);
        if (!curNode)
        {
            return false;
        }

        // If we're editing a state machine, skip nodes that can't act as a state.
        if (isStateMachine && !curNode->GetCanActAsState())
        {
            return false;
        }

        // Skip if we can have only one node of the given type.
        if (curNode->GetCanHaveOnlyOneInsideParent() && parentNode->CheckIfHasChildOfType(azrtti_typeid(curNode)))
        {
            return false;
        }

        // If we are not inside a state machine and the node we check can only be inside a state machine, then we can skip it.
        if (!isStateMachine && curNode->GetCanBeInsideStateMachineOnly())
        {
            return false;
        }

        // Check if this node can only be used within child-state machines and skip in this case.
        if (!isSubStateMachine && curNode->GetCanBeInsideChildStateMachineOnly())
        {
            return false;
        }

        return true;
    }
} // namespace EMStudio
