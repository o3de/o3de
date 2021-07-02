/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include the required headers
#include "ImporterCommands.h"
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/MotionSet.h>
#include <EMotionFX/Source/Attachment.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Exporters/ExporterLib/Exporter/Exporter.h>
#include "CommandManager.h"
#include <AzFramework/API/ApplicationAPI.h>


namespace CommandSystem
{
    //--------------------------------------------------------------------------------
    // ImportActor
    //--------------------------------------------------------------------------------

    // constructor
    CommandImportActor::CommandImportActor(MCore::Command* orgCommand)
        : MCore::Command("ImportActor", orgCommand)
    {
        mPreviouslyUsedID = MCORE_INVALIDINDEX32;
    }


    // destructor
    CommandImportActor::~CommandImportActor()
    {
    }


    // execute
    bool CommandImportActor::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // get the actor id from the parameters
        uint32 actorID = MCORE_INVALIDINDEX32;
        if (parameters.CheckIfHasParameter("actorID"))
        {
            actorID = parameters.GetValueAsInt("actorID", MCORE_INVALIDINDEX32);
            if (EMotionFX::GetActorManager().FindActorByID(actorID))
            {
                outResult = AZStd::string::format("Cannot import actor. Actor ID %i is already in use.", actorID);
                return false;
            }
        }

        // get the filename of the actor
        AZStd::string filename;
        parameters.GetValue("filename", "", filename);
        
        EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePathKeepCase, filename);
        // Resolve the filename if it starts with a path alias
        if (filename.starts_with('@'))
        {
            filename = EMotionFX::EMotionFXManager::ResolvePath(filename.c_str());
        }

        // check if we have already loaded the actor
        EMotionFX::Actor* actorFromManager = EMotionFX::GetActorManager().FindActorByFileName(filename.c_str());
        if (actorFromManager)
        {
            AZStd::to_string(outResult, actorFromManager->GetID());
            return true;
        }

        // init the settings
        EMotionFX::Importer::ActorSettings settings;

        // extract default values from the command syntax automatically, if they aren't specified explicitly
        settings.mLoadLimits                    = parameters.GetValueAsBool("loadLimits",           this);
        settings.mLoadMorphTargets              = parameters.GetValueAsBool("loadMorphTargets",     this);
        settings.mLoadSkeletalLODs              = parameters.GetValueAsBool("loadSkeletalLODs",     this);
        settings.mDualQuatSkinning              = parameters.GetValueAsBool("dualQuatSkinning",     this);

        // try to load the actor
        AZStd::shared_ptr<EMotionFX::Actor> actor {EMotionFX::GetImporter().LoadActor(filename.c_str(), &settings)};
        if (!actor)
        {
            outResult = AZStd::string::format("Failed to load actor from '%s'. File may not exist at this path or may have incorrect permissions", filename.c_str());
            return false;
        }

        // Because the actor is directly loaded from disk (without going through an actor asset), we need to ask for a blocking
        // load for the asset that actor is depend on.
        actor->Finalize(EMotionFX::Actor::LoadRequirement::RequireBlockingLoad);

        // set the actor id in case we have specified it as parameter
        if (actorID != MCORE_INVALIDINDEX32)
        {
            actor->SetID(actorID);
        }

        // in case we are in a redo call assign the previously used id
        if (mPreviouslyUsedID != MCORE_INVALIDINDEX32)
        {
            actor->SetID(mPreviouslyUsedID);
        }
        mPreviouslyUsedID = actor->GetID();

        // select the actor automatically
        if (parameters.GetValueAsBool("autoSelect", this))
        {
            GetCommandManager()->ExecuteCommandInsideCommand(AZStd::string::format("Select -actorID %i", actor->GetID()).c_str(), outResult);
        }


        // mark the workspace as dirty
        mOldWorkspaceDirtyFlag = GetCommandManager()->GetWorkspaceDirtyFlag();
        GetCommandManager()->SetWorkspaceDirtyFlag(true);

        // return the id of the newly created actor
        AZStd::to_string(outResult, actor->GetID());

        EMotionFX::GetActorManager().RegisterActor(AZStd::move(actor));

        return true;
    }


    // undo the command
    bool CommandImportActor::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // get the actor ID
        uint32 actorID = parameters.GetValueAsInt("actorID", MCORE_INVALIDINDEX32);
        if (actorID == MCORE_INVALIDINDEX32)
        {
            actorID = mPreviouslyUsedID;
        }

        // check if we have to unselect the actors created by this command
        const bool unselect = parameters.GetValueAsBool("autoSelect", this);
        if (unselect)
        {
            GetCommandManager()->ExecuteCommandInsideCommand(AZStd::string::format("Unselect -actorID %i", actorID).c_str(), outResult);
        }

        // find the actor based on the given id
        AZStd::shared_ptr<EMotionFX::Actor> actor = EMotionFX::GetActorManager().FindSharedActorByID(actorID);
        if (actor == nullptr)
        {
            outResult = AZStd::string::format("Cannot remove actor. Actor ID %i is not valid.", actorID);
            return false;
        }

        EMotionFX::GetActorManager().UnregisterActor(actor);

        // update our render actors
        AZStd::string updateRenderActorsResult;
        GetCommandManager()->ExecuteCommandInsideCommand("UpdateRenderActors", updateRenderActorsResult);

        // restore the workspace dirty flag
        GetCommandManager()->SetWorkspaceDirtyFlag(mOldWorkspaceDirtyFlag);

        return true;
    }


    // init the syntax of the command
    void CommandImportActor::InitSyntax()
    {
        // required
        GetSyntax().ReserveParameters(17);
        GetSyntax().AddRequiredParameter("filename", "The filename of the actor file to load.", MCore::CommandSyntax::PARAMTYPE_STRING);

        // optional parameters
        GetSyntax().AddParameter("actorID",             "The identification number to give the actor. In case this parameter is not specified the actor manager will automatically set a unique id to the actor.", MCore::CommandSyntax::PARAMTYPE_INT, "-1");
        GetSyntax().AddParameter("loadMeshes",          "Load 3D mesh geometry or not.",                                        MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "true");
        GetSyntax().AddParameter("loadTangents",        "Load vertex tangents or not.",                                         MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "true");
        GetSyntax().AddParameter("loadLimits",          "Load node limits or not.",                                             MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "true");
        GetSyntax().AddParameter("loadGeomLods",        "Load geometry LOD levels or not.",                                     MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "true");
        GetSyntax().AddParameter("loadMorphTargets",    "Load morph targets or not.",                                           MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "true");
        GetSyntax().AddParameter("loadCollisionMeshes", "Load collision meshes or not.",                                        MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "true");
        GetSyntax().AddParameter("loadSkeletalLODs",    "Load skeletal LOD levels.",                                            MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "true");
        GetSyntax().AddParameter("dualQuatSkinning",    "Enable software skinning using dual quaternions.",                     MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "false");
        GetSyntax().AddParameter("loadSkinningInfo",    "Load skinning information (bone influences) or not.",                  MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "true");
        GetSyntax().AddParameter("loadMaterialLayers",  "Load standard material layers (textures) or not.",                     MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "true");
        GetSyntax().AddParameter("autoGenTangents",     "Automatically generate tangents when they are not present or not.",    MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "true");
        GetSyntax().AddParameter("autoSelect",          "Set the current selected actor to the newly loaded actor or leave selection as it was before.",        MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "true");
    }


    // get the description
    const char* CommandImportActor::GetDescription() const
    {
        return "This command can be used to import EMotion FX actor files. Actor files can represent 3D objects and characters. They can for example contain full 3D character meshes linked to a hierarchy of bones or a complete game level or just a hierarchy of objects.";
    }

    //--------------------------------------------------------------------------------
    // ImportMotion
    //--------------------------------------------------------------------------------

    // constructor
    CommandImportMotion::CommandImportMotion(MCore::Command* orgCommand)
        : MCore::Command("ImportMotion", orgCommand)
    {
        mOldMotionID = MCORE_INVALIDINDEX32;
    }


    // destructor
    CommandImportMotion::~CommandImportMotion()
    {
    }


    // execute
    bool CommandImportMotion::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // get the motion id from the parameters
        uint32 motionID = MCORE_INVALIDINDEX32;
        if (parameters.CheckIfHasParameter("motionID"))
        {
            motionID = parameters.GetValueAsInt("motionID", MCORE_INVALIDINDEX32);
            if (EMotionFX::GetMotionManager().FindMotionByID(motionID))
            {
                outResult = AZStd::string::format("Cannot import motion. Motion ID %i is already in use.", motionID);
                return false;
            }
        }

        // get the filename
        AZStd::string filename;
        parameters.GetValue("filename", "", &filename);
        
        EBUS_EVENT(AzFramework::ApplicationRequests::Bus, NormalizePathKeepCase, filename);

        // Resolve any path aliases as part of the filename parameter
        if (filename.starts_with('@'))
        {
            filename = EMotionFX::EMotionFXManager::ResolvePath(filename.c_str());
        }

        AZStd::string extension;
        AzFramework::StringFunc::Path::GetExtension(filename.c_str(), extension, false /* include dot */);

        // check if we have already loaded the motion
        if (EMotionFX::GetMotionManager().FindMotionByFileName(filename.c_str()))
        {
            outResult = AZStd::string::format("Motion '%s' has already been loaded. Skipping.", filename.c_str());
            return true;
        }

        // check if we are dealing with a motion
        EMotionFX::Motion* motion = nullptr;
        if (AzFramework::StringFunc::Equal(extension.c_str(), "motion", false /* no case */))
        {
            EMotionFX::Importer::MotionSettings settings;
            settings.mLoadMotionEvents = parameters.GetValueAsBool("loadMotionEvents", this);
            motion = EMotionFX::GetImporter().LoadMotion(filename.c_str(), &settings);
        }

        // check if the motion is invalid
        if (motion == nullptr)
        {
            outResult = AZStd::string::format("Failed to load motion from file '%s'.", filename.c_str());
            return false;
        }

        // set the motion id in case we have specified it as parameter
        if (motionID != MCORE_INVALIDINDEX32)
        {
            motion->SetID(motionID);
        }

        // in case we are in a redo call assign the previously used id
        if (mOldMotionID != MCORE_INVALIDINDEX32)
        {
            motion->SetID(mOldMotionID);
        }
        mOldMotionID = motion->GetID();
        mOldFileName = motion->GetFileName();

        // set the motion name
        AZStd::string motionName;
        AzFramework::StringFunc::Path::GetFileName(filename.c_str(), motionName);
        motion->SetName(motionName.c_str());

        // select the motion automatically
        if (parameters.GetValueAsBool("autoSelect", this))
        {
            GetCommandManager()->GetCurrentSelection().AddMotion(motion);
        }

        // mark the workspace as dirty
        mOldWorkspaceDirtyFlag = GetCommandManager()->GetWorkspaceDirtyFlag();
        GetCommandManager()->SetWorkspaceDirtyFlag(true);

        // reset the dirty flag
        motion->SetDirtyFlag(false);
        return true;
    }


    // undo the command
    bool CommandImportMotion::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        MCORE_UNUSED(parameters);

        // execute the group command
        AZStd::string commandString;
        commandString = AZStd::string::format("RemoveMotion -filename \"%s\"", mOldFileName.c_str());
        bool result = GetCommandManager()->ExecuteCommandInsideCommand(commandString.c_str(), outResult);

        // restore the workspace dirty flag
        GetCommandManager()->SetWorkspaceDirtyFlag(mOldWorkspaceDirtyFlag);

        return result;
    }


    // init the syntax of the command
    void CommandImportMotion::InitSyntax()
    {
        // required
        GetSyntax().ReserveParameters(7);
        GetSyntax().AddRequiredParameter("filename", "The filename of the motion file to load.", MCore::CommandSyntax::PARAMTYPE_STRING);

        // optional parameters
        GetSyntax().AddParameter("motionID",            "The identification number to give the motion. In case this parameter is not specified the motion will automatically get a unique id.",                 MCore::CommandSyntax::PARAMTYPE_INT, "-1");
        GetSyntax().AddParameter("loadMotionEvents",    "Set to false if you wish to disable loading of motion events.",                                                                                        MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "true");
        GetSyntax().AddParameter("autoRegisterEvents",  "Set to true if you want to automatically register new motion event types.",                                                                            MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "true");
        GetSyntax().AddParameter("autoSelect",          "Set the current selected actor to the newly loaded actor or leave selection as it was before.",                                                        MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "false");
    }


    // get the description
    const char* CommandImportMotion::GetDescription() const
    {
        return "This command can be used to import EMotion FX motion files. The command can load skeletal as well as morph target motions.";
    }

} // namespace CommandSystem
