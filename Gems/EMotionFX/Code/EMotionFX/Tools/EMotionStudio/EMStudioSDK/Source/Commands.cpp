/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Commands.h"
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/CommandSystem/Source/MetaData.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/MainWindow.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/FileManager.h>

#include <SceneAPIExt/Rules/ActorPhysicsSetupRule.h>
#include <SceneAPIExt/Rules/SimulatedObjectSetupRule.h>
#include <SceneAPIExt/Rules/RootMotionExtractionRule.h>
#include <SceneAPIExt/Rules/MetaDataRule.h>
#include <SceneAPIExt/Rules/MotionMetaDataRule.h>
#include <SceneAPIExt/Groups/MotionGroup.h>
#include <SceneAPIExt/Groups/ActorGroup.h>


namespace EMStudio
{
    void SourceControlCommand::InitSyntax()
    {
        GetSyntax().AddParameter(
            "sourceControl",
            "Enable or disable source control auto checkout and add for this file (perforce etc).",
            MCore::CommandSyntax::PARAMTYPE_BOOLEAN,
            "true"
        );
    }

    /**
    * @brief: Checks out or adds a file to source control
    *
    * @param[in] parameters The MCore commandline for this Command, to query the value of the -sourceControl parameter.
    * @param[in] filename The name of the file to check out or add.
    * @param[out] outResult A description of any errors that happen.
    * @param[in,out] inOutFileExistedBefore True in case the file was already checked into source control before, false if not.
    *                                       In case the file gets added during the function call, the parameter will be adjusted accordingly.
    * @param[in] useSourceControl Checkout or add the file in case of true, only do a file existance check when false.
    * @param[in] add True when adding a newly-created file, false otherwise.
    *
    * @return: True on success, false otherwise
    *
    * This method is designed to be called twice, once before saving a file and
    * once after saving a file. Before the file is saved, @p add should be
    * false, and iff a file named @p filename exists, that file is checked out.
    * The second call, after the save operation, @p add should be true. If the
    * file was not checked out the first time, it is checked out the second
    * time.
    *
    * This is to accommodate the Perforce workflow: an existing file must be
    * checked out before editing it, a new file must be added after it is
    * created.
    */
    bool SourceControlCommand::CheckOutFile(const char* filename, bool& inOutFileExistedBefore, AZStd::string& outResult, bool useSourceControl, bool add)
    {
        if (add && inOutFileExistedBefore)
        {
            // We do not need to add the file if it existed before the save
            // operation, it will already have been checked out
            return true;
        }

        inOutFileExistedBefore = AZ::IO::FileIOBase::GetInstance()->Exists(filename);

        bool checkoutResult = true;
        if (useSourceControl && inOutFileExistedBefore)
        {
            using ApplicationBus = AzToolsFramework::ToolsApplicationRequestBus;
            ApplicationBus::BroadcastResult(checkoutResult,
                &ApplicationBus::Events::RequestEditForFileBlocking,
                filename,
                "Checking out file from source control.",
                ApplicationBus::Events::RequestEditProgressCallback());

            if (!checkoutResult)
            {
                outResult = AZStd::string::format("Cannot check out file '%s' from source control.", filename);
                AZ_Error("EMotionFX", false, outResult.c_str());
            }
        }

        return checkoutResult;
    }

    bool SourceControlCommand::CheckOutFile(const MCore::CommandLine& parameters, const char* filename, AZStd::string& outResult, bool add)
    {
        const bool sourceControl = parameters.GetValueAsBool("sourceControl", this);
        return CheckOutFile(filename, m_fileExistsBeforehand, outResult, sourceControl, add);
    }

    //--------------------------------------------------------------------------------
    // CommandSaveActorAssetInfo
    //--------------------------------------------------------------------------------
    CommandSaveActorAssetInfo::CommandSaveActorAssetInfo(MCore::Command* orgCommand)
        : MCore::Command("SaveActorAssetInfo", orgCommand)
    {
    }


    CommandSaveActorAssetInfo::~CommandSaveActorAssetInfo()
    {
    }


    bool CommandSaveActorAssetInfo::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        const uint32 actorID = parameters.GetValueAsInt("actorID", this);

        EMotionFX::Actor* actor = EMotionFX::GetActorManager().FindActorByID(actorID);
        if (actor == nullptr)
        {
            outResult = AZStd::string::format("Actor cannot be saved. Actor with id '%i' does not exist.", actorID);
            return false;
        }

        AZStd::string productFilename = actor->GetFileName();

        // Get the group name from the product filename (assuming the product filename is the group name).
        AZStd::string groupName;
        if (!AzFramework::StringFunc::Path::GetFileName(productFilename.c_str(), groupName))
        {
            outResult = AZStd::string::format("Cannot get product name from asset cache file '%s'.", productFilename.c_str());
            return false;
        }

        // Get the full file path for the asset source file based on the product filename.
        bool fullPathFound = false;
        AZStd::string sourceAssetFilename;
        EBUS_EVENT_RESULT(fullPathFound, AzToolsFramework::AssetSystemRequestBus, GetFullSourcePathFromRelativeProductPath, productFilename, sourceAssetFilename);

        // Generate meta data command for all changes being made to the actor.
        const AZStd::string metaDataString = CommandSystem::MetaData::GenerateActorMetaData(actor);

        AZ_TraceContext("External tool rule", sourceAssetFilename);

        if (sourceAssetFilename.empty())
        {
            AZ_Error("EMotionFX", false, "Source asset filename is empty.");
            return false;
        }

        // Load the manifest from disk.
        AZStd::shared_ptr<AZ::SceneAPI::Containers::Scene> scene;
        AZ::SceneAPI::Events::SceneSerializationBus::BroadcastResult(scene, &AZ::SceneAPI::Events::SceneSerializationBus::Events::LoadScene, sourceAssetFilename, AZ::Uuid::CreateNull(), "");
        if (!scene)
        {
            AZ_Error("EMotionFX", false, "Unable to save meta data to manifest due to failed scene loading.");
            return false;
        }

        AZ::SceneAPI::Containers::SceneManifest& manifest = scene->GetManifest();
        auto values = manifest.GetValueStorage();
        auto groupView = AZ::SceneAPI::Containers::MakeDerivedFilterView<EMotionFX::Pipeline::Group::ActorGroup>(values);
        for (EMotionFX::Pipeline::Group::ActorGroup& group : groupView)
        {
            // Non-case sensitive group name comparison. Product filenames are lower case only and might mismatch casing of the entered group name.
            if (AzFramework::StringFunc::Equal(group.GetName().c_str(), groupName.c_str()))
            {
                EMotionFX::Pipeline::Rule::MetaDataRule::SaveMetaData(*scene, group, metaDataString);

                // Save physics setup only in case there is some data.
                const AZStd::shared_ptr<EMotionFX::PhysicsSetup>& physicsSetup = actor->GetPhysicsSetup();
                if (!physicsSetup->GetRagdollConfig().m_nodes.empty() ||
                    !physicsSetup->GetHitDetectionConfig().m_nodes.empty() ||
                    !physicsSetup->GetClothConfig().m_nodes.empty() ||
                    !physicsSetup->GetSimulatedObjectColliderConfig().m_nodes.empty())
                {
                    EMotionFX::Pipeline::Rule::SaveToGroup<EMotionFX::Pipeline::Rule::ActorPhysicsSetupRule, AZStd::shared_ptr<EMotionFX::PhysicsSetup>>(*scene, group, physicsSetup);
                }
                else
                {
                    EMotionFX::Pipeline::Rule::RemoveRuleFromGroup<EMotionFX::Pipeline::Rule::ActorPhysicsSetupRule, AZStd::shared_ptr<EMotionFX::PhysicsSetup>>(*scene, group);
                }

                // Save simulated object rule
                const AZStd::shared_ptr<EMotionFX::SimulatedObjectSetup>& simulatedObjectSetup = actor->GetSimulatedObjectSetup();
                if (simulatedObjectSetup->GetNumSimulatedObjects() > 0)
                {
                    EMotionFX::Pipeline::Rule::SaveToGroup<EMotionFX::Pipeline::Rule::SimulatedObjectSetupRule, AZStd::shared_ptr<EMotionFX::SimulatedObjectSetup>>(*scene, group, simulatedObjectSetup);
                }
                else
                {
                    EMotionFX::Pipeline::Rule::RemoveRuleFromGroup<EMotionFX::Pipeline::Rule::SimulatedObjectSetupRule, AZStd::shared_ptr<EMotionFX::SimulatedObjectSetup>>(*scene, group);
                }
            }
        }

        const AZStd::string& manifestFilename = scene->GetManifestFilename();
        const bool fileExisted = AZ::IO::FileIOBase::GetInstance()->Exists(manifestFilename.c_str());

        // Source Control: Checkout file.
        if (fileExisted)
        {
            using ApplicationBus = AzToolsFramework::ToolsApplicationRequestBus;
            bool checkoutResult = false;
            ApplicationBus::BroadcastResult(checkoutResult, &ApplicationBus::Events::RequestEditForFileBlocking, manifestFilename.c_str(), "Checking out manifest from source control.", []([[maybe_unused]] int& current, [[maybe_unused]] int& max) {});
            if (!checkoutResult)
            {
                AZ_Error("EMotionFX", false, "Cannot checkout file '%s' from source control.", manifestFilename.c_str());
                return false;
            }
        }

        const bool saveResult = manifest.SaveToFile(manifestFilename.c_str());
        if (saveResult)
        {
            actor->SetDirtyFlag(false);
        }

        // Source Control: Add file in case it did not exist before (when saving it the first time).
        if (saveResult && !fileExisted)
        {
            using ApplicationBus = AzToolsFramework::ToolsApplicationRequestBus;
            bool checkoutResult = false;
            ApplicationBus::BroadcastResult(checkoutResult, &ApplicationBus::Events::RequestEditForFileBlocking, manifestFilename.c_str(), "Adding manifest to source control.", []([[maybe_unused]] int& current, [[maybe_unused]] int& max) {});
            AZ_Error("EMotionFX", checkoutResult, "Cannot add file '%s' to source control.", manifestFilename.c_str());
        }

        return saveResult;
    }


    bool CommandSaveActorAssetInfo::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        MCORE_UNUSED(parameters);
        MCORE_UNUSED(outResult);
        return true;
    }


    void CommandSaveActorAssetInfo::InitSyntax()
    {
        GetSyntax().ReserveParameters(1);
        GetSyntax().AddRequiredParameter("actorID", "The id of the actor to save.", MCore::CommandSyntax::PARAMTYPE_INT);
    }


    const char* CommandSaveActorAssetInfo::GetDescription() const
    {
        return "Save the .assetinfo of a actor.";
    }


    //--------------------------------------------------------------------------------
    // CommandSaveMotionAssetInfo
    //--------------------------------------------------------------------------------
    CommandSaveMotionAssetInfo::CommandSaveMotionAssetInfo(MCore::Command* orgCommand)
        : MCore::Command("SaveMotionAssetInfo", orgCommand)
    {
    }


    CommandSaveMotionAssetInfo::~CommandSaveMotionAssetInfo()
    {
    }


    bool CommandSaveMotionAssetInfo::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        const int32 motionId = parameters.GetValueAsInt("motionID", this);
        outResult.clear();

        EMotionFX::Motion* motion = EMotionFX::GetMotionManager().FindMotionByID(motionId);
        if (motion == nullptr)
        {
            outResult = AZStd::string::format("Motion .assetinfo cannot be saved. Motion with id '%i' does not exist.", motionId);
            return false;
        }

        AZStd::string productFilename = motion->GetFileName();

        // Get the group name from the product filename (assuming the product filename is the group name).
        AZStd::string groupName;
        if (!AzFramework::StringFunc::Path::GetFileName(productFilename.c_str(), groupName))
        {
            outResult = AZStd::string::format("Motion .assetinfo cannot be saved. Cannot get product name from asset cache file '%s'.", productFilename.c_str());
            return false;
        }

        // Get the full file path for the asset source file based on the product filename.
        bool fullPathFound = false;
        AZStd::string sourceAssetFilename;
        EBUS_EVENT_RESULT(fullPathFound, AzToolsFramework::AssetSystemRequestBus, GetFullSourcePathFromRelativeProductPath, productFilename, sourceAssetFilename);

        // Load the manifest from disk.
        AZStd::shared_ptr<AZ::SceneAPI::Containers::Scene> scene;
        AZ::SceneAPI::Events::SceneSerializationBus::BroadcastResult(scene, &AZ::SceneAPI::Events::SceneSerializationBus::Events::LoadScene, sourceAssetFilename, AZ::Uuid::CreateNull(), "");
        if (!scene)
        {
            AZ_Error("EMotionFX", false, "Unable to save meta data to manifest due to failed scene loading.");
            return false;
        }

        AZ::SceneAPI::Containers::SceneManifest& manifest = scene->GetManifest();
        auto values = manifest.GetValueStorage();
        auto groupView = AZ::SceneAPI::Containers::MakeDerivedFilterView<EMotionFX::Pipeline::Group::MotionGroup>(values);
        for (EMotionFX::Pipeline::Group::MotionGroup& group : groupView)
        {
            // Non-case sensitive group name comparison. Product filenames are lower case only and might mismatch casing of the entered group name.
            if (AzFramework::StringFunc::Equal(group.GetName().c_str(), groupName.c_str()))
            {
                // Remove legacy meta data rule.
                EMotionFX::Pipeline::Rule::RemoveRuleFromGroup<EMotionFX::Pipeline::Rule::MetaDataRule, const AZStd::vector<MCore::Command*>>(*scene, group);

                // Add motion meta data.
                auto motionMetaData = AZStd::make_shared<EMotionFX::Pipeline::Rule::MotionMetaData>(motion->GetMotionExtractionFlags(), motion->GetEventTable());
                EMotionFX::Pipeline::Rule::SaveToGroup<EMotionFX::Pipeline::Rule::MotionMetaDataRule, AZStd::shared_ptr<EMotionFX::Pipeline::Rule::MotionMetaData>>(*scene, group, motionMetaData);

                // Save RootMotionExtractionRule
                if (auto rootMotionData = motion->GetRootMotionExtractionData())
                {
                    EMotionFX::Pipeline::Rule::SaveToGroup<
                        EMotionFX::Pipeline::Rule::RootMotionExtractionRule,
                        AZStd::shared_ptr<EMotionFX::RootMotionExtractionData>>(*scene, group, rootMotionData);
                }
                else
                {
                    EMotionFX::Pipeline::Rule::RemoveRuleFromGroup<
                        EMotionFX::Pipeline::Rule::RootMotionExtractionRule,
                        AZStd::shared_ptr<EMotionFX::RootMotionExtractionData>>(*scene, group);
                }
            }
        }

        const AZStd::string& manifestFilename = scene->GetManifestFilename();
        const bool fileExisted = AZ::IO::FileIOBase::GetInstance()->Exists(manifestFilename.c_str());

        // Source Control: Checkout file.
        if (fileExisted)
        {
            using ApplicationBus = AzToolsFramework::ToolsApplicationRequestBus;
            bool checkoutResult = false;
            ApplicationBus::BroadcastResult(checkoutResult, &ApplicationBus::Events::RequestEditForFileBlocking, manifestFilename.c_str(), "Checking out manifest from source control.", []([[maybe_unused]] int& current, [[maybe_unused]] int& max) {});
            if (!checkoutResult)
            {
                AZ_Error("EMotionFX", false, "Cannot checkout file '%s' from source control.", manifestFilename.c_str());
                return false;
            }
        }

        const bool saveResult = manifest.SaveToFile(manifestFilename.c_str());
        if (saveResult)
        {
            motion->SetDirtyFlag(false);
        }

        // Source Control: Add file in case it did not exist before (when saving it the first time).
        if (saveResult && !fileExisted)
        {
            using ApplicationBus = AzToolsFramework::ToolsApplicationRequestBus;
            bool checkoutResult = false;
            ApplicationBus::BroadcastResult(checkoutResult, &ApplicationBus::Events::RequestEditForFileBlocking, manifestFilename.c_str(), "Adding manifest to source control.", []([[maybe_unused]] int& current, [[maybe_unused]] int& max) {});
            AZ_Error("EMotionFX", checkoutResult, "Cannot add file '%s' to source control.", manifestFilename.c_str());
        }

        return saveResult;
    }


    bool CommandSaveMotionAssetInfo::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        MCORE_UNUSED(parameters);
        MCORE_UNUSED(outResult);
        return true;
    }


    void CommandSaveMotionAssetInfo::InitSyntax()
    {
        GetSyntax().ReserveParameters(1);
        GetSyntax().AddRequiredParameter("motionID", "The id of the motion to save.", MCore::CommandSyntax::PARAMTYPE_INT);
    }


    const char* CommandSaveMotionAssetInfo::GetDescription() const
    {
        return "Save the .assetinfo of a motion.";
    }



    //--------------------------------------------------------------------------------
    // CommandSaveMotionSet
    //--------------------------------------------------------------------------------
    CommandSaveMotionSet::CommandSaveMotionSet(MCore::Command* orgCommand)
        : SourceControlCommand("SaveMotionSet", orgCommand)
    {
    }


    CommandSaveMotionSet::~CommandSaveMotionSet()
    {
    }


    void CommandSaveMotionSet::RecursiveSetDirtyFlag(EMotionFX::MotionSet* motionSet, bool dirtyFlag)
    {
        motionSet->SetDirtyFlag(dirtyFlag);

        const size_t numChildSets = motionSet->GetNumChildSets();
        for (size_t i = 0; i < numChildSets; ++i)
        {
            EMotionFX::MotionSet* childSet = motionSet->GetChildSet(i);
            RecursiveSetDirtyFlag(childSet, dirtyFlag);
        }
    }

    CommandEditorLoadAnimGraph::CommandEditorLoadAnimGraph(MCore::Command* orgCommand)
        : CommandSystem::CommandLoadAnimGraph(orgCommand)
    {
        m_relocateFilenameFunction = RelocateFilename;
    }

    void CommandEditorLoadAnimGraph::RelocateFilename(AZStd::string& filename)
    {
        GetMainWindow()->GetFileManager()->RelocateToAssetSourceFolder(filename);
    }

    CommandEditorLoadMotionSet::CommandEditorLoadMotionSet(MCore::Command* orgCommand)
        : CommandSystem::CommandLoadMotionSet(orgCommand)
    {
        m_relocateFilenameFunction = CommandEditorLoadAnimGraph::RelocateFilename;
    }

    bool CommandSaveMotionSet::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        const int32 motionSetID = parameters.GetValueAsInt("motionSetID", this);

        EMotionFX::MotionSet* motionSet = EMotionFX::GetMotionManager().FindMotionSetByID(motionSetID);
        if (!motionSet)
        {
            outResult = AZStd::string::format("Motion set cannot be saved. Motion set with id '%i' does not exist.", motionSetID);
            return false;
        }

        AZStd::string filename;
        parameters.GetValue("filename", this, filename);
        // Resolve the filename if it starts with a path alias
        if (filename.starts_with('@'))
        {
            filename = EMotionFX::EMotionFXManager::ResolvePath(filename.c_str());
        }

        // Avoid saving to asset cache folder.
        if (!GetMainWindow()->GetFileManager()->RelocateToAssetSourceFolder(filename))
        {
            outResult = AZStd::string::format("Motion set cannot be saved. Unable to find source asset path for (%s)", filename.c_str());
            return false;
        }
        EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePathKeepCase, filename);

        if (!CheckOutFile(parameters, filename.c_str(), outResult, false))
        {
            return false;
        }

        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (!context)
        {
            AZ_Error("EMotionFX", false, "Can't get serialize context from component application.");
            return false;
        }

        const bool saveResult = motionSet->SaveToFile(filename, context);

        if (saveResult)
        {
            GetMainWindow()->GetFileManager()->SourceAssetChanged(filename);

            // Add file in case it did not exist before (when saving it the first time).
            if (!CheckOutFile(parameters, filename.c_str(), outResult, true))
            {
                return false;
            }
        }

        // Set the new filename.
        if (parameters.GetValueAsBool("updateFilename", this))
        {
            motionSet->SetFilename(filename.c_str());
        }

        // Reset all dirty flags as we saved it.
        if (parameters.GetValueAsBool("updateDirtyFlag", this))
        {
            RecursiveSetDirtyFlag(motionSet, false);
        }
        return true;
    }


    bool CommandSaveMotionSet::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        MCORE_UNUSED(parameters);
        MCORE_UNUSED(outResult);
        return true;
    }


    void CommandSaveMotionSet::InitSyntax()
    {
        GetSyntax().ReserveParameters(5);
        SourceControlCommand::InitSyntax();
        GetSyntax().AddRequiredParameter("filename",    "The filename of the motion set file.",             MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddRequiredParameter("motionSetID", "The id of the motion set to save.",                MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddParameter("updateFilename",      "True to update the filename of the motion set.",   MCore::CommandSyntax::PARAMTYPE_BOOLEAN,    "true");
        GetSyntax().AddParameter("updateDirtyFlag",     "True to update the dirty flag of the motion set.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN,    "true");
    }


    const char* CommandSaveMotionSet::GetDescription() const
    {
        return "Save the given motion set to disk.";
    }


    //-------------------------------------------------------------------------------------
    // Save the given anim graph
    //-------------------------------------------------------------------------------------
    CommandSaveAnimGraph::CommandSaveAnimGraph(MCore::Command* orgCommand)
        : SourceControlCommand("SaveAnimGraph", orgCommand)
    {
    }


    CommandSaveAnimGraph::~CommandSaveAnimGraph()
    {
    }


    bool CommandSaveAnimGraph::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // Get the anim graph index inside the anim graph manager and check if it is in range.
        const int32 animGraphIndex = parameters.GetValueAsInt("index", -1);
        if (animGraphIndex == -1 || animGraphIndex >= (int32)EMotionFX::GetAnimGraphManager().GetNumAnimGraphs())
        {
            outResult = "Cannot save anim graph. Anim graph index is not valid.";
            return false;
        }

        AZStd::string filename;
        parameters.GetValue("filename", this, filename);
        // Resolve the filename if it starts with a path alias
        if (filename.starts_with('@'))
        {
            filename = EMotionFX::EMotionFXManager::ResolvePath(filename.c_str());
        }

        // Avoid saving to asset cache folder.
        if (!GetMainWindow()->GetFileManager()->RelocateToAssetSourceFolder(filename))
        {
            outResult = AZStd::string::format("Animation graph cannot be saved. Unable to find source asset path for (%s)", filename.c_str());
            return false;
        }
        EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePathKeepCase, filename);

        AZStd::string companyName;
        parameters.GetValue("companyName", this, companyName);

        // get the anim graph from the manager
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().GetAnimGraph(animGraphIndex);

        if (!CheckOutFile(parameters, filename.c_str(), outResult, false))
        {
            return false;
        }

        AZ::SerializeContext* context = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (!context)
        {
            AZ_Error("EMotionFX", false, "Can't get serialize context from component application.");
            return false;
        }

        const bool saveResult = animGraph->SaveToFile(filename, context);
        if (saveResult)
        {
            if (parameters.GetValueAsBool("updateFilename", this))
            {
                animGraph->SetFileName(filename.c_str());
            }

            if (parameters.GetValueAsBool("updateDirtyFlag", this))
            {
                animGraph->SetDirtyFlag(false);
            }

            GetMainWindow()->GetFileManager()->SourceAssetChanged(filename);

            // Add file in case it did not exist before (when saving it the first time).
            if (!CheckOutFile(parameters, filename.c_str(), outResult, true))
            {
                return false;
            }
        }

        return saveResult;
    }


    bool CommandSaveAnimGraph::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        MCORE_UNUSED(parameters);
        MCORE_UNUSED(outResult);
        return true;
    }


    void CommandSaveAnimGraph::InitSyntax()
    {
        GetSyntax().ReserveParameters(6);
        SourceControlCommand::InitSyntax();
        GetSyntax().AddRequiredParameter("filename",    "The filename of the anim graph file.", MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddRequiredParameter("index",       "The index inside the anim graph manager of the anim graph to save.", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddParameter("updateFilename",      "True to update the filename of the anim graph.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "true");
        GetSyntax().AddParameter("updateDirtyFlag",     "True to update the dirty flag of the anim graph.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "true");
        GetSyntax().AddParameter("companyName",         "The company name to which this anim graph belongs to.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
    }


    const char* CommandSaveAnimGraph::GetDescription() const
    {
        return "This command saves a anim graph to the given file.";
    }


    //--------------------------------------------------------------------------------
    // CommandSaveWorkspace
    //--------------------------------------------------------------------------------
    CommandSaveWorkspace::CommandSaveWorkspace(MCore::Command* orgCommand)
        : MCore::Command("SaveWorkspace", orgCommand)
    {
    }


    CommandSaveWorkspace::~CommandSaveWorkspace()
    {
    }


    bool CommandSaveWorkspace::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        MCORE_UNUSED(outResult);

        const bool skipSourceControl = GetManager()->GetSkipSourceControlCommands();

        AZStd::string filename;
        parameters.GetValue("filename", this, filename);
        // Resolve the filename if it starts with a path alias
        if (filename.starts_with('@'))
        {
            filename = EMotionFX::EMotionFXManager::ResolvePath(filename.c_str());
        }

        // Avoid saving to asset cache folder.
        if (!GetMainWindow()->GetFileManager()->RelocateToAssetSourceFolder(filename))
        {
            outResult = AZStd::string::format("Workspace cannot be saved. Unable to find source asset path for (%s)", filename.c_str());
            return false;
        }
        EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePathKeepCase, filename);

        const bool fileExisted = AZ::IO::FileIOBase::GetInstance()->Exists(filename.c_str());

        // Source Control: Checkout file.
        if (fileExisted && !skipSourceControl)
        {
            using ApplicationBus = AzToolsFramework::ToolsApplicationRequestBus;
            bool checkoutResult = false;
            ApplicationBus::BroadcastResult(checkoutResult, 
                &ApplicationBus::Events::RequestEditForFileBlocking, 
                filename.c_str(), 
                "Checking out workspace from source control.", 
                ApplicationBus::Events::RequestEditProgressCallback());

            if (!checkoutResult)
            {
                outResult = AZStd::string::format("Cannot check out file '%s' from source control.", filename.c_str());
                AZ_Error("EMotionFX", false, outResult.c_str());
                return false;
            }
        }

        EMStudio::Workspace* workspace = GetManager()->GetWorkspace();
        const bool saveResult = workspace->Save(filename.c_str());
        if (saveResult)
        {
            workspace->SetDirtyFlag(false);
        }

        // Source Control: Add file in case it did not exist before (when saving it the first time).
        if (saveResult && !fileExisted && !skipSourceControl)
        {
            using ApplicationBus = AzToolsFramework::ToolsApplicationRequestBus;
            bool checkoutResult = false;
            ApplicationBus::BroadcastResult(checkoutResult, 
                &ApplicationBus::Events::RequestEditForFileBlocking, 
                filename.c_str(), 
                "Adding workspace to source control.", 
                ApplicationBus::Events::RequestEditProgressCallback());

            if (!checkoutResult)
            {
                outResult = AZStd::string::format("Cannot add file '%s' to source control.", filename.c_str());
                AZ_Error("EMotionFX", false, outResult.c_str());
                return false;
            }
        }

        return saveResult;
    }


    bool CommandSaveWorkspace::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        MCORE_UNUSED(parameters);
        MCORE_UNUSED(outResult);
        return true;
    }


    void CommandSaveWorkspace::InitSyntax()
    {
        GetSyntax().ReserveParameters(1);
        GetSyntax().AddRequiredParameter("filename", "The filename of the workspace.", MCore::CommandSyntax::PARAMTYPE_STRING);
    }


    const char* CommandSaveWorkspace::GetDescription() const
    {
        return "This command save the workspace.";
    }
} // namespace EMStudio
