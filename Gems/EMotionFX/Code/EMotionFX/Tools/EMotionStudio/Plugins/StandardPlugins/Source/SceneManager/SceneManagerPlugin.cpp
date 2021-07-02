/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
    void SaveDirtyActorFilesCallback::GetDirtyFileNames(AZStd::vector<AZStd::string>* outFileNames, AZStd::vector<ObjectPointer>* outObjects)
    {
        const uint32 numLeaderActors = EMotionFX::GetActorManager().GetNumActors();
        for (uint32 i = 0; i < numLeaderActors; ++i)
        {
            EMotionFX::Actor* actor = EMotionFX::GetActorManager().GetActor(i);

            // return in case we found a dirty file
            if (actor->GetDirtyFlag() && actor->GetIsUsedForVisualization() == false)
            {
                // add the filename to the dirty filenames array
                outFileNames->push_back(actor->GetFileName());

                // add the link to the actual object
                ObjectPointer objPointer;
                objPointer.mActor = actor;
                outObjects->push_back(objPointer);
            }
        }
    }


    int SaveDirtyActorFilesCallback::SaveDirtyFiles(const AZStd::vector<AZStd::string>& filenamesToSave, const AZStd::vector<ObjectPointer>& objects, MCore::CommandGroup* commandGroup)
    {
        MCORE_UNUSED(filenamesToSave);

        const size_t numObjects = objects.size();
        for (size_t i = 0; i < numObjects; ++i)
        {
            // get the current object pointer and skip directly if the type check fails
            ObjectPointer objPointer = objects[i];
            if (objPointer.mActor == nullptr)
            {
                continue;
            }

            EMotionFX::Actor* actor = objPointer.mActor;
            if (mPlugin->SaveDirtyActor(actor, commandGroup, false) == DirtyFileManager::CANCELED)
            {
                return DirtyFileManager::CANCELED;
            }
        }

        return DirtyFileManager::FINISHED;
    }


    // constructor
    SceneManagerPlugin::SceneManagerPlugin()
        : EMStudio::DockWidgetPlugin()
    {
        mImportActorCallback                = nullptr;
        mCreateActorInstanceCallback        = nullptr;
        mSelectCallback                     = nullptr;
        mUnselectCallback                   = nullptr;
        mClearSelectionCallback             = nullptr;
        mRemoveActorCallback                = nullptr;
        mRemoveActorInstanceCallback        = nullptr;
        mSaveActorAssetInfoCallback         = nullptr;
        mScaleActorDataCallback             = nullptr;
        mActorPropsWindow                   = nullptr;
        mAdjustActorCallback                = nullptr;
        mActorSetCollisionMeshesCallback    = nullptr;
        mAdjustActorInstanceCallback        = nullptr;
        mDirtyFilesCallback                 = nullptr;
            }


    // save dirty actor
    int SceneManagerPlugin::SaveDirtyActor(EMotionFX::Actor* actor, [[maybe_unused]] MCore::CommandGroup* commandGroup, bool askBeforeSaving, bool showCancelButton)
    {
        // only process changed files
        if (actor->GetDirtyFlag() == false)
        {
            return DirtyFileManager::NOFILESTOSAVE;
        }

        // and files that really represent an actor that we take serious in emstudio :)
        if (actor->GetIsUsedForVisualization())
        {
            return DirtyFileManager::NOFILESTOSAVE;
        }

        if (askBeforeSaving)
        {
            EMStudio::GetApp()->setOverrideCursor(QCursor(Qt::ArrowCursor));

            QMessageBox msgBox(GetMainWindow());
            AZStd::string text;
            AZStd::string filename = actor->GetFileNameString();
            AZStd::string extension;
            AzFramework::StringFunc::Path::GetExtension(filename.c_str(), extension, false /* include dot */);

            if (filename.empty() == false && extension.empty() == false)
            {
                text = AZStd::string::format("Save changes to '%s'?", actor->GetFileName());
            }
            else if (actor->GetNameString().empty() == false)
            {
                text = AZStd::string::format("Save changes to the actor named '%s'?", actor->GetName());
            }
            else
            {
                text = "Save changes to untitled actor?";
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
                GetMainWindow()->GetFileManager()->SaveActor(actor);
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
            GetMainWindow()->GetFileManager()->SaveActor(actor);
        }

        return DirtyFileManager::FINISHED;
    }


    // destructor
    SceneManagerPlugin::~SceneManagerPlugin()
    {
        // unregister the command callbacks and get rid of the memory
        GetCommandManager()->RemoveCommandCallback(mImportActorCallback, false);
        GetCommandManager()->RemoveCommandCallback(mCreateActorInstanceCallback, false);
        GetCommandManager()->RemoveCommandCallback(mSelectCallback, false);
        GetCommandManager()->RemoveCommandCallback(mUnselectCallback, false);
        GetCommandManager()->RemoveCommandCallback(mClearSelectionCallback, false);
        GetCommandManager()->RemoveCommandCallback(mRemoveActorCallback, false);
        GetCommandManager()->RemoveCommandCallback(mRemoveActorInstanceCallback, false);
        GetCommandManager()->RemoveCommandCallback(mSaveActorAssetInfoCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAdjustActorCallback, false);
        GetCommandManager()->RemoveCommandCallback(mActorSetCollisionMeshesCallback, false);
        GetCommandManager()->RemoveCommandCallback(mAdjustActorInstanceCallback, false);
        GetCommandManager()->RemoveCommandCallback(mScaleActorDataCallback, false);
        delete mImportActorCallback;
        delete mCreateActorInstanceCallback;
        delete mSelectCallback;
        delete mUnselectCallback;
        delete mClearSelectionCallback;
        delete mRemoveActorCallback;
        delete mRemoveActorInstanceCallback;
        delete mSaveActorAssetInfoCallback;
        delete mAdjustActorCallback;
        delete mActorSetCollisionMeshesCallback;
        delete mAdjustActorInstanceCallback;
        delete mScaleActorDataCallback;

        GetMainWindow()->GetDirtyFileManager()->RemoveCallback(mDirtyFilesCallback, false);
        delete mDirtyFilesCallback;
    }


    // clone the log window
    EMStudioPlugin* SceneManagerPlugin::Clone()
    {
        return new SceneManagerPlugin();
    }


    // init after the parent dock window has been created
    bool SceneManagerPlugin::Init()
    {
        // create the dialog stack
        MysticQt::DialogStack* dialogStack = new MysticQt::DialogStack();

        // create and register the command callbacks only (only execute this code once for all plugins)
        mImportActorCallback                = new ImportActorCallback(false);
        mCreateActorInstanceCallback        = new CreateActorInstanceCallback(false);
        mSelectCallback                     = new CommandSelectCallback(false);
        mUnselectCallback                   = new CommandUnselectCallback(false);
        mClearSelectionCallback             = new CommandClearSelectionCallback(false);
        mRemoveActorCallback                = new RemoveActorCallback(false);
        mRemoveActorInstanceCallback        = new RemoveActorInstanceCallback(false);
        mSaveActorAssetInfoCallback         = new SaveActorAssetInfoCallback(false);
        mAdjustActorCallback                = new CommandAdjustActorCallback(false);
        mActorSetCollisionMeshesCallback    = new CommandActorSetCollisionMeshesCallback(false);
        mAdjustActorInstanceCallback        = new CommandAdjustActorInstanceCallback(false);
        mScaleActorDataCallback             = new CommandScaleActorDataCallback(false);

        GetCommandManager()->RegisterCommandCallback("ImportActor", mImportActorCallback);
        GetCommandManager()->RegisterCommandCallback("CreateActorInstance", mCreateActorInstanceCallback);
        GetCommandManager()->RegisterCommandCallback("Select", mSelectCallback);
        GetCommandManager()->RegisterCommandCallback("Unselect", mUnselectCallback);
        GetCommandManager()->RegisterCommandCallback("ClearSelection", mClearSelectionCallback);
        GetCommandManager()->RegisterCommandCallback("RemoveActor", mRemoveActorCallback);
        GetCommandManager()->RegisterCommandCallback("RemoveActorInstance", mRemoveActorInstanceCallback);
        GetCommandManager()->RegisterCommandCallback("SaveActorAssetInfo", mSaveActorAssetInfoCallback);
        GetCommandManager()->RegisterCommandCallback("AdjustActor", mAdjustActorCallback);
        GetCommandManager()->RegisterCommandCallback("ActorSetCollisionMeshes", mActorSetCollisionMeshesCallback);
        GetCommandManager()->RegisterCommandCallback("AdjustActorInstance", mAdjustActorInstanceCallback);
        GetCommandManager()->RegisterCommandCallback("ScaleActorData", mScaleActorDataCallback);

        // create the actors window
        mActorsWindow = new ActorsWindow(this);

        // add in the dialog stack
        dialogStack->Add(mActorsWindow, "Actors", false, true, true);

        // create the actor properties window
        mActorPropsWindow = new ActorPropertiesWindow(mDock, this);
        mActorPropsWindow->Init();

        // add the actor properties window to the stack window
        dialogStack->Add(mActorPropsWindow, "Actor Properties", false, false, true);

        // set dialog stack as main widget of the dock
        mDock->setWidget(dialogStack);

        // connect
        connect(mDock, &QDockWidget::visibilityChanged, this, &SceneManagerPlugin::WindowReInit);

        // reinit the dialog
        ReInit();

        // initialize the dirty files callback
        mDirtyFilesCallback = new SaveDirtyActorFilesCallback(this);
        GetMainWindow()->GetDirtyFileManager()->AddCallback(mDirtyFilesCallback);

        return true;
    }


    // reinitialize the window
    void SceneManagerPlugin::ReInit()
    {
        // reinit the actors window
        mActorsWindow->ReInit();

        // update the interface
        UpdateInterface();
    }


    // function to update the interface
    void SceneManagerPlugin::UpdateInterface()
    {
        // update interface of the actors window
        mActorsWindow->UpdateInterface();

        // update interface of the actor properties window
        mActorPropsWindow->UpdateInterface();
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
