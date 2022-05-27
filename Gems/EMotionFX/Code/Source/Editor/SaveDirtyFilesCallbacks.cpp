/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/FileManager.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/MainWindow.h>
#include <Editor/SaveDirtyFilesCallbacks.h>
#include <QApplication>
#include <QMessageBox>

namespace EMStudio
{
    void SaveDirtyActorFilesCallback::GetDirtyFileNames(
        AZStd::vector<AZStd::string>* outFileNames, AZStd::vector<ObjectPointer>* outObjects)
    {
        const size_t numLeaderActors = EMotionFX::GetActorManager().GetNumActors();
        for (size_t i = 0; i < numLeaderActors; ++i)
        {
            EMotionFX::Actor* actor = EMotionFX::GetActorManager().GetActor(i);

            if (actor->GetDirtyFlag() &&
                !actor->GetIsUsedForVisualization())
            {
                outFileNames->push_back(actor->GetFileName());

                ObjectPointer objPointer;
                objPointer.m_actor = actor;
                outObjects->push_back(objPointer);
            }
        }
    }

    int SaveDirtyActorFilesCallback::SaveDirtyFiles(
        [[maybe_unused]] const AZStd::vector<AZStd::string>& filenamesToSave,
        const AZStd::vector<ObjectPointer>& objects,
        MCore::CommandGroup* commandGroup)
    {
        for (const ObjectPointer& objPointer : objects)
        {
            if (!objPointer.m_actor)
            {
                continue;
            }

            EMotionFX::Actor* actor = objPointer.m_actor;
            if (SaveDirtyActor(actor, commandGroup, false) == DirtyFileManager::CANCELED)
            {
                return DirtyFileManager::CANCELED;
            }
        }

        return DirtyFileManager::FINISHED;
    }

    int SaveDirtyActorFilesCallback::SaveDirtyActor(
        EMotionFX::Actor* actor, [[maybe_unused]] MCore::CommandGroup* commandGroup, bool askBeforeSaving, bool showCancelButton)
    {
        // only process changed files
        if (!actor->GetDirtyFlag())
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

            if (!filename.empty() &&
                !extension.empty())
            {
                text = AZStd::string::format("Save changes to '%s'?", actor->GetFileName());
            }
            else if (!actor->GetNameString().empty())
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

    ///////////////////////////////////////////////////////////////////////////

    void SaveDirtyMotionFilesCallback::GetDirtyFileNames(
        AZStd::vector<AZStd::string>* outFileNames, AZStd::vector<ObjectPointer>* outObjects)
    {
        const size_t numMotions = EMotionFX::GetMotionManager().GetNumMotions();
        for (size_t i = 0; i < numMotions; ++i)
        {
            EMotionFX::Motion* motion = EMotionFX::GetMotionManager().GetMotion(i);

            if (motion->GetIsOwnedByRuntime())
            {
                continue;
            }

            if (motion->GetDirtyFlag())
            {
                outFileNames->push_back(motion->GetFileName());

                ObjectPointer objPointer;
                objPointer.m_motion = motion;
                outObjects->push_back(objPointer);
            }
        }
    }

    int SaveDirtyMotionFilesCallback::SaveDirtyFiles(
        [[maybe_unused]] const AZStd::vector<AZStd::string>& filenamesToSave,
        const AZStd::vector<ObjectPointer>& objects,
        MCore::CommandGroup* commandGroup)
    {
        for (const ObjectPointer& objPointer : objects)
        {
            if (!objPointer.m_motion)
            {
                continue;
            }

            EMotionFX::Motion* motion = objPointer.m_motion;
            if (SaveDirtyMotion(motion, commandGroup, false) == DirtyFileManager::CANCELED)
            {
                return DirtyFileManager::CANCELED;
            }
        }

        return DirtyFileManager::FINISHED;
    }

    int SaveDirtyMotionFilesCallback::SaveDirtyMotion(
        EMotionFX::Motion* motion, [[maybe_unused]] MCore::CommandGroup* commandGroup, bool askBeforeSaving, bool showCancelButton)
    {
        // only process changed files
        if (!motion->GetDirtyFlag())
        {
            return DirtyFileManager::NOFILESTOSAVE;
        }

        if (askBeforeSaving)
        {
            EMStudio::GetApp()->setOverrideCursor(QCursor(Qt::ArrowCursor));

            QMessageBox msgBox(GetMainWindow());
            AZStd::string text;

            if (!motion->GetFileNameString().empty())
            {
                text = AZStd::string::format("Save changes to '%s'?", motion->GetFileName());
            }
            else if (!motion->GetNameString().empty())
            {
                text = AZStd::string::format("Save changes to the motion named '%s'?", motion->GetName());
            }
            else
            {
                text = "Save changes to untitled motion?";
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
                    GetMainWindow()->GetFileManager()->SaveMotion(motion->GetID());
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
            GetMainWindow()->GetFileManager()->SaveMotion(motion->GetID());
        }

        return DirtyFileManager::FINISHED;
    }

    ///////////////////////////////////////////////////////////////////////////

    void SaveDirtyMotionSetFilesCallback::GetDirtyFileNames(
        AZStd::vector<AZStd::string>* outFileNames, AZStd::vector<ObjectPointer>* outObjects)
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

            if (motionSet->GetDirtyFlag())
            {
                outFileNames->push_back(motionSet->GetFilename());

                ObjectPointer objPointer;
                objPointer.m_motionSet = motionSet;
                outObjects->push_back(objPointer);
            }
        }
    }

    int SaveDirtyMotionSetFilesCallback::SaveDirtyFiles(
        [[maybe_unused]] const AZStd::vector<AZStd::string>& filenamesToSave,
        const AZStd::vector<ObjectPointer>& objects,
        MCore::CommandGroup* commandGroup)
    {
        for (const ObjectPointer& objPointer : objects)
        {
            if (!objPointer.m_motionSet)
            {
                continue;
            }

            EMotionFX::MotionSet* motionSet = objPointer.m_motionSet;
            if (SaveDirtyMotionSet(motionSet, commandGroup, false) == DirtyFileManager::CANCELED)
            {
                return DirtyFileManager::CANCELED;
            }
        }

        return DirtyFileManager::FINISHED;
    }

    int SaveDirtyMotionSetFilesCallback::SaveDirtyMotionSet(
        EMotionFX::MotionSet* motionSet, MCore::CommandGroup* commandGroup, bool askBeforeSaving, bool showCancelButton)
    {
        // only save root motion sets
        if (motionSet->GetParentSet())
        {
            return DirtyFileManager::NOFILESTOSAVE;
        }

        // only process changed files
        if (!motionSet->GetDirtyFlag())
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
                    GetMainWindow()->GetFileManager()->SaveMotionSet(GetMainWindow(), motionSet, commandGroup);
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
            GetMainWindow()->GetFileManager()->SaveMotionSet(GetMainWindow(), motionSet, commandGroup);
        }

        return DirtyFileManager::FINISHED;
    }

    ///////////////////////////////////////////////////////////////////////////

    void SaveDirtyAnimGraphFilesCallback::GetDirtyFileNames(
        AZStd::vector<AZStd::string>* outFileNames, AZStd::vector<ObjectPointer>* outObjects)
    {
        const size_t numAnimGraphs = EMotionFX::GetAnimGraphManager().GetNumAnimGraphs();
        for (size_t i = 0; i < numAnimGraphs; ++i)
        {
            EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().GetAnimGraph(i);

            if (animGraph->GetIsOwnedByRuntime())
            {
                continue;
            }

            if (animGraph->GetDirtyFlag())
            {
                outFileNames->push_back(animGraph->GetFileName());

                ObjectPointer objPointer;
                objPointer.m_animGraph = animGraph;
                outObjects->push_back(objPointer);
            }
        }
    }

    int SaveDirtyAnimGraphFilesCallback::SaveDirtyFiles(
        [[maybe_unused]] const AZStd::vector<AZStd::string>& filenamesToSave,
        const AZStd::vector<ObjectPointer>& objects,
        MCore::CommandGroup* commandGroup)
    {
        for (const ObjectPointer& objPointer : objects)
        {
            if (!objPointer.m_animGraph)
            {
                continue;
            }

            EMotionFX::AnimGraph* animGraph = objPointer.m_animGraph;
            if (SaveDirtyAnimGraph(animGraph, commandGroup, false) == DirtyFileManager::CANCELED)
            {
                return DirtyFileManager::CANCELED;
            }
        }

        return DirtyFileManager::FINISHED;
    }

    int SaveDirtyAnimGraphFilesCallback::SaveDirtyAnimGraph(
        EMotionFX::AnimGraph* animGraph, MCore::CommandGroup* commandGroup, bool askBeforeSaving, bool showCancelButton)
    {
        if (!animGraph)
        {
            return QMessageBox::Discard;
        }

        // only process changed files
        if (!animGraph->GetDirtyFlag())
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
                    GetMainWindow()->GetFileManager()->SaveAnimGraph(GetMainWindow(), animGraph, commandGroup);
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
            GetMainWindow()->GetFileManager()->SaveAnimGraph(GetMainWindow(), animGraph, commandGroup);
        }

        return DirtyFileManager::FINISHED;
    }

    ///////////////////////////////////////////////////////////////////////////

    void SaveDirtyWorkspaceCallback::GetDirtyFileNames(AZStd::vector<AZStd::string>* outFileNames, AZStd::vector<ObjectPointer>* outObjects)
    {
        Workspace* workspace = GetManager()->GetWorkspace();
        if (workspace->GetDirtyFlag())
        {
            // add the filename to the dirty filenames array
            outFileNames->push_back(workspace->GetFilename());

            // add the link to the actual object
            ObjectPointer objPointer;
            objPointer.m_workspace = workspace;
            outObjects->push_back(objPointer);
        }
    }

    int SaveDirtyWorkspaceCallback::SaveDirtyFiles(
        [[maybe_unused]] const AZStd::vector<AZStd::string>& filenamesToSave,
        const AZStd::vector<ObjectPointer>& objects,
        MCore::CommandGroup* commandGroup)
    {
        for (const ObjectPointer& objPointer : objects)
        {
            if (!objPointer.m_workspace)
            {
                continue;
            }

            Workspace* workspace = objPointer.m_workspace;

            // has the workspace been saved already or is it a new one?
            if (workspace->GetFilenameString().empty())
            {
                // open up save as dialog so that we can choose a filename
                const AZStd::string filename = GetMainWindow()->GetFileManager()->SaveWorkspaceFileDialog(GetMainWindow());
                if (filename.empty())
                {
                    return DirtyFileManager::CANCELED;
                }

                // save the workspace using the newly selected filename
                AZStd::string command = AZStd::string::format("SaveWorkspace -filename \"%s\"", filename.c_str());
                commandGroup->AddCommandString(command);
            }
            else
            {
                // save workspace using its filename
                AZStd::string command = AZStd::string::format("SaveWorkspace -filename \"%s\"", workspace->GetFilename());
                commandGroup->AddCommandString(command);
            }
        }

        return DirtyFileManager::FINISHED;
    }
} // namespace EMStudio
