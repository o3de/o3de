/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Workspace.h"
#include "EMStudioManager.h"
#include "FileManager.h"
#include "MainWindow.h"
#include <MCore/Source/LogManager.h>
#include <MCore/Source/FileSystem.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/AttachmentNode.h>
#include <EMotionFX/Source/AttachmentSkin.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/CommandSystem/Source/MotionSetCommands.h>
#include <EMotionFX/Source/ActorManager.h>

#include <AzCore/IO/Path/Path.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

#include <QApplication>
#include <QMessageBox>
#include <QSettings>
#include <QFile>


namespace EMStudio
{
    Workspace::Workspace()
    {
        m_dirtyFlag = false;
    }


    Workspace::~Workspace()
    {
    }


    void Workspace::AddFile(AZStd::string* inOutCommands, const char* command, const AZStd::string& filename, const char* additionalParameters) const
    {
        if (filename.empty())
        {
            return;
        }

        AZStd::string commandString;
        AZ::IO::Path resultFileName = filename;

        // If the filename is in the asset source folder, we relocated it to the cache folder first.
        if (!GetMainWindow()->GetFileManager()->IsFileInAssetCache(resultFileName.Native()) &&
            GetMainWindow()->GetFileManager()->IsFileInAssetSource(resultFileName.Native()))
        {
            GetMainWindow()->GetFileManager()->RelocateToAssetCacheFolder(resultFileName.Native());
        }

        if (GetMainWindow()->GetFileManager()->IsFileInAssetCache(resultFileName.Native()))
        {
            // Retrieve relative filename for file in cache folder.
            if (auto fileIoBase = AZ::IO::FileIOBase::GetInstance(); fileIoBase != nullptr)
            {
                AZ::IO::FixedMaxPath convertedPath;
                fileIoBase->ConvertToAlias(convertedPath, resultFileName);
                resultFileName = convertedPath;
            }
        }
        else
        {
            // If the filename is not in asset source folder or cache folder, then we try find it using the asset system.
            bool success = false;
            AZStd::string watchFolder;
            AZ::Data::AssetInfo assetInfo;
            AzToolsFramework::AssetSystemRequestBus::BroadcastResult(success, &AzToolsFramework::AssetSystemRequestBus::Events::GetSourceInfoBySourcePath, filename.c_str(), assetInfo, watchFolder);
            if (success)
            {
                // In this case, the file likely exists in the other folder not scanned by AP (e.g Gems/something/Assets)
                // AP will process this file and move the processed file in cache folder.
                AZ::IO::Path assetCachePath;
                if (auto settingsRegistry = AZ::SettingsRegistry::Get(); settingsRegistry != nullptr)
                {
                    settingsRegistry->Get(assetCachePath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_CacheRootFolder);
                }
                if (auto fileIoBase = AZ::IO::FileIOBase::GetInstance(); fileIoBase != nullptr)
                {
                    AZ::IO::FixedMaxPath convertedPath;
                    fileIoBase->ConvertToAlias(convertedPath, assetCachePath / assetInfo.m_relativePath);
                    resultFileName = convertedPath;
                }
            }
            else
            {
                // If the file can't be detected by the AP, then we store an absolute path instead.
                // This will result in workspace not compatible when using by other machine.
                AZ_Warning("EMotionFX", true, "File %s is not cannot be found in the asset system, using absolute path instead.");
            }
        }

        AZStd::string resultFilenameString = resultFileName.c_str();
        AzFramework::StringFunc::AssetDatabasePath::Normalize(resultFilenameString);
        commandString = AZStd::string::format("%s -filename \"%s\"", command, resultFilenameString.c_str());

        if (additionalParameters)
        {
            commandString += " ";
            commandString += additionalParameters;
        }

        commandString += "\n";

        (*inOutCommands) += commandString;
    }

    struct ActivationIndices
    {
        int32 m_actorInstanceCommandIndex = -1;
        int32 m_animGraphCommandIndex = -1;
        int32 m_motionSetCommandIndex = -1;
    };

    bool Workspace::SaveToFile(const char* filename) const
    {
        QSettings settings(filename, QSettings::IniFormat, GetManager()->GetMainWindow());

        AZStd::string commandString;
        AZStd::string commands;

        // For each actor we are going to set a different activation command. We need to store the command indices
        // that will produce the results for the actorInstanceIndex, animGraphIndex and motionSetIndex that are
        // currently selected
        typedef AZStd::unordered_map<EMotionFX::ActorInstance*, ActivationIndices> ActivationIndicesByActorInstance;
        ActivationIndicesByActorInstance activationIndicesByActorInstance;
        int32 commandIndex = 0;

        const size_t numActorInstances = EMotionFX::GetActorManager().GetNumActorInstances();

        // actors
        const size_t numActors = EMotionFX::GetActorManager().GetNumActors();
        for (size_t i = 0; i < numActors; ++i)
        {
            EMotionFX::Actor* actor = EMotionFX::GetActorManager().GetActor(i);

            for (size_t j = 0; j < numActorInstances; ++j)
            {
                EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().GetActorInstance(j);
                if (actorInstance->GetActor() != actor)
                {
                    continue;
                }

                if (actorInstance->GetIsOwnedByRuntime())
                {
                    continue;
                }

                const EMotionFX::Transform& transform = actorInstance->GetLocalSpaceTransform();
                const AZ::Vector3&       pos     = transform.m_position;
                const AZ::Quaternion&    rot     = transform.m_rotation;

                #ifndef EMFX_SCALE_DISABLED
                    const AZ::Vector3& scale = transform.m_scale;
                #else
                    const AZ::Vector3 scale = AZ::Vector3::CreateOne();
                #endif

                // We need to add it here because if we dont do the last result will be the id of the actor instance and then it won't work anymore.
                AddFile(&commands, "ImportActor", actor->GetFileName());
                ++commandIndex;
                commandString = AZStd::string::format("CreateActorInstance -actorID %%LASTRESULT%% -xPos %f -yPos %f -zPos %f -xScale %f -yScale %f -zScale %f -rot %s\n",
                        static_cast<float>(pos.GetX()), static_cast<float>(pos.GetY()), static_cast<float>(pos.GetZ()), static_cast<float>(scale.GetX()), static_cast<float>(scale.GetY()), static_cast<float>(scale.GetZ()),
                        AZStd::to_string(rot).c_str());
                commands += commandString;

                activationIndicesByActorInstance[actorInstance].m_actorInstanceCommandIndex = commandIndex;
                ++commandIndex;
            }
        }

        // attachments
        for (size_t i = 0; i < numActorInstances; ++i)
        {
            EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().GetActorInstance(i);

            if (actorInstance->GetIsOwnedByRuntime())
            {
                continue;
            }

            if (actorInstance->GetIsAttachment())
            {
                EMotionFX::Attachment*      attachment                  = actorInstance->GetSelfAttachment();
                EMotionFX::ActorInstance*   attachedToActorInstance     = attachment->GetAttachToActorInstance();
                const size_t                attachedToInstanceIndex     = EMotionFX::GetActorManager().FindActorInstanceIndex(attachedToActorInstance);
                const size_t                attachtmentInstanceIndex    = EMotionFX::GetActorManager().FindActorInstanceIndex(actorInstance);

                if (actorInstance->GetIsSkinAttachment())
                {
                    commandString = AZStd::string::format("AddDeformableAttachment -attachmentIndex %zu -attachToIndex %zu\n", attachtmentInstanceIndex, attachedToInstanceIndex);
                    commands += commandString;
                    ++commandIndex;
                }
                else
                {
                    EMotionFX::AttachmentNode*  attachmentSingleNode    = static_cast<EMotionFX::AttachmentNode*>(attachment);
                    const size_t                attachedToNodeIndex     = attachmentSingleNode->GetAttachToNodeIndex();
                    EMotionFX::Actor*           attachedToActor         = attachedToActorInstance->GetActor();
                    EMotionFX::Node*            attachedToNode          = attachedToActor->GetSkeleton()->GetNode(attachedToNodeIndex);

                    commandString = AZStd::string::format("AddAttachment -attachmentIndex %zu -attachToIndex %zu -attachToNode \"%s\"\n", attachtmentInstanceIndex, attachedToInstanceIndex, attachedToNode->GetName());
                    commands += commandString;
                    ++commandIndex;
                }
            }
        }

        // motion sets
        const size_t numRootMotionSets = EMotionFX::GetMotionManager().CalcNumRootMotionSets();
        AZStd::unordered_set<EMotionFX::Motion*> motionsInMotionSets;
        for (size_t i = 0; i < numRootMotionSets; ++i)
        {
            EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().FindRootMotionSet(i);

            if (motionSet->GetIsOwnedByRuntime())
            {
                continue;
            }

            AddFile(&commands, "LoadMotionSet", motionSet->GetFilename());

            for (ActivationIndicesByActorInstance::value_type& indicesByActorInstance : activationIndicesByActorInstance)
            {
                EMotionFX::AnimGraphInstance* animGraphInstance = indicesByActorInstance.first->GetAnimGraphInstance();
                if (!animGraphInstance)
                {
                    continue;
                }

                EMotionFX::MotionSet* currentActiveMotionSet = animGraphInstance->GetMotionSet();
                if (currentActiveMotionSet == motionSet)
                {
                    indicesByActorInstance.second.m_motionSetCommandIndex = commandIndex;
                    break;
                }
            }
            ++commandIndex;

            motionSet->RecursiveGetMotions(motionsInMotionSets);
        }

        // motions that are not in the above motion sets
        const size_t numMotions = EMotionFX::GetMotionManager().GetNumMotions();
        for (size_t i = 0; i < numMotions; ++i)
        {
            EMotionFX::Motion* motion = EMotionFX::GetMotionManager().GetMotion(i);

            if (motion->GetIsOwnedByRuntime())
            {
                continue;
            }
            if (motionsInMotionSets.find(motion) != motionsInMotionSets.end())
            {
                continue; // Already saved by a motion set
            }

            AddFile(&commands, "ImportMotion", motion->GetFileName());
            ++commandIndex;
        }

        // anim graphs
        // We need to avoid storing two times the same anim graph. This could happen if the anim graph was loaded from a reference
        // node. We need to integrate the asset system into the AnimGraphManager
        AZStd::unordered_set<AZStd::string> animGraphFilenames;
        const size_t numAnimGraphs = EMotionFX::GetAnimGraphManager().GetNumAnimGraphs();
        for (size_t i = 0; i < numAnimGraphs; ++i)
        {
            EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().GetAnimGraph(i);

            if (animGraph->GetIsOwnedByRuntime())
            {
                continue;
            }

            bool fileAdded = false;
            if (animGraphFilenames.emplace(animGraph->GetFileNameString()).second)
            {
                AddFile(&commands, "LoadAnimGraph", animGraph->GetFileName());
                fileAdded = true;
            }

            for (ActivationIndicesByActorInstance::value_type& indicesByActorInstance : activationIndicesByActorInstance)
            {
                EMotionFX::AnimGraphInstance* animGraphInstance = indicesByActorInstance.first->GetAnimGraphInstance();
                if (!animGraphInstance)
                {
                    continue;
                }

                EMotionFX::AnimGraph* currentActiveAnimGraph = animGraphInstance->GetAnimGraph();
                if (currentActiveAnimGraph->GetFileNameString() == animGraph->GetFileNameString())
                {
                    indicesByActorInstance.second.m_animGraphCommandIndex = commandIndex;
                    break;
                }
            }

            if (fileAdded)
            {
                ++commandIndex;
            }
        }

        // activate anim graph for each actor instance
        for (size_t i = 0; i < numActorInstances; ++i)
        {
            EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().GetActorInstance(i);

            if (actorInstance->GetIsOwnedByRuntime())
            {
                continue;
            }
            EMotionFX::AnimGraphInstance* animGraphInstance = actorInstance->GetAnimGraphInstance();
            if (!animGraphInstance)
            {
                continue;
            }

            ActivationIndicesByActorInstance::const_iterator itActivationIndices = activationIndicesByActorInstance.find(actorInstance);
            if (itActivationIndices == activationIndicesByActorInstance.end())
            {
                continue;
            }

            // only activate the saved anim graph
            if (itActivationIndices->second.m_animGraphCommandIndex != -1
                && itActivationIndices->second.m_motionSetCommandIndex != -1)
            {
                commandString = AZStd::string::format("ActivateAnimGraph -actorInstanceID %%LASTRESULT%d%% -animGraphID %%LASTRESULT%d%% -motionSetID %%LASTRESULT%d%% -visualizeScale %f\n",
                        (commandIndex - itActivationIndices->second.m_actorInstanceCommandIndex),
                        (commandIndex - itActivationIndices->second.m_animGraphCommandIndex),
                        (commandIndex - itActivationIndices->second.m_motionSetCommandIndex),
                        animGraphInstance->GetVisualizeScale());
                commands += commandString;
                ++commandIndex;
            }
        }

        // set workspace values
        settings.setValue("version", 1);
        settings.setValue("startScript", commands.c_str());

        // sync to ensure the status is correct because qt delays the write to file
        settings.sync();

        // check the status for no error
        return settings.status() == QSettings::NoError;
    }


    bool Workspace::Save(const char* filename, bool updateFileName, bool updateDirtyFlag)
    {
        // save to file secured by a backup file
        if (MCore::FileSystem::SaveToFileSecured(filename, [this, filename] { return SaveToFile(filename); }, GetCommandManager()))
        {
            // update the workspace filename
            if (updateFileName)
            {
                m_filename = filename;
            }

            // update the workspace dirty flag
            if (updateDirtyFlag)
            {
                GetCommandManager()->SetWorkspaceDirtyFlag(false);
                m_dirtyFlag = false;
            }

            // save succeeded
            return true;
        }
        else
        {
            // show the error report
            GetCommandManager()->ShowErrorReport();

            // save failed
            return false;
        }
    }


    bool Workspace::Load(const char* filename, MCore::CommandGroup* commandGroup)
    {
        if (!QFile::exists(filename))
        {
            return false;
        }

        QSettings settings(filename, QSettings::IniFormat, (QWidget*)GetManager()->GetMainWindow());

        m_filename       = filename;

        AZStd::string commandsString = FromQtString(settings.value("startScript", "").toString());

        AZStd::vector<AZStd::string> commands;
        AzFramework::StringFunc::Tokenize(commandsString.c_str(), commands, MCore::CharacterConstants::endLine, true /* keep empty strings */, true /* keep space strings */);

        const AZStd::string& assetCacheFolder = EMotionFX::GetEMotionFX().GetAssetCacheFolder();

        const size_t numCommands = commands.size();
        for (size_t i = 0; i < numCommands; ++i)
        {
            // check if the string is empty and skip it in this case
            if (commands[i].empty())
            {
                continue;
            }

            // if we are dealing with a comment skip it as well
            if (commands[i].find("//") == 0)
            {
                continue;
            }

            AzFramework::StringFunc::Replace(commands[i], "@products@", assetCacheFolder.c_str());
            AzFramework::StringFunc::Replace(commands[i], "@assets@", assetCacheFolder.c_str());
            AzFramework::StringFunc::Replace(commands[i], "@root@", assetCacheFolder.c_str());
            AzFramework::StringFunc::Replace(commands[i], "@projectplatformcache@", assetCacheFolder.c_str());
            AzFramework::StringFunc::Replace(commands[i], "//", AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING);
            AzFramework::StringFunc::Replace(commands[i], "\\\\", AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING);
            AzFramework::StringFunc::Replace(commands[i], "/\\", AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING);

            // add the command to the command group
            commandGroup->AddCommandString(commands[i]);
        }

        GetCommandManager()->SetWorkspaceDirtyFlag(false);
        m_dirtyFlag = false;
        return true;
    }


    void Workspace::Reset()
    {
        m_filename.clear();

        GetCommandManager()->SetWorkspaceDirtyFlag(false);
        m_dirtyFlag = false;
    }


    bool Workspace::GetDirtyFlag() const
    {
        if (m_dirtyFlag)
        {
            return true;
        }

        if (GetCommandManager()->GetWorkspaceDirtyFlag() && GetCommandManager()->GetUserOpenedWorkspaceFlag())
        {
            return true;
        }

        return false;
    }


    void Workspace::SetDirtyFlag(bool dirty)
    {
        m_dirtyFlag = dirty;
        GetCommandManager()->SetWorkspaceDirtyFlag(dirty);
    }
} // namespace EMStudio

