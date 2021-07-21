/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
#include "NodeGroupWindow.h"
#include "BlendTreeVisualNode.h"
#include "DebugEventHandler.h"
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

#if AZ_TRAIT_EMOTIONFX_HAS_GAME_CONTROLLER
    #include "GameControllerWindow.h"
#endif

#include <EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionStudio/EMStudioSDK/Source/FileManager.h>
#include <EMotionStudio/EMStudioSDK/Source/MainWindow.h>
#include <EMotionStudio/EMStudioSDK/Source/NotificationWindow.h>
#include <EMotionStudio/EMStudioSDK/Source/SaveChangedFilesManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphActionManager.h>
#include <EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphModel.h>
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
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphEventHandler, EMotionFX::EventHandlerAllocator, 0)


    class SaveDirtyAnimGraphFilesCallback
        : public SaveDirtyFilesCallback
    {
        MCORE_MEMORYOBJECTCATEGORY(SaveDirtyFilesCallback, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK)

    public:
        SaveDirtyAnimGraphFilesCallback()
            : SaveDirtyFilesCallback()                           {}
        ~SaveDirtyAnimGraphFilesCallback()                                                     {}

        enum
        {
            TYPE_ID = 0x00000004
        };
        uint32 GetType() const override                                                     { return TYPE_ID; }
        uint32 GetPriority() const override                                                 { return 1; }
        bool GetIsPostProcessed() const override                                            { return false; }

        void GetDirtyFileNames(AZStd::vector<AZStd::string>* outFileNames, AZStd::vector<ObjectPointer>* outObjects) override
        {
            // get the number of anim graphs and iterate through them
            const uint32 numAnimGraphs = EMotionFX::GetAnimGraphManager().GetNumAnimGraphs();
            for (uint32 i = 0; i < numAnimGraphs; ++i)
            {
                // return in case we found a dirty file
                EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().GetAnimGraph(i);

                if (animGraph->GetIsOwnedByRuntime())
                {
                    continue;
                }

                if (animGraph->GetDirtyFlag())
                {
                    // add the filename to the dirty filenames array
                    outFileNames->push_back(animGraph->GetFileName());

                    // add the link to the actual object
                    ObjectPointer objPointer;
                    objPointer.mAnimGraph = animGraph;
                    outObjects->push_back(objPointer);
                }
            }
        }

        int SaveDirtyFiles(const AZStd::vector<AZStd::string>& filenamesToSave, const AZStd::vector<ObjectPointer>& objects, MCore::CommandGroup* commandGroup) override
        {
            MCORE_UNUSED(filenamesToSave);

            EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(AnimGraphPlugin::CLASS_ID);
            AnimGraphPlugin* animGraphPlugin = (AnimGraphPlugin*)plugin;
            if (plugin == nullptr)
            {
                return DirtyFileManager::FINISHED;
            }

            const size_t numObjects = objects.size();
            for (size_t i = 0; i < numObjects; ++i)
            {
                // get the current object pointer and skip directly if the type check fails
                ObjectPointer objPointer = objects[i];
                if (objPointer.mAnimGraph == nullptr)
                {
                    continue;
                }

                EMotionFX::AnimGraph* animGraph = objPointer.mAnimGraph;
                if (animGraphPlugin->SaveDirtyAnimGraph(animGraph, commandGroup, false) == DirtyFileManager::CANCELED)
                {
                    return DirtyFileManager::CANCELED;
                }
            }

            return DirtyFileManager::FINISHED;
        }

        const char* GetExtension() const override       { return "animgraph"; }
        const char* GetFileType() const override        { return "anim graph"; }
        const AZ::Uuid GetFileRttiType() const override
        {
            return azrtti_typeid<EMotionFX::AnimGraph>();
        }
    };


    // constructor
    AnimGraphPlugin::AnimGraphPlugin()
        : EMStudio::DockWidgetPlugin()
        , mEventHandler(this)
    {
        mGraphWidget                    = nullptr;
        mNavigateWidget                 = nullptr;
        mAttributeDock                  = nullptr;
        mNodeGroupDock                  = nullptr;
        mPaletteWidget                  = nullptr;
        mNodePaletteDock                = nullptr;
        mParameterDock                  = nullptr;
        mParameterWindow                = nullptr;
        mNodeGroupWindow                = nullptr;
        mAttributesWindow               = nullptr;
        mActiveAnimGraph                = nullptr;
        m_animGraphObjectFactory        = nullptr;
        mGraphNodeFactory               = nullptr;
        mViewWidget                     = nullptr;
        mDirtyFilesCallback             = nullptr;
        m_navigationHistory             = nullptr;

        mDisplayFlags                   = 0;
        //  mShowProcessed                  = false;
        mDisableRendering               = false;
        mLastPlayTime                   = -1;
        mTotalTime                      = FLT_MAX;
#if AZ_TRAIT_EMOTIONFX_HAS_GAME_CONTROLLER
        mGameControllerWindow           = nullptr;
        mGameControllerDock             = nullptr;
#endif
        m_animGraphModel                = nullptr;
        m_actionManager                 = nullptr;
    }


    // destructor
    AnimGraphPlugin::~AnimGraphPlugin()
    {
        // destroy the event handler
        EMotionFX::GetEventManager().RemoveEventHandler(&mEventHandler);

        // unregister the command callbacks and get rid of the memory
        for (MCore::Command::Callback* callback : m_commandCallbacks)
        {
            GetCommandManager()->RemoveCommandCallback(callback, true);
        }

        // remove the dirty file manager callback
        GetMainWindow()->GetDirtyFileManager()->RemoveCallback(mDirtyFilesCallback, false);
        delete mDirtyFilesCallback;

        delete m_animGraphObjectFactory;

        // delete the graph node factory
        delete mGraphNodeFactory;

        // remove the attribute dock widget
        if (mParameterDock)
        {
            EMStudio::GetMainWindow()->removeDockWidget(mParameterDock);
            delete mParameterDock;
        }

        // remove the attribute dock widget
        if (mAttributeDock)
        {
            EMStudio::GetMainWindow()->removeDockWidget(mAttributeDock);
            delete mAttributeDock;
        }

        // remove the node group dock widget
        if (mNodeGroupDock)
        {
            EMStudio::GetMainWindow()->removeDockWidget(mNodeGroupDock);
            delete mNodeGroupDock;
        }

        // remove the blend node palette
        if (mNodePaletteDock)
        {
            EMStudio::GetMainWindow()->removeDockWidget(mNodePaletteDock);
            delete mNodePaletteDock;
        }

        // remove the game controller dock
    #if AZ_TRAIT_EMOTIONFX_HAS_GAME_CONTROLLER
        if (mGameControllerDock)
        {
            EMStudio::GetMainWindow()->removeDockWidget(mGameControllerDock);
            delete mGameControllerDock;
        }
    #endif
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


    // get the compile date
    const char* AnimGraphPlugin::GetCompileDate() const
    {
        return MCORE_DATE;
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


    // get the creator name
    const char* AnimGraphPlugin::GetCreatorName() const
    {
        return "O3DE";
    }


    // get the version
    float AnimGraphPlugin::GetVersion() const
    {
        return 1.0f;
    }

    void AnimGraphPlugin::AddWindowMenuEntries(QMenu* parent)
    {
        // Only create menu items if this plugin has been initialized
        // During startup, plugins can be constructed more than once, so don't add connections for those items
        if (GetAttributeDock() != nullptr)
        {
            mDockWindowActions[WINDOWS_PARAMETERWINDOW] = parent->addAction("Parameter Window");
            mDockWindowActions[WINDOWS_PARAMETERWINDOW]->setCheckable(true);
            mDockWindowActions[WINDOWS_ATTRIBUTEWINDOW] = parent->addAction("Attribute Window");
            mDockWindowActions[WINDOWS_ATTRIBUTEWINDOW]->setCheckable(true);
            mDockWindowActions[WINDOWS_NODEGROUPWINDOW] = parent->addAction("Node Group Window");
            mDockWindowActions[WINDOWS_NODEGROUPWINDOW]->setCheckable(true);
            mDockWindowActions[WINDOWS_PALETTEWINDOW] = parent->addAction("Palette Window");
            mDockWindowActions[WINDOWS_PALETTEWINDOW]->setCheckable(true);
#if AZ_TRAIT_EMOTIONFX_HAS_GAME_CONTROLLER
            mDockWindowActions[WINDOWS_GAMECONTROLLERWINDOW] = parent->addAction("Game Controller Window");
            mDockWindowActions[WINDOWS_GAMECONTROLLERWINDOW]->setCheckable(true);
#endif
            connect(mDockWindowActions[WINDOWS_PARAMETERWINDOW], &QAction::triggered, this, [this](bool checked) {
                UpdateWindowVisibility(WINDOWS_PARAMETERWINDOW, checked);
            });
            connect(mDockWindowActions[WINDOWS_ATTRIBUTEWINDOW], &QAction::triggered, this, [this](bool checked) {
                UpdateWindowVisibility(WINDOWS_ATTRIBUTEWINDOW, checked);
            });
            connect(mDockWindowActions[WINDOWS_NODEGROUPWINDOW], &QAction::triggered, this, [this](bool checked) {
                UpdateWindowVisibility(WINDOWS_NODEGROUPWINDOW, checked);
            });
            connect(mDockWindowActions[WINDOWS_PALETTEWINDOW], &QAction::triggered, this, [this](bool checked) {
                UpdateWindowVisibility(WINDOWS_PALETTEWINDOW, checked);
            });
#if AZ_TRAIT_EMOTIONFX_HAS_GAME_CONTROLLER
            connect(mDockWindowActions[WINDOWS_GAMECONTROLLERWINDOW], &QAction::triggered, this, [this](bool checked) {
                UpdateWindowVisibility(WINDOWS_GAMECONTROLLERWINDOW, checked);
            });
#endif

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
        case WINDOWS_ATTRIBUTEWINDOW:
            dockWidget = GetAttributeDock();
            break;
        case WINDOWS_NODEGROUPWINDOW:
            dockWidget = GetNodeGroupDock();
            break;
        case WINDOWS_PALETTEWINDOW:
            dockWidget = GetNodePaletteDock();
            break;
#if AZ_TRAIT_EMOTIONFX_HAS_GAME_CONTROLLER
        case WINDOWS_GAMECONTROLLERWINDOW:
            dockWidget = GetGameControllerDock();
            break;
#endif
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
        SetOptionFlag(WINDOWS_ATTRIBUTEWINDOW, GetAttributeDock()->isVisible());
        SetOptionFlag(WINDOWS_PALETTEWINDOW, GetNodePaletteDock()->isVisible());
        SetOptionFlag(WINDOWS_NODEGROUPWINDOW, GetNodeGroupDock()->isVisible());
#if AZ_TRAIT_EMOTIONFX_HAS_GAME_CONTROLLER
        SetOptionFlag(WINDOWS_GAMECONTROLLERWINDOW, GetGameControllerDock()->isVisible());
#endif
    }

    void AnimGraphPlugin::SetOptionFlag(EDockWindowOptionFlag option, bool isEnabled)
    {
        const uint32 optionIndex = (uint32)option;
        if (mDockWindowActions[optionIndex])
        {
            mDockWindowActions[optionIndex]->setChecked(isEnabled);
        }
    }

    void AnimGraphPlugin::SetOptionEnabled(EDockWindowOptionFlag option, bool isEnabled)
    {
        const uint32 optionIndex = (uint32)option;
        if (mDockWindowActions[optionIndex])
        {
            mDockWindowActions[optionIndex]->setEnabled(isEnabled);
        }
    }


    // clone the log window
    EMStudioPlugin* AnimGraphPlugin::Clone()
    {
        AnimGraphPlugin* newPlugin = new AnimGraphPlugin();
        return newPlugin;
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
        if (AZStd::find(mPerFrameCallbacks.begin(), mPerFrameCallbacks.end(), callback) == mPerFrameCallbacks.end())
        {
            mPerFrameCallbacks.push_back(callback);
        }
    }

    void AnimGraphPlugin::UnregisterPerFrameCallback(AnimGraphPerFrameCallback* callback)
    {
        auto it = AZStd::find(mPerFrameCallbacks.begin(), mPerFrameCallbacks.end(), callback);
        if (it != mPerFrameCallbacks.end())
        {
            mPerFrameCallbacks.erase(it);
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
        mGraphNodeFactory = new GraphNodeFactory();

        // create the corresponding widget that holds the menu and the toolbar
        mViewWidget = new BlendGraphViewWidget(this, mDock);
        mDock->setWidget(mViewWidget);
        //mDock->setWidget( mGraphWidget ); // old: without menu and toolbar

        // create the graph widget
        mGraphWidget = new BlendGraphWidget(this, mViewWidget);
        //mGraphWidget->resize(1000, 700);
        //mGraphWidget->move(0,50);
        //mGraphWidget->show();

        // get the main window
        QMainWindow* mainWindow = GetMainWindow();

        // create the attribute dock window
        mAttributeDock = new AzQtComponents::StyledDockWidget("Attributes", mainWindow);
        mainWindow->addDockWidget(Qt::RightDockWidgetArea, mAttributeDock);
        QDockWidget::DockWidgetFeatures features = QDockWidget::NoDockWidgetFeatures;
        //features |= QDockWidget::DockWidgetClosable;
        features |= QDockWidget::DockWidgetFloatable;
        features |= QDockWidget::DockWidgetMovable;
        mAttributeDock->setFeatures(features);
        mAttributeDock->setObjectName("AnimGraphPlugin::mAttributeDock");
        mAttributesWindow = new AttributesWindow(this);
        mAttributeDock->setWidget(mAttributesWindow);

        // create the node group dock window
        mNodeGroupDock = new AzQtComponents::StyledDockWidget("Node Groups", mainWindow);
        mainWindow->addDockWidget(Qt::RightDockWidgetArea, mNodeGroupDock);
        features = QDockWidget::NoDockWidgetFeatures;
        //features |= QDockWidget::DockWidgetClosable;
        features |= QDockWidget::DockWidgetFloatable;
        features |= QDockWidget::DockWidgetMovable;
        mNodeGroupDock->setFeatures(features);
        mNodeGroupDock->setObjectName("AnimGraphPlugin::mNodeGroupDock");
        mNodeGroupWindow = new NodeGroupWindow(this);
        mNodeGroupDock->setWidget(mNodeGroupWindow);

        // create the node palette dock
        mNodePaletteDock = new AzQtComponents::StyledDockWidget("Anim Graph Palette", mainWindow);
        mainWindow->addDockWidget(Qt::RightDockWidgetArea, mNodePaletteDock);
        features = QDockWidget::NoDockWidgetFeatures;
        //features |= QDockWidget::DockWidgetClosable;
        features |= QDockWidget::DockWidgetFloatable;
        features |= QDockWidget::DockWidgetMovable;
        mNodePaletteDock->setFeatures(features);
        mNodePaletteDock->setObjectName("AnimGraphPlugin::mPaletteDock");
        mPaletteWidget = new NodePaletteWidget(this);
        mNodePaletteDock->setWidget(mPaletteWidget);

        // create the parameter dock
        QScrollArea* scrollArea = new QScrollArea();
        mParameterDock = new AzQtComponents::StyledDockWidget("Parameters", mainWindow);
        mainWindow->addDockWidget(Qt::RightDockWidgetArea, mParameterDock);
        features = QDockWidget::NoDockWidgetFeatures;
        //features |= QDockWidget::DockWidgetClosable;
        features |= QDockWidget::DockWidgetFloatable;
        features |= QDockWidget::DockWidgetMovable;
        mParameterDock->setFeatures(features);
        mParameterDock->setObjectName("AnimGraphPlugin::mParameterDock");
        mParameterWindow = new ParameterWindow(this);
        mParameterDock->setWidget(scrollArea);
        scrollArea->setWidget(mParameterWindow);
        scrollArea->setWidgetResizable(true);

        // Create Navigation Widget (embedded into BlendGraphViewWidget)
        mNavigateWidget = new NavigateWidget(this);

        // init the display flags
        mDisplayFlags = 0;

        // init the view widget
        // it must be init after navigate widget is created because actions are linked to it
        mViewWidget->Init(mGraphWidget);

    #if AZ_TRAIT_EMOTIONFX_HAS_GAME_CONTROLLER
        // create the game controller dock
        mGameControllerDock = new AzQtComponents::StyledDockWidget("Game Controller", mainWindow);
        mainWindow->addDockWidget(Qt::RightDockWidgetArea, mGameControllerDock);
        features = QDockWidget::NoDockWidgetFeatures;
        //features |= QDockWidget::DockWidgetClosable;
        features |= QDockWidget::DockWidgetFloatable;
        features |= QDockWidget::DockWidgetMovable;
        mGameControllerDock->setFeatures(features);
        mGameControllerDock->setObjectName("AnimGraphPlugin::mGameControllerDock");
        mGameControllerWindow = new GameControllerWindow(this);
        mGameControllerDock->setWidget(mGameControllerWindow);
    #endif

        // load options
        LoadOptions();

        // initialize the dirty files callback
        mDirtyFilesCallback = new SaveDirtyAnimGraphFilesCallback();
        GetMainWindow()->GetDirtyFileManager()->AddCallback(mDirtyFilesCallback);

        // construct the event handler
        EMotionFX::GetEventManager().AddEventHandler(&mEventHandler);

        // connect to the timeline recorder data
        TimeViewPlugin* timeViewPlugin = FindTimeViewPlugin();
        if (timeViewPlugin)
        {
            connect(timeViewPlugin, &TimeViewPlugin::DoubleClickedRecorderNodeHistoryItem, this, &AnimGraphPlugin::OnDoubleClickedRecorderNodeHistoryItem);
            connect(timeViewPlugin, &TimeViewPlugin::ClickedRecorderNodeHistoryItem, this, &AnimGraphPlugin::OnClickedRecorderNodeHistoryItem);
            // detect changes in the recorder
            connect(timeViewPlugin, &TimeViewPlugin::RecorderStateChanged, mParameterWindow, &ParameterWindow::OnRecorderStateChanged);
        }

        EMotionFX::AnimGraph* firstSelectedAnimGraph = CommandSystem::GetCommandManager()->GetCurrentSelection().GetFirstAnimGraph();
        SetActiveAnimGraph(firstSelectedAnimGraph);
        return true;
    }


    // load the options
    void AnimGraphPlugin::LoadOptions()
    {
        QSettings settings(AZStd::string(GetManager()->GetAppDataFolder() + "EMStudioRenderOptions.cfg").c_str(), QSettings::IniFormat, this);
        mOptions = AnimGraphOptions::Load(&settings);
    }

    // save the options
    void AnimGraphPlugin::SaveOptions()
    {
        QSettings settings(AZStd::string(GetManager()->GetAppDataFolder() + "EMStudioRenderOptions.cfg").c_str(), QSettings::IniFormat, this);
        mOptions.Save(&settings);
    }


    // triggered after loading a new layout
    void AnimGraphPlugin::OnAfterLoadLayout()
    {
        // fit graph on screen
        if (mGraphWidget->GetActiveGraph())
        {
            mGraphWidget->GetActiveGraph()->FitGraphOnScreen(mGraphWidget->geometry().width(), mGraphWidget->geometry().height(), mGraphWidget->GetMousePos(), false);
        }

        // connect to the timeline recorder data
        TimeViewPlugin* timeViewPlugin = FindTimeViewPlugin();
        if (timeViewPlugin)
        {
            connect(timeViewPlugin, &TimeViewPlugin::DoubleClickedRecorderNodeHistoryItem, this, &AnimGraphPlugin::OnDoubleClickedRecorderNodeHistoryItem);
            connect(timeViewPlugin, &TimeViewPlugin::ClickedRecorderNodeHistoryItem, this, &AnimGraphPlugin::OnClickedRecorderNodeHistoryItem);
        }

        SetOptionFlag(WINDOWS_PARAMETERWINDOW, GetParameterDock()->isVisible());
        SetOptionFlag(WINDOWS_ATTRIBUTEWINDOW, GetAttributeDock()->isVisible());
        SetOptionFlag(WINDOWS_PALETTEWINDOW, GetNodePaletteDock()->isVisible());
#if AZ_TRAIT_EMOTIONFX_HAS_GAME_CONTROLLER
        SetOptionFlag(WINDOWS_GAMECONTROLLERWINDOW, GetGameControllerDock()->isVisible());
#endif
        SetOptionFlag(WINDOWS_NODEGROUPWINDOW, GetNodeGroupDock()->isVisible());
    }


    // init for a given anim graph
    void AnimGraphPlugin::InitForAnimGraph(EMotionFX::AnimGraph* setup)
    {
        AZ_UNUSED(setup);
        mAttributesWindow->Unlock();
        mAttributesWindow->Init(QModelIndex(), true); // Force update
        mParameterWindow->Reinit();
        mNodeGroupWindow->Init();
        mViewWidget->UpdateAnimGraphOptions();
#if AZ_TRAIT_EMOTIONFX_HAS_GAME_CONTROLLER
        mGameControllerWindow->ReInit();
#endif
    }


    // constructor
    AnimGraphEventHandler::AnimGraphEventHandler(AnimGraphPlugin* plugin)
        : EMotionFX::EventHandler()
    {
        mPlugin = plugin;
    }


    bool AnimGraphEventHandler::OnRayIntersectionTest(const AZ::Vector3& start, const AZ::Vector3& end, EMotionFX::IntersectionInfo* outIntersectInfo)
    {
        outIntersectInfo->mIsValid = true;

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

        const uint32 numActorInstances = EMotionFX::GetActorManager().GetNumActorInstances();
        for (uint32 i = 0; i < numActorInstances; ++i)
        {
            EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().GetActorInstance(i);

            if (actorInstance->GetIsOwnedByRuntime())
            {
                continue;
            }

            if (actorInstance == outIntersectInfo->mIgnoreActorInstance)
            {
                continue;
            }

            if (actorInstance->IntersectsMesh(0, ray, &pos, &normal, &uv, &baryU, &baryV, &startIndex) == nullptr)
            {
                continue;
            }

            if (first)
            {
                outIntersectInfo->mPosition     = pos;
                outIntersectInfo->mNormal       = normal;
                outIntersectInfo->mUV           = uv;
                outIntersectInfo->mBaryCentricU = baryU;
                outIntersectInfo->mBaryCentricV = baryU;
                closestDist = MCore::SafeLength(start - pos);
            }
            else
            {
                float dist = MCore::SafeLength(start - pos);
                if (dist < closestDist)
                {
                    outIntersectInfo->mPosition     = pos;
                    outIntersectInfo->mNormal       = normal;
                    outIntersectInfo->mUV           = uv;
                    outIntersectInfo->mBaryCentricU = baryU;
                    outIntersectInfo->mBaryCentricV = baryU;
                    closestDist = MCore::SafeLength(start - pos);
                    closestDist = dist;
                }
            }

            first   = false;
            result  = true;
        }

        /*
            // collide with ground plane
            MCore::Vector3 groundNormal(0.0f, 0.0f, 0.0f);
            groundNormal[MCore::GetCoordinateSystem().GetUpIndex()] = 1.0f;
            MCore::PlaneEq groundPlane( groundNormal, Vector3(0.0f, 0.0f, 0.0f) );
            bool result = MCore::Ray(start, end).Intersects( groundPlane, &(outIntersectInfo->mPosition) );
            outIntersectInfo->mNormal = groundNormal;
        */
        return result;
    }


    // set the gizmo offsets
    void AnimGraphEventHandler::OnSetVisualManipulatorOffset(EMotionFX::AnimGraphInstance* animGraphInstance, uint32 paramIndex, const AZ::Vector3& offset)
    {
        EMStudioManager* manager = GetManager();

        // get the paremeter name
        const AZStd::string& paramName = animGraphInstance->GetAnimGraph()->FindParameter(paramIndex)->GetName();

        // iterate over all gizmos that are active
        MCore::Array<MCommon::TransformationManipulator*>* gizmos = manager->GetTransformationManipulators();
        const uint32 numGizmos = gizmos->GetLength();
        for (uint32 i = 0; i < numGizmos; ++i)
        {
            MCommon::TransformationManipulator* gizmo = gizmos->GetItem(i);

            // check the gizmo name
            if (paramName == gizmo->GetName())
            {
                gizmo->SetRenderOffset(offset);
                return;
            }
        }
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
        const uint32 numConnections = node->GetNumConnections();
        for (uint32 c = 0; c < numConnections; ++c)
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
        const uint32 numNodes = parentNode->GetNumChildNodes();
        for (uint32 i = 0; i < numNodes; ++i)
        {
            // get the child node and skip it in case it is the parameter node itself
            EMotionFX::AnimGraphNode* childNode = parentNode->GetChildNode(i);
            if (childNode == node)
            {
                continue;
            }

            // get the number of outgoing connections and iterate through them
            const uint32 numConnections = childNode->GetNumConnections();
            for (uint32 c = 0; c < numConnections; ++c)
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
        if (mPlugin->GetActiveAnimGraph() == animGraph)
        {
            mPlugin->SetActiveAnimGraph(nullptr);
        }
    }

    void AnimGraphEventHandler::OnDeleteAnimGraphInstance(EMotionFX::AnimGraphInstance* animGraphInstance)
    {
        mPlugin->GetAnimGraphModel().SetAnimGraphInstance(animGraphInstance->GetAnimGraph(), animGraphInstance, nullptr);
    }


    // activate a given anim graph
    void AnimGraphPlugin::SetActiveAnimGraph(EMotionFX::AnimGraph* animGraph)
    {
        if (mActiveAnimGraph != animGraph)
        {
            mActiveAnimGraph = animGraph;
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
        const uint32 numActorInstances = selectionList.GetNumSelectedActorInstances();
        for (uint32 i = 0; i < numActorInstances; ++i)
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


    void AnimGraphPlugin::SaveAnimGraph(const char* filename, uint32 animGraphIndex, MCore::CommandGroup* commandGroup)
    {
        const AZStd::string command = AZStd::string::format("SaveAnimGraph -index %i -filename \"%s\"", animGraphIndex, filename);

        if (commandGroup == nullptr)
        {
            AZStd::string result;
            if (!EMStudio::GetCommandManager()->ExecuteCommand(command, result))
            {
                GetNotificationWindowManager()->CreateNotificationWindow(NotificationWindow::TYPE_ERROR,
                    AZStd::string::format("AnimGraph <font color=red>failed</font> to save<br/><br/>%s", result.c_str()).c_str());
            }
            else
            {
                GetNotificationWindowManager()->CreateNotificationWindow(NotificationWindow::TYPE_SUCCESS,
                    "AnimGraph <font color=green>successfully</font> saved");
            }
        }
        else
        {
            commandGroup->AddCommandString(command);
        }
    }


    void AnimGraphPlugin::SaveAnimGraph(EMotionFX::AnimGraph* animGraph, MCore::CommandGroup* commandGroup)
    {
        const uint32 animGraphIndex = EMotionFX::GetAnimGraphManager().FindAnimGraphIndex(animGraph);
        if (animGraphIndex == MCORE_INVALIDINDEX32)
        {
            return;
        }

        AZStd::string filename = animGraph->GetFileName();
        if (filename.empty())
        {
            filename = GetMainWindow()->GetFileManager()->SaveAnimGraphFileDialog(GetViewWidget());
            if (filename.empty())
            {
                return;
            }
        }

        SaveAnimGraph(filename.c_str(), animGraphIndex, commandGroup);
    }


    void AnimGraphPlugin::SaveAnimGraphAs(EMotionFX::AnimGraph* animGraph, MCore::CommandGroup* commandGroup)
    {
        const AZStd::string filename = GetMainWindow()->GetFileManager()->SaveAnimGraphFileDialog(mViewWidget);
        if (filename.empty())
        {
            return;
        }

        AZStd::string assetFilename = filename;
        GetMainWindow()->GetFileManager()->RelocateToAssetCacheFolder(assetFilename);
        AZStd::string sourceFilename = filename;
        GetMainWindow()->GetFileManager()->RelocateToAssetSourceFolder(sourceFilename);

        // Are we about to overwrite an already opened anim graph?
        const EMotionFX::AnimGraph* sourceAnimGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByFileName(sourceFilename.c_str(), /*isTool*/ true);
        const EMotionFX::AnimGraph* cacheAnimGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByFileName(assetFilename.c_str(), /*isTool*/ true);
        const EMotionFX::AnimGraph* focusedAnimGraph = m_animGraphModel->GetFocusedAnimGraph();
        if (QFile::exists(sourceFilename.c_str()) &&
            (sourceAnimGraph || cacheAnimGraph) &&
            (sourceAnimGraph != focusedAnimGraph && cacheAnimGraph != focusedAnimGraph))
        {
            QMessageBox::warning(mDock, "Cannot overwrite anim graph", "Anim graph is already opened and cannot be overwritten.", QMessageBox::Ok);
            return;
        }

        const uint32 animGraphIndex = EMotionFX::GetAnimGraphManager().FindAnimGraphIndex(animGraph);
        if (animGraphIndex == MCORE_INVALIDINDEX32)
        {
            MCore::LogError("Cannot save anim graph. Anim graph index invalid.");
            return;
        }

        SaveAnimGraph(filename.c_str(), animGraphIndex, commandGroup);
    }


    void AnimGraphPlugin::OnFileOpen()
    {
        AZStd::string filename = GetMainWindow()->GetFileManager()->LoadAnimGraphFileDialog(mViewWidget);
        GetMainWindow()->activateWindow();
        if (filename.empty())
        {
            return;
        }

        // Auto-relocate to asset source folder.

        if (!GetMainWindow()->GetFileManager()->RelocateToAssetSourceFolder(filename))
        {
            const AZStd::string errorString = AZStd::string::format("Unable to find Anim Graph -filename \"%s\"", filename.c_str());
            AZ_Error("EMotionFX", false, errorString.c_str());
            return;
        }

        const CommandSystem::SelectionList& selectionList = GetCommandManager()->GetCurrentSelection();
        const uint32 numActorInstances = selectionList.GetNumSelectedActorInstances();

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
                const uint32 numMotionSets = EMotionFX::GetMotionManager().GetNumMotionSets();
                if (numMotionSets > 0)
                {
                    for (uint32 i = 0; i < numMotionSets; ++i)
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
                for (uint32 i = 0; i < numActorInstances; ++i)
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

        const uint32 animGraphIndex = EMotionFX::GetAnimGraphManager().FindAnimGraphIndex(animGraph);
        assert(animGraphIndex != MCORE_INVALIDINDEX32);

        const AZStd::string filename = animGraph->GetFileName();
        if (filename.empty())
        {
            OnFileSaveAs();
        }
        else
        {
            SaveAnimGraph(filename.c_str(), animGraphIndex);
        }
    }


    void AnimGraphPlugin::OnFileSaveAs()
    {
        EMotionFX::AnimGraph* animGraph = m_animGraphModel->GetFocusedAnimGraph();
        if (!animGraph)
        {
            return;
        }

        SaveAnimGraphAs(animGraph);
    }


    // timer event
    void AnimGraphPlugin::ProcessFrame(float timePassedInSeconds)
    {
        if (GetManager()->GetAvoidRendering() || !mGraphWidget || mGraphWidget->visibleRegion().isEmpty())
        {
            return;
        }

        mTotalTime += timePassedInSeconds;

        for (AnimGraphPerFrameCallback* callback : mPerFrameCallbacks)
        {
            callback->ProcessFrame(timePassedInSeconds);
        }

        bool redraw = false;
    #ifdef MCORE_DEBUG
        if (mTotalTime > 1.0f / 30.0f)
    #else
        if (mTotalTime > 1.0f / 60.0f)
    #endif
        {
            redraw = true;
            mTotalTime = 0.0f;
        }

        if (EMotionFX::GetRecorder().GetIsInPlayMode())
        {
            if (MCore::Compare<float>::CheckIfIsClose(EMotionFX::GetRecorder().GetCurrentPlayTime(), mLastPlayTime, 0.001f) == false)
            {
                mParameterWindow->UpdateParameterValues();
                mLastPlayTime = EMotionFX::GetRecorder().GetCurrentPlayTime();
            }
        }

        mGraphWidget->ProcessFrame(redraw);
    }


    int AnimGraphPlugin::SaveDirtyAnimGraph(EMotionFX::AnimGraph* animGraph, MCore::CommandGroup* commandGroup, bool askBeforeSaving, bool showCancelButton)
    {
        // make sure the anim graph is valid
        if (animGraph == nullptr)
        {
            return QMessageBox::Discard;
        }

        // only process changed files
        if (animGraph->GetDirtyFlag() == false)
        {
            return DirtyFileManager::NOFILESTOSAVE;
        }

        if (askBeforeSaving)
        {
            EMStudio::GetApp()->setOverrideCursor(QCursor(Qt::ArrowCursor));

            QMessageBox msgBox(GetMainWindow());
            AZStd::string text;

            if (!animGraph->GetFileNameString().empty())
            {
                text = AZStd::string::format("Save changes to '%s'?", animGraph->GetFileName());
            }
            else
            {
                text = "Save changes to untitled anim graph?";
            }

            msgBox.setText(text.c_str());
            msgBox.setWindowTitle("Save Changes");

            if (showCancelButton)
            {
                msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
            }
            else
            {
                msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard);
            }

            msgBox.setDefaultButton(QMessageBox::Save);
            msgBox.setIcon(QMessageBox::Question);

            int messageBoxResult = msgBox.exec();
            switch (messageBoxResult)
            {
            case QMessageBox::Save:
            {
                SaveAnimGraph(animGraph, commandGroup);
                break;
            }
            case QMessageBox::Discard:
            {
                EMStudio::GetApp()->restoreOverrideCursor();
                return DirtyFileManager::FINISHED;
            }
            case QMessageBox::Cancel:
            {
                EMStudio::GetApp()->restoreOverrideCursor();
                return DirtyFileManager::CANCELED;
            }
            }
        }
        else
        {
            // save without asking first
            SaveAnimGraph(animGraph, commandGroup);
        }

        return DirtyFileManager::FINISHED;
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
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(historyItem->mAnimGraphID);
        if (animGraph == nullptr)
        {
            QMessageBox::warning(mDock, "Cannot Find Anim Graph", "The anim graph used by this node cannot be located anymore, did you delete it?", QMessageBox::Ok);
            return;
        }

        EMotionFX::AnimGraphNode* foundNode = animGraph->RecursiveFindNodeById(historyItem->mNodeId);
        if (foundNode == nullptr)
        {
            QMessageBox::warning(mDock, "Cannot Find Node", "The anim graph node cannot be found. Did you perhaps delete the node or change animgraph?", QMessageBox::Ok);
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
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(historyItem->mAnimGraphID);
        if (animGraph == nullptr)
        {
            QMessageBox::warning(mDock, "Cannot Find Anim Graph", "The anim graph used by this node cannot be located anymore, did you delete it?", QMessageBox::Ok);
            return;
        }

        EMotionFX::AnimGraphNode* foundNode = animGraph->RecursiveFindNodeById(historyItem->mNodeId);
        if (foundNode == nullptr)
        {
            QMessageBox::warning(mDock, "Cannot Find Node", "The anim graph node cannot be found. Did you perhaps delete the node or change animgraph?", QMessageBox::Ok);
            return;
        }

        EMotionFX::AnimGraphNode* nodeToShow = foundNode->GetParentNode();
        if (nodeToShow)
        {
            const QModelIndex foundNodeIndex = m_animGraphModel->FindModelIndex(nodeToShow, historyItem->mAnimGraphInstance);
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
