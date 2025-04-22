/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include required headers
#include "SceneManagerPlugin.h"
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/MorphSetup.h>
#include <EMotionFX/CommandSystem/Source/ActorCommands.h>
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include "../../../../EMStudioSDK/Source/MainWindow.h"
#include "../../../../EMStudioSDK/Source/FileManager.h"
#include <MysticQt/Source/DialogStack.h>
#include <QMessageBox>
#include <QApplication>
#include <QToolTip>
#include <QMenu>


namespace EMStudio
{
    SceneManagerPlugin::SceneManagerPlugin()
        : EMStudio::DockWidgetPlugin()
    {
        m_importActorCallback                = nullptr;
        m_createActorInstanceCallback        = nullptr;
        m_selectCallback                     = nullptr;
        m_unselectCallback                   = nullptr;
        m_clearSelectionCallback             = nullptr;
        m_removeActorCallback                = nullptr;
        m_removeActorInstanceCallback        = nullptr;
        m_saveActorAssetInfoCallback         = nullptr;
        m_scaleActorDataCallback             = nullptr;
        m_actorPropsWindow                   = nullptr;
        m_adjustActorCallback                = nullptr;
        m_actorSetCollisionMeshesCallback    = nullptr;
        m_adjustActorInstanceCallback        = nullptr;
    }

    SceneManagerPlugin::~SceneManagerPlugin()
    {
        // unregister the command callbacks and get rid of the memory
        GetCommandManager()->RemoveCommandCallback(m_importActorCallback, false);
        GetCommandManager()->RemoveCommandCallback(m_createActorInstanceCallback, false);
        GetCommandManager()->RemoveCommandCallback(m_selectCallback, false);
        GetCommandManager()->RemoveCommandCallback(m_unselectCallback, false);
        GetCommandManager()->RemoveCommandCallback(m_clearSelectionCallback, false);
        GetCommandManager()->RemoveCommandCallback(m_removeActorCallback, false);
        GetCommandManager()->RemoveCommandCallback(m_removeActorInstanceCallback, false);
        GetCommandManager()->RemoveCommandCallback(m_saveActorAssetInfoCallback, false);
        GetCommandManager()->RemoveCommandCallback(m_adjustActorCallback, false);
        GetCommandManager()->RemoveCommandCallback(m_actorSetCollisionMeshesCallback, false);
        GetCommandManager()->RemoveCommandCallback(m_adjustActorInstanceCallback, false);
        GetCommandManager()->RemoveCommandCallback(m_scaleActorDataCallback, false);
        delete m_importActorCallback;
        delete m_createActorInstanceCallback;
        delete m_selectCallback;
        delete m_unselectCallback;
        delete m_clearSelectionCallback;
        delete m_removeActorCallback;
        delete m_removeActorInstanceCallback;
        delete m_saveActorAssetInfoCallback;
        delete m_adjustActorCallback;
        delete m_actorSetCollisionMeshesCallback;
        delete m_adjustActorInstanceCallback;
        delete m_scaleActorDataCallback;
    }

    // init after the parent dock window has been created
    bool SceneManagerPlugin::Init()
    {
        // create the dialog stack
        MysticQt::DialogStack* dialogStack = new MysticQt::DialogStack();

        // create and register the command callbacks only (only execute this code once for all plugins)
        m_importActorCallback                = new ImportActorCallback(false);
        m_createActorInstanceCallback        = new CreateActorInstanceCallback(false);
        m_selectCallback                     = new CommandSelectCallback(false);
        m_unselectCallback                   = new CommandUnselectCallback(false);
        m_clearSelectionCallback             = new CommandClearSelectionCallback(false);
        m_removeActorCallback                = new RemoveActorCallback(false);
        m_removeActorInstanceCallback        = new RemoveActorInstanceCallback(false);
        m_saveActorAssetInfoCallback         = new SaveActorAssetInfoCallback(false);
        m_adjustActorCallback                = new CommandAdjustActorCallback(false);
        m_actorSetCollisionMeshesCallback    = new CommandActorSetCollisionMeshesCallback(false);
        m_adjustActorInstanceCallback        = new CommandAdjustActorInstanceCallback(false);
        m_scaleActorDataCallback             = new CommandScaleActorDataCallback(false);

        GetCommandManager()->RegisterCommandCallback("ImportActor", m_importActorCallback);
        GetCommandManager()->RegisterCommandCallback("CreateActorInstance", m_createActorInstanceCallback);
        GetCommandManager()->RegisterCommandCallback("Select", m_selectCallback);
        GetCommandManager()->RegisterCommandCallback("Unselect", m_unselectCallback);
        GetCommandManager()->RegisterCommandCallback("ClearSelection", m_clearSelectionCallback);
        GetCommandManager()->RegisterCommandCallback("RemoveActor", m_removeActorCallback);
        GetCommandManager()->RegisterCommandCallback("RemoveActorInstance", m_removeActorInstanceCallback);
        GetCommandManager()->RegisterCommandCallback("SaveActorAssetInfo", m_saveActorAssetInfoCallback);
        GetCommandManager()->RegisterCommandCallback("AdjustActor", m_adjustActorCallback);
        GetCommandManager()->RegisterCommandCallback("ActorSetCollisionMeshes", m_actorSetCollisionMeshesCallback);
        GetCommandManager()->RegisterCommandCallback("AdjustActorInstance", m_adjustActorInstanceCallback);
        GetCommandManager()->RegisterCommandCallback("ScaleActorData", m_scaleActorDataCallback);

        // create the actors window
        m_actorsWindow = new ActorsWindow(this);

        // add in the dialog stack
        dialogStack->Add(m_actorsWindow, "Actors", false, true, true);

        // create the actor properties window
        m_actorPropsWindow = new ActorPropertiesWindow(m_dock, this);
        m_actorPropsWindow->Init();

        // add the actor properties window to the stack window
        dialogStack->Add(m_actorPropsWindow, "Actor Properties", false, false, true);

        // set dialog stack as main widget of the dock
        m_dock->setWidget(dialogStack);

        // connect
        connect(m_dock, &QDockWidget::visibilityChanged, this, &SceneManagerPlugin::WindowReInit);

        // reinit the dialog
        ReInit();

        return true;
    }


    // reinitialize the window
    void SceneManagerPlugin::ReInit()
    {
        // reinit the actors window
        m_actorsWindow->ReInit();

        // update the interface
        UpdateInterface();
    }


    // function to update the interface
    void SceneManagerPlugin::UpdateInterface()
    {
        // update interface of the actors window
        m_actorsWindow->UpdateInterface();

        // update interface of the actor properties window
        m_actorPropsWindow->UpdateInterface();
    }


    // reinit the window when it gets activated
    void SceneManagerPlugin::WindowReInit(bool visible)
    {
        if (visible)
        {
            ReInit();
        }
    }


    //-----------------------------------------------------------------------------------------
    // command callbacks
    //-----------------------------------------------------------------------------------------
    bool ReInitSceneManagerPlugin()
    {
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(SceneManagerPlugin::CLASS_ID);
        if (plugin == nullptr)
        {
            return false;
        }

        SceneManagerPlugin* sceneManagerWindow = (SceneManagerPlugin*)plugin;

        // is the plugin visible? only update it if it is visible
        if (sceneManagerWindow->GetDockWidget()->visibleRegion().isEmpty() == false)
        {
            sceneManagerWindow->ReInit();
        }

        return true;
    }


    bool UpdateInterfaceSceneManagerPlugin()
    {
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(SceneManagerPlugin::CLASS_ID);
        if (plugin == nullptr)
        {
            return false;
        }

        SceneManagerPlugin* sceneManagerWindow = (SceneManagerPlugin*)plugin;

        // is the plugin visible? only update it if it is visible
        //if (sceneManagerWindow->GetDockWidget()->visibleRegion().isEmpty() == false)
        sceneManagerWindow->UpdateInterface();

        return true;
    }


    // command callbacks
    bool SceneManagerPlugin::ImportActorCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        return ReInitSceneManagerPlugin();
    }


    bool SceneManagerPlugin::ImportActorCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        return ReInitSceneManagerPlugin();
    }


    bool SceneManagerPlugin::SaveActorAssetInfoCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        return ReInitSceneManagerPlugin();
    }


    bool SceneManagerPlugin::SaveActorAssetInfoCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)                { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitSceneManagerPlugin(); }


    bool SceneManagerPlugin::CommandScaleActorDataCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine) { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitSceneManagerPlugin(); }
    bool SceneManagerPlugin::CommandScaleActorDataCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)    { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitSceneManagerPlugin(); }


    bool SceneManagerPlugin::RemoveActorCallback::Execute([[maybe_unused]] MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(commandLine);
        return ReInitSceneManagerPlugin();
    }


    bool SceneManagerPlugin::RemoveActorCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)              { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitSceneManagerPlugin(); }
    bool SceneManagerPlugin::RemoveActorInstanceCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)   { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitSceneManagerPlugin(); }
    bool SceneManagerPlugin::RemoveActorInstanceCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)      { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitSceneManagerPlugin(); }
    bool SceneManagerPlugin::CreateActorInstanceCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)   { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitSceneManagerPlugin(); }
    bool SceneManagerPlugin::CreateActorInstanceCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)      { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitSceneManagerPlugin(); }
    bool SceneManagerPlugin::CommandSelectCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasActorSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return UpdateInterfaceSceneManagerPlugin();
    }
    bool SceneManagerPlugin::CommandSelectCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasActorSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return UpdateInterfaceSceneManagerPlugin();
    }
    bool SceneManagerPlugin::CommandUnselectCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasActorSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return UpdateInterfaceSceneManagerPlugin();
    }
    bool SceneManagerPlugin::CommandUnselectCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        if (CommandSystem::CheckIfHasActorSelectionParameter(commandLine) == false)
        {
            return true;
        }
        return UpdateInterfaceSceneManagerPlugin();
    }
    bool SceneManagerPlugin::CommandClearSelectionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine) { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitSceneManagerPlugin(); }
    bool SceneManagerPlugin::CommandClearSelectionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)    { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitSceneManagerPlugin(); }


    bool SceneManagerPlugin::CommandAdjustActorCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        return ReInitSceneManagerPlugin();
    }


    bool SceneManagerPlugin::CommandAdjustActorCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        return ReInitSceneManagerPlugin();
    }


    bool SceneManagerPlugin::CommandActorSetCollisionMeshesCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        return ReInitSceneManagerPlugin();
    }


    bool SceneManagerPlugin::CommandActorSetCollisionMeshesCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        return ReInitSceneManagerPlugin();
    }


    bool SceneManagerPlugin::CommandAdjustActorInstanceCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)    { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateInterfaceSceneManagerPlugin(); }
    bool SceneManagerPlugin::CommandAdjustActorInstanceCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)       { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return UpdateInterfaceSceneManagerPlugin(); }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/SceneManager/moc_SceneManagerPlugin.cpp>
