/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MotionSetsWindowPlugin.h"
#include <QPushButton>
#include <QLabel>
#include <QWidget>
#include <QHeaderView>
#include <QApplication>
#include <QMenu>
#include <QFileDialog>
#include <QMessageBox>
#include <QContextMenuEvent>
#include <QTableWidget>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/Commands.h>
#include "../../../../EMStudioSDK/Source/EMStudioCore.h"
#include "../../../../EMStudioSDK/Source/MainWindow.h"
#include "../../../../EMStudioSDK/Source/SaveChangedFilesManager.h"
#include "../../../../EMStudioSDK/Source/FileManager.h"
#include <MCore/Source/LogManager.h>
#include <MCore/Source/IDGenerator.h>
#include <Editor/AnimGraphEditorBus.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/MotionSet.h>
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include "../MotionWindow/MotionListWindow.h"

#include <AzCore/Debug/Trace.h>
#include <AzFramework/API/ApplicationAPI.h>

namespace EMStudio
{
    class SaveDirtyMotionSetFilesCallback
        : public SaveDirtyFilesCallback
    {
        MCORE_MEMORYOBJECTCATEGORY(SaveDirtyMotionSetFilesCallback, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS)

    public:
        SaveDirtyMotionSetFilesCallback(MotionSetsWindowPlugin* plugin)
            : SaveDirtyFilesCallback()  { m_plugin = plugin; }
        ~SaveDirtyMotionSetFilesCallback()                                                      {}

        enum
        {
            TYPE_ID = 0x00000002
        };
        uint32 GetType() const override                                         { return TYPE_ID; }
        uint32 GetPriority() const override                                     { return 2; }
        bool GetIsPostProcessed() const override                                { return false; }

        void GetDirtyFileNames(AZStd::vector<AZStd::string>* outFileNames, AZStd::vector<ObjectPointer>* outObjects) override
        {
            const size_t numMotionSets = EMotionFX::GetMotionManager().GetNumMotionSets();
            for (size_t i = 0; i < numMotionSets; ++i)
            {
                EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().GetMotionSet(i);

                if (motionSet->GetIsOwnedByRuntime())
                {
                    continue;
                }

                // only save root motion sets
                if (motionSet->GetParentSet())
                {
                    continue;
                }

                // return in case we found a dirty file
                if (motionSet->GetDirtyFlag())
                {
                    // add the filename to the dirty filenames array
                    outFileNames->push_back(motionSet->GetFilename());

                    // add the link to the actual object
                    ObjectPointer objPointer;
                    objPointer.m_motionSet = motionSet;
                    outObjects->push_back(objPointer);
                }
            }
        }

        int SaveDirtyFiles(const AZStd::vector<AZStd::string>& filenamesToSave, const AZStd::vector<ObjectPointer>& objects, MCore::CommandGroup* commandGroup) override
        {
            MCORE_UNUSED(filenamesToSave);

            const size_t numObjects = objects.size();
            for (size_t i = 0; i < numObjects; ++i)
            {
                // get the current object pointer and skip directly if the type check fails
                ObjectPointer objPointer = objects[i];
                if (objPointer.m_motionSet == nullptr)
                {
                    continue;
                }

                EMotionFX::MotionSet* motionSet = objPointer.m_motionSet;
                if (m_plugin->SaveDirtyMotionSet(motionSet, commandGroup, false) == DirtyFileManager::CANCELED)
                {
                    return DirtyFileManager::CANCELED;
                }
            }

            return DirtyFileManager::FINISHED;
        }

        const char* GetExtension() const override       { return "motionset"; }
        const char* GetFileType() const override        { return "motion set"; }
        const AZ::Uuid GetFileRttiType() const override
        {
            return azrtti_typeid<EMotionFX::MotionSet>();
        }

    private:
        MotionSetsWindowPlugin* m_plugin;
    };

    MotionSetsWindowPlugin::MotionSetsWindowPlugin()
        : EMStudio::DockWidgetPlugin()
    {
        m_dialogStack                    = nullptr;
        m_selectedSet                    = nullptr;
        m_motionSetManagementWindow      = nullptr;
        m_motionSetWindow                = nullptr;
        m_dirtyFilesCallback             = nullptr;
    }

    MotionSetsWindowPlugin::~MotionSetsWindowPlugin()
    {
        for (MCore::Command::Callback* callback : m_callbacks)
        {
            GetCommandManager()->RemoveCommandCallback(callback, false);
            delete callback;
        }

        GetMainWindow()->GetDirtyFileManager()->RemoveCallback(m_dirtyFilesCallback, false);
        delete m_dirtyFilesCallback;
    }

    EMStudioPlugin* MotionSetsWindowPlugin::Clone()
    {
        MotionSetsWindowPlugin* newPlugin = new MotionSetsWindowPlugin();
        return newPlugin;
    }

    // init after the parent dock window has been created
    bool MotionSetsWindowPlugin::Init()
    {
        auto AddCallback = [=](const char* commandName, MCore::Command::Callback* callback)
        {
            m_callbacks.emplace_back(callback);
            GetCommandManager()->RegisterCommandCallback(commandName, m_callbacks.back());
        };

        AddCallback("CreateMotionSet", new CommandReinitCallback(false));
        AddCallback("RemoveMotionSet", new CommandReinitCallback(false));
        AddCallback("RemoveMotion", new CommandReinitCallback(false));
        AddCallback("AdjustMotionSet", new CommandAdjustMotionSetCallback(false));
        AddCallback("MotionSetAddMotion", new CommandMotionSetAddMotionCallback(false));
        AddCallback("MotionSetRemoveMotion", new CommandMotionSetRemoveMotionCallback(false));
        AddCallback("MotionSetAdjustMotion", new CommandMotionSetAdjustMotionCallback(false));
        AddCallback("LoadMotionSet", new CommandLoadMotionSetCallback(false));

        // create the dialog stack
        assert(m_dialogStack == nullptr);
        m_dialogStack = new MysticQt::DialogStack(m_dock);
        m_dock->setWidget(m_dialogStack);

        // connect the window activation signal to refresh if reactivated
        connect(m_dock, &QDockWidget::visibilityChanged, this, &MotionSetsWindowPlugin::WindowReInit);

        // create the set management window
        m_motionSetManagementWindow = new MotionSetManagementWindow(this, m_dialogStack);
        m_motionSetManagementWindow->Init();
        m_dialogStack->Add(m_motionSetManagementWindow, "Motion Set Management", false, true, true, false);

        // create the motion set properties window
        m_motionSetWindow = new MotionSetWindow(this, m_dialogStack);
        m_motionSetWindow->Init();
        m_dialogStack->Add(m_motionSetWindow, "Motion Set", false, true);

        ReInit();
        SetSelectedSet(nullptr);

        // initialize the dirty files callback
        m_dirtyFilesCallback = new SaveDirtyMotionSetFilesCallback(this);
        GetMainWindow()->GetDirtyFileManager()->AddCallback(m_dirtyFilesCallback);

        return true;
    }


    EMotionFX::MotionSet* MotionSetsWindowPlugin::GetSelectedSet() const
    {
        if (EMotionFX::GetMotionManager().FindMotionSetIndex(m_selectedSet) == InvalidIndex)
        {
            return nullptr;
        }

        return m_selectedSet;
    }


    void MotionSetsWindowPlugin::ReInit()
    {
        // Validate existence of selected motion set and reset selection in case selection is invalid.
        if (EMotionFX::GetMotionManager().FindMotionSetIndex(m_selectedSet) == InvalidIndex)
        {
            m_selectedSet = nullptr;
        }

        SetSelectedSet(m_selectedSet);
        m_motionSetManagementWindow->ReInit();
        m_motionSetWindow->ReInit();
    }


    int MotionSetsWindowPlugin::SaveDirtyMotionSet(EMotionFX::MotionSet* motionSet, MCore::CommandGroup* commandGroup, bool askBeforeSaving, bool showCancelButton)
    {
        // only save root motion sets
        if (motionSet->GetParentSet())
        {
            return DirtyFileManager::NOFILESTOSAVE;
        }

        // only process changed files
        if (motionSet->GetDirtyFlag() == false)
        {
            return DirtyFileManager::NOFILESTOSAVE;
        }

        if (askBeforeSaving)
        {
            EMStudio::GetApp()->setOverrideCursor(QCursor(Qt::ArrowCursor));

            QMessageBox msgBox(GetMainWindow());
            AZStd::string text;
            const AZStd::string& filename = motionSet->GetFilenameString();
            AZStd::string extension;
            AzFramework::StringFunc::Path::GetExtension(filename.c_str(), extension, false /* include dot */);

            if (!filename.empty() && !extension.empty())
            {
                text = AZStd::string::format("Save changes to '%s'?", motionSet->GetFilename());
            }
            else if (!motionSet->GetNameString().empty())
            {
                text = AZStd::string::format("Save changes to the motion set named '%s'?", motionSet->GetName());
            }
            else
            {
                text = "Save changes to untitled motion set?";
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
                GetMainWindow()->GetFileManager()->SaveMotionSet(GetManagementWindow(), motionSet, commandGroup);
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
            GetMainWindow()->GetFileManager()->SaveMotionSet(GetManagementWindow(), motionSet, commandGroup);
        }

        return DirtyFileManager::FINISHED;
    }


    void MotionSetsWindowPlugin::SetSelectedSet(EMotionFX::MotionSet* motionSet)
    {
        m_selectedSet = motionSet;

        if (motionSet)
        {
            m_motionSetManagementWindow->SelectItemsById(motionSet->GetID());
        }
        m_motionSetManagementWindow->ReInit();
        m_motionSetManagementWindow->UpdateInterface();
        m_motionSetWindow->ReInit();
        m_motionSetWindow->UpdateInterface();
    }


    // reinit the window when it gets activated
    void MotionSetsWindowPlugin::WindowReInit(bool visible)
    {
        if (visible)
        {
            ReInit();
        }
    }


    void MotionSetsWindowPlugin::LoadMotionSet(AZStd::string filename)
    {
        if (filename.empty())
        {
            return;
        }

        // Auto-relocate to asset source folder.
        if (!GetMainWindow()->GetFileManager()->RelocateToAssetSourceFolder(filename))
        {
            const AZStd::string errorString = AZStd::string::format("Unable to find MotionSet -filename \"%s\"", filename.c_str());
            AZ_Error("EMotionFX", false, errorString.c_str());
            return;
        }

        const AZStd::string command = AZStd::string::format("LoadMotionSet -filename \"%s\"", filename.c_str());

        AZStd::string result;
        if (!EMStudio::GetCommandManager()->ExecuteCommand(command, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }


    //-----------------------------------------------------------------------------------------
    // Command callbacks
    //-----------------------------------------------------------------------------------------
    bool ReInitMotionSetsPlugin()
    {
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(MotionSetsWindowPlugin::CLASS_ID);
        if (plugin == nullptr)
        {
            return false;
        }

        MotionSetsWindowPlugin* motionSetsPlugin = (MotionSetsWindowPlugin*)plugin;
        motionSetsPlugin->ReInit();

        return true;
    }


    bool UpdateMotionSetsPlugin()
    {
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(MotionSetsWindowPlugin::CLASS_ID);
        if (plugin == nullptr)
        {
            return false;
        }

        MotionSetsWindowPlugin* motionSetsPlugin = (MotionSetsWindowPlugin*)plugin;

        // is the plugin visible? only update it if it is visible
        if (motionSetsPlugin->GetDockWidget()->visibleRegion().isEmpty() == false)
        {
            motionSetsPlugin->SetSelectedSet(motionSetsPlugin->GetSelectedSet());
        }

        return true;
    }


    bool MotionSetsWindowPlugin::GetMotionSetCommandInfo(MCore::Command* command, const MCore::CommandLine& parameters, EMotionFX::MotionSet** outMotionSet, MotionSetsWindowPlugin** outPlugin)
    {
        // Get the motion set id and find the corresponding motion set.
        const int32 motionSetID = parameters.GetValueAsInt("motionSetID", command);
        EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().FindMotionSetByID(motionSetID);
        if (!motionSet)
        {
            return false;
        }

        // Get the plugin and add the motion entry to the user interface.
        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(MotionSetsWindowPlugin::CLASS_ID);
        if (!plugin)
        {
            return false;
        }
        MotionSetsWindowPlugin* motionSetsPlugin = (MotionSetsWindowPlugin*)plugin;

        *outMotionSet = motionSet;
        *outPlugin = motionSetsPlugin;
        return true;
    }


    EMotionFX::MotionSet::MotionEntry* MotionSetsWindowPlugin::FindBestMatchMotionEntryById(const AZStd::string& motionId)
    {
        // Find motion entry by motionId through motion sets, return nullptr if not found.
        const CommandSystem::SelectionList& selectionList = GetCommandManager()->GetCurrentSelection();
        EMotionFX::MotionSet* motionSet = nullptr;
        EMotionFX::MotionSet::MotionEntry* motionEntry = nullptr;

        // Look for motion entry in animgraph instance of the selected actor instance.
        EMotionFX::ActorInstance* actorInstance = selectionList.GetSingleActorInstance();
        if (actorInstance)
        {
            EMotionFX::AnimGraphInstance* animGraphInstance = actorInstance->GetAnimGraphInstance();
            if (animGraphInstance)
            {
                EMotionFX::MotionSet* animGraphInstanceMotionSet = animGraphInstance->GetMotionSet();
                if (animGraphInstanceMotionSet)
                {
                    motionEntry = animGraphInstanceMotionSet->RecursiveFindMotionEntryById(motionId);
                    if (motionEntry)
                    {
                        return motionEntry;
                    }
                }
            }
        }
        
        // If motion entry not found, look in MotionSet combo box of the AnimGraph Editor, find the motion entry by motionId.
        EMotionFX::AnimGraphEditorRequestBus::BroadcastResult(motionSet, &EMotionFX::AnimGraphEditorRequests::GetSelectedMotionSet);
        if (motionSet)
        {
            motionEntry = motionSet->RecursiveFindMotionEntryById(motionId);
            if (motionEntry)
            {
                return motionEntry;
            }
        }

        // If motion entry is still not found, look through all motion sets not owned by runtime.
        const EMotionFX::MotionManager& motionManager = EMotionFX::GetMotionManager();
        for(size_t i = 0; i < motionManager.GetNumMotionSets(); ++i)
        {
            motionSet = motionManager.GetMotionSet(i);
            if (motionSet->GetIsOwnedByRuntime())
            {
                continue;
            }
            motionEntry = motionSet->RecursiveFindMotionEntryById(motionId);
            if (motionEntry)
            {
                return motionEntry;
            }
        }
        return nullptr;
    }


    bool MotionSetsWindowPlugin::CommandReinitCallback::Execute([[maybe_unused]] MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(commandLine);
        return ReInitMotionSetsPlugin();
    }

    bool MotionSetsWindowPlugin::CommandReinitCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)                   { MCORE_UNUSED(command); MCORE_UNUSED(commandLine); return ReInitMotionSetsPlugin(); }

    bool MotionSetsWindowPlugin::CommandAdjustMotionSetCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);

        if (commandLine.CheckIfHasParameter("newName"))
        {
            // add it to the interfaces
            EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(MotionSetsWindowPlugin::CLASS_ID);
            if (plugin == nullptr)
            {
                return false;
            }

            MotionSetsWindowPlugin* motionSetsPlugin = (MotionSetsWindowPlugin*)plugin;
            motionSetsPlugin->GetManagementWindow()->ReInit();
        }

        return true;
    }

    bool MotionSetsWindowPlugin::CommandAdjustMotionSetCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);

        if (commandLine.CheckIfHasParameter("newName"))
        {
            // add it to the interfaces
            EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(MotionSetsWindowPlugin::CLASS_ID);
            if (plugin == nullptr)
            {
                return false;
            }

            MotionSetsWindowPlugin* motionSetsPlugin = (MotionSetsWindowPlugin*)plugin;
            motionSetsPlugin->GetManagementWindow()->ReInit();
        }

        return true;
    }


    bool MotionSetsWindowPlugin::CommandMotionSetAddMotionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        UpdateMotionSetsPlugin();
        return true;
    }


    bool MotionSetsWindowPlugin::CommandMotionSetAddMotionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        // Calls the MotionSetRemoveMotion command internally, so the callback from that is already called.
        return true;
    }


    bool MotionSetsWindowPlugin::CommandMotionSetRemoveMotionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        UpdateMotionSetsPlugin();
        return true;
    }


    bool MotionSetsWindowPlugin::CommandMotionSetRemoveMotionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        // Calls the MotionSetAddMotion command internally, so the callback from that is already called.
        return true;
    }


    bool CommandMotionSetAdjustMotionCallbackHelper(MCore::Command* command, const MCore::CommandLine& commandLine, const char* newMotionId, const char* oldMotionId)
    {
        EMotionFX::MotionSet* motionSet = nullptr;
        MotionSetsWindowPlugin* plugin = nullptr;
        if (!MotionSetsWindowPlugin::GetMotionSetCommandInfo(command, commandLine, &motionSet, &plugin))
        {
            return false;
        }

        // Find the corresponding motion entry.
        EMotionFX::MotionSet::MotionEntry* motionEntry = motionSet->FindMotionEntryById(newMotionId);
        if (!motionEntry)
        {
            return false;
        }

        return plugin->GetMotionSetWindow()->UpdateMotion(motionSet, motionEntry, oldMotionId);
    }


    bool MotionSetsWindowPlugin::CommandMotionSetAdjustMotionCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        // Get the previous motion id.
        AZStd::string newMotionId;
        AZStd::string oldMotionId;
        if (commandLine.CheckIfHasParameter("newIDString"))
        {
            commandLine.GetValue("newIDString", command, newMotionId);
            commandLine.GetValue("idString", command, oldMotionId);
        }
        else
        {
            commandLine.GetValue("idString", command, newMotionId);
            commandLine.GetValue("idString", command, oldMotionId);
        }

        return CommandMotionSetAdjustMotionCallbackHelper(command, commandLine, newMotionId.c_str(), oldMotionId.c_str());
    }


    bool MotionSetsWindowPlugin::CommandMotionSetAdjustMotionCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        // Calls the MotionSetAdjustMotion command internally, so the callback from that is already called.
        return true;
    }


    bool MotionSetsWindowPlugin::CommandLoadMotionSetCallback::Execute(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);

        AZStd::string filename;
        commandLine.GetValue("filename", command, filename);
        EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePathKeepCase, filename);

        CommandEditorLoadAnimGraph::RelocateFilename(filename);

        EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().FindMotionSetByFileName(filename.c_str());
        if (motionSet == nullptr)
        {
            AZ_Error("Animation", false, "Cannot find motion set.");
            return false;
        }

        EMStudioPlugin* plugin = EMStudio::GetPluginManager()->FindActivePlugin(MotionSetsWindowPlugin::CLASS_ID);
        if (plugin == nullptr)
        {
            return false;
        }

        MotionSetsWindowPlugin* motionSetsPlugin = (MotionSetsWindowPlugin*)plugin;

        // select the first motion set
        if (EMotionFX::GetMotionManager().GetNumMotionSets() > 0)
        {
            const size_t numMotionSets = EMotionFX::GetMotionManager().GetNumMotionSets();

            for (size_t i = 0; i < numMotionSets; ++i)
            {
                EMotionFX::MotionSet* motionSet2 = EMotionFX::GetMotionManager().GetMotionSet(0);

                if (motionSet2->GetIsOwnedByRuntime())
                {
                    continue;
                }

                motionSetsPlugin->SetSelectedSet(motionSet2);
                break;
            }
        }

        return ReInitMotionSetsPlugin();
    }


    bool MotionSetsWindowPlugin::CommandLoadMotionSetCallback::Undo(MCore::Command* command, const MCore::CommandLine& commandLine)
    {
        MCORE_UNUSED(command);
        MCORE_UNUSED(commandLine);
        return ReInitMotionSetsPlugin();
    }


    int MotionSetsWindowPlugin::OnSaveDirtyMotionSets()
    {
        return GetMainWindow()->GetDirtyFileManager()->SaveDirtyFiles(SaveDirtyMotionSetFilesCallback::TYPE_ID);
    }

    void MotionSetsWindowPlugin::OnAfterLoadProject()
    {
        ReInitMotionSetsPlugin();
    }
} // namespace EMStudio

#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/MotionSetsWindow/moc_MotionSetsWindowPlugin.cpp>
