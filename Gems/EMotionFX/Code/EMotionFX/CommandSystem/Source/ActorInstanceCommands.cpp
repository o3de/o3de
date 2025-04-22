/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ActorInstanceCommands.h"

#include <AzCore/Serialization/Locale.h>

#include <EMotionFX/Source/ActorManager.h>
#include <MCore/Source/LogManager.h>
#include <MCore/Source/StringConversions.h>
#include "CommandManager.h"

namespace CommandSystem
{
    //--------------------------------------------------------------------------------
    // CreateActorInstance
    //--------------------------------------------------------------------------------

    // constructor
    CommandCreateActorInstance::CommandCreateActorInstance(MCore::Command* orgCommand)
        : MCore::Command("CreateActorInstance", orgCommand)
    {
        m_previouslyUsedId = MCORE_INVALIDINDEX32;
    }


    // destructor
    CommandCreateActorInstance::~CommandCreateActorInstance()
    {
    }


    // execute the command
    bool CommandCreateActorInstance::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        EMotionFX::Actor* actor;
        if (parameters.CheckIfHasParameter("actorID"))
        {
            const uint32 actorID = parameters.GetValueAsInt("actorID", MCORE_INVALIDINDEX32);

            // find the actor based on the given id
            actor = EMotionFX::GetActorManager().FindActorByID(actorID);
            if (actor == nullptr)
            {
                outResult = AZStd::string::format("Cannot create actor instance. Actor ID %i is not valid.", actorID);
                return false;
            }
        }
        else
        {
            // check if there is any actor selected at all
            SelectionList& selection = GetCommandManager()->GetCurrentSelection();
            if (selection.GetNumSelectedActors() == 0)
            {
                outResult = "No actor has been selected, please select one first.";
                return false;
            }

            // get the first selected actor
            actor = selection.GetActor(0);
        }

        // get the other parameter info
        const float scaleX          = parameters.GetValueAsFloat("xScale",  this);
        const float scaleY          = parameters.GetValueAsFloat("yScale",  this);
        const float scaleZ          = parameters.GetValueAsFloat("zScale",  this);
        const AZ::Vector4 rotV4     = parameters.GetValueAsVector4("rot",   this);
        AZ::Vector3 scale(scaleX, scaleY, scaleZ);

        // validate the scale values, don't allow zero or negative scaling
        if (MCore::InRange(scaleX, 0.0001f, 10000.0f) == false ||
            MCore::InRange(scaleY, 0.0001f, 10000.0f) == false ||
            MCore::InRange(scaleZ, 0.0001f, 10000.0f) == false)
        {
            AZ_Warning("EMotionFX", false, "The scale values must be between 0.0001 and 10000. Resetting scale back to 1.0.");
            scale = AZ::Vector3::CreateOne();
        }

        AZ::Quaternion rot(rotV4.GetX(), rotV4.GetY(), rotV4.GetZ(), rotV4.GetW());

        // check if we have to select the new actor instances created by this command automatically
        const bool select = parameters.GetValueAsBool("autoSelect", this);

        // create the instance
        EMotionFX::ActorInstance* newInstance = EMotionFX::ActorInstance::Create(actor);
        newInstance->UpdateVisualizeScale();

        // get the actor instance ID to give the actor instance
        uint32 actorInstanceID = MCORE_INVALIDINDEX32;
        if (parameters.CheckIfHasParameter("actorInstanceID"))
        {
            actorInstanceID = parameters.GetValueAsInt("actorInstanceID", MCORE_INVALIDINDEX32);
            if (EMotionFX::GetActorManager().FindActorInstanceByID(actorInstanceID))
            {
                outResult = AZStd::string::format("Cannot create actor instance. Actor instance ID %i is already in use.", actorInstanceID);
                return false;
            }
        }

        // set the actor instance id in case we have specified it as parameter
        if (actorInstanceID != MCORE_INVALIDINDEX32)
        {
            newInstance->SetID(actorInstanceID);
        }

        // in case of redoing the command set the previously used id
        if (m_previouslyUsedId != MCORE_INVALIDINDEX32)
        {
            newInstance->SetID(m_previouslyUsedId);
        }

        m_previouslyUsedId = newInstance->GetID();

        // setup the position, rotation and scale
        AZ::Vector3 newPos = newInstance->GetLocalSpaceTransform().m_position;
        if (parameters.CheckIfHasParameter("xPos"))
        {
            newPos.SetX(parameters.GetValueAsFloat("xPos", this));
        }
        if (parameters.CheckIfHasParameter("yPos"))
        {
            newPos.SetY(parameters.GetValueAsFloat("yPos", this));
        }
        if (parameters.CheckIfHasParameter("zPos"))
        {
            newPos.SetZ(parameters.GetValueAsFloat("zPos", this));
        }
        newInstance->SetLocalSpacePosition(newPos);
        newInstance->SetLocalSpaceRotation(rot);

        EMFX_SCALECODE
        (
            newInstance->SetLocalSpaceScale(scale);
        )

        // add the actor instance to the selection
        if (select)
        {
            GetCommandManager()->ExecuteCommandInsideCommand(AZStd::string::format("Select -actorInstanceID %u", newInstance->GetID()).c_str(), outResult);

            if (EMotionFX::GetActorManager().GetNumActorInstances() == 1 && GetCommandManager()->GetLockSelection() == false)
            {
                GetCommandManager()->ExecuteCommandInsideCommand("ToggleLockSelection", outResult);
            }

            if (EMotionFX::GetActorManager().GetNumActorInstances() > 1 && GetCommandManager()->GetLockSelection())
            {
                GetCommandManager()->ExecuteCommandInsideCommand("ToggleLockSelection", outResult);
            }
        }

        // mark the workspace as dirty
        m_oldWorkspaceDirtyFlag = GetCommandManager()->GetWorkspaceDirtyFlag();
        GetCommandManager()->SetWorkspaceDirtyFlag(true);

        // return the id of the newly created actor instance
        AZStd::to_string(outResult, newInstance->GetID());
        return true;
    }


    // undo the command
    bool CommandCreateActorInstance::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // check if we have to unselect the actors created by this command
        const bool unselect = parameters.GetValueAsBool("autoSelect", this);

        // get the actor instance ID
        uint32 actorInstanceID = parameters.GetValueAsInt("actorInstanceID", MCORE_INVALIDINDEX32);
        if (actorInstanceID == MCORE_INVALIDINDEX32)
        {
            actorInstanceID = m_previouslyUsedId;
        }

        // find the actor intance based on the given id
        EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().FindActorInstanceByID(actorInstanceID);
        if (actorInstance == nullptr)
        {
            outResult = AZStd::string::format("Cannot undo create actor instance command. Actor instance ID %i is not valid.", actorInstanceID);
            return false;
        }

        // remove the actor instance from the selection
        if (unselect)
        {
            GetCommandManager()->ExecuteCommandInsideCommand(AZStd::string::format("Unselect -actorInstanceID %i", actorInstanceID), outResult);

            if (EMotionFX::GetActorManager().GetNumActorInstances() == 1 && GetCommandManager()->GetLockSelection() == false)
            {
                GetCommandManager()->ExecuteCommandInsideCommand("ToggleLockSelection", outResult);
            }

            if (EMotionFX::GetActorManager().GetNumActorInstances() > 1 && GetCommandManager()->GetLockSelection())
            {
                GetCommandManager()->ExecuteCommandInsideCommand("ToggleLockSelection", outResult);
            }
        }

        // restore the workspace dirty flag
        GetCommandManager()->SetWorkspaceDirtyFlag(m_oldWorkspaceDirtyFlag);

        // get rid of the actor instance
        if (actorInstance)
        {
            actorInstance->Destroy();
        }
        return true;
    }


    // init the syntax of the command
    void CommandCreateActorInstance::InitSyntax()
    {
        // optional parameters
        GetSyntax().ReserveParameters(13);
        GetSyntax().AddParameter("actorID",         "The identification number of the actor for which we want to create an actor instance.",                                                                                                            MCore::CommandSyntax::PARAMTYPE_INT,        "-1");
        GetSyntax().AddParameter("actorInstanceID", "The actor instance identification number to give the new actor instance. In case this parameter is not specified the IDGenerator will automatically assign a unique ID to the actor instance.",    MCore::CommandSyntax::PARAMTYPE_INT,        "-1");
        GetSyntax().AddParameter("xPos",            "The x-axis of the position of the instance.",                                                                                                                                                      MCore::CommandSyntax::PARAMTYPE_FLOAT,      "0.0");
        GetSyntax().AddParameter("yPos",            "The y-axis of the position of the instance.",                                                                                                                                                      MCore::CommandSyntax::PARAMTYPE_FLOAT,      "0.0");
        GetSyntax().AddParameter("zPos",            "The z-axis of the position of the instance.",                                                                                                                                                      MCore::CommandSyntax::PARAMTYPE_FLOAT,      "0.0");
        GetSyntax().AddParameter("rot",             "The rotation of the actor instance.",                                                                                                                                                              MCore::CommandSyntax::PARAMTYPE_VECTOR4,    "0.0,0.0,0.0,1.0");
        GetSyntax().AddParameter("xScale",          "The x-axis scale of the instances.",                                                                                                                                                               MCore::CommandSyntax::PARAMTYPE_FLOAT,      "1.0");
        GetSyntax().AddParameter("yScale",          "The y-axis scale of the instances.",                                                                                                                                                               MCore::CommandSyntax::PARAMTYPE_FLOAT,      "1.0");
        GetSyntax().AddParameter("zScale",          "The z-axis scale of the instances.",                                                                                                                                                               MCore::CommandSyntax::PARAMTYPE_FLOAT,      "1.0");
        //GetSyntax().AddParameter("xSpacing",      "The x-axis spacing between the instances.",                                                                                                                                                        MCore::CommandSyntax::PARAMTYPE_FLOAT,      "5.0");
        //GetSyntax().AddParameter("ySpacing",      "The y-axis spacing between the instances.",                                                                                                                                                        MCore::CommandSyntax::PARAMTYPE_FLOAT,      "0.0");
        //GetSyntax().AddParameter("zSpacing",      "The z-axis spacing between the instances.",                                                                                                                                                        MCore::CommandSyntax::PARAMTYPE_FLOAT,      "0.0");
        GetSyntax().AddParameter("autoSelect",      "Automatically add the the newly created actor instance to the selection.",                                                                                                                         MCore::CommandSyntax::PARAMTYPE_BOOLEAN,    "true");
    }


    // get the description
    const char* CommandCreateActorInstance::GetDescription() const
    {
        return "This command can be used to create an actor instance from the selected Actor object.";
    }

    //--------------------------------------------------------------------------------
    // CommandAdjustActorInstance
    //--------------------------------------------------------------------------------

    // constructor
    CommandAdjustActorInstance::CommandAdjustActorInstance(MCore::Command* orgCommand)
        : MCore::Command("AdjustActorInstance", orgCommand)
    {
    }


    // destructor
    CommandAdjustActorInstance::~CommandAdjustActorInstance()
    {
    }


    // execute
    bool CommandAdjustActorInstance::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        const uint32 actorInstanceID = parameters.GetValueAsInt("actorInstanceID", MCORE_INVALIDINDEX32);

        // find the actor intance based on the given id
        EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().FindActorInstanceByID(actorInstanceID);
        if (actorInstance == nullptr)
        {
            outResult = AZStd::string::format("Cannot adjust actor instance. Actor instance ID %i is not valid.", actorInstanceID);
            return false;
        }

        // set the position
        if (parameters.CheckIfHasParameter("pos"))
        {
            AZ::Vector3 value       = parameters.GetValueAsVector3("pos", this);
            m_oldPosition            = actorInstance->GetLocalSpaceTransform().m_position;
            actorInstance->SetLocalSpacePosition(value);
        }

        // set the rotation
        if (parameters.CheckIfHasParameter("rot"))
        {
            AZ::Vector4 value   = parameters.GetValueAsVector4("rot", this);
            m_oldRotation        = actorInstance->GetLocalSpaceTransform().m_rotation;
            actorInstance->SetLocalSpaceRotation(AZ::Quaternion(value.GetX(), value.GetY(), value.GetZ(), value.GetW()));
        }

        // set the scale
        EMFX_SCALECODE
        (
            if (parameters.CheckIfHasParameter("scale"))
            {
                AZ::Vector3 value       = parameters.GetValueAsVector3("scale", this);
                m_oldScale               = actorInstance->GetLocalSpaceTransform().m_scale;
                actorInstance->SetLocalSpaceScale(value);
            }
        )

        // set the LOD level
        if (parameters.CheckIfHasParameter("lodLevel"))
        {
            uint32 value    = parameters.GetValueAsInt("lodLevel", this);
            m_oldLodLevel    = actorInstance->GetLODLevel();
            actorInstance->SetLODLevel(value);
        }

        // set the visibility flag
        if (parameters.CheckIfHasParameter("isVisible"))
        {
            bool value      = parameters.GetValueAsBool("isVisible", this);
            m_oldIsVisible   = actorInstance->GetIsVisible();
            actorInstance->SetIsVisible(value);
        }

        // set the rendering flag
        if (parameters.CheckIfHasParameter("doRender"))
        {
            bool value      = parameters.GetValueAsBool("doRender", this);
            m_oldDoRender    = actorInstance->GetRender();
            actorInstance->SetRender(value);
        }

        // mark the workspace as dirty
        m_oldWorkspaceDirtyFlag = GetCommandManager()->GetWorkspaceDirtyFlag();
        GetCommandManager()->SetWorkspaceDirtyFlag(true);

        return true;
    }


    // undo the command
    bool CommandAdjustActorInstance::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        const uint32 actorInstanceID = parameters.GetValueAsInt("actorInstanceID", MCORE_INVALIDINDEX32);

        // find the actor intance based on the given id
        EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().FindActorInstanceByID(actorInstanceID);
        if (actorInstance == nullptr)
        {
            outResult = AZStd::string::format("Cannot adjust actor instance. Actor instance ID %i is not valid.", actorInstanceID);
            return false;
        }

        // set the position
        if (parameters.CheckIfHasParameter("pos"))
        {
            actorInstance->SetLocalSpacePosition(m_oldPosition);
        }

        // set the rotation
        if (parameters.CheckIfHasParameter("rot"))
        {
            actorInstance->SetLocalSpaceRotation(m_oldRotation);
        }

        // set the scale
        EMFX_SCALECODE
        (
            if (parameters.CheckIfHasParameter("scale"))
            {
                actorInstance->SetLocalSpaceScale(m_oldScale);
            }
        )

        // set the LOD level
        if (parameters.CheckIfHasParameter("lodLevel"))
        {
            actorInstance->SetLODLevel(m_oldLodLevel);
        }

        // set the visibility flag
        if (parameters.CheckIfHasParameter("isVisible"))
        {
            actorInstance->SetIsVisible(m_oldIsVisible);
        }

        // set the rendering flag
        if (parameters.CheckIfHasParameter("doRender"))
        {
            actorInstance->SetRender(m_oldDoRender);
        }

        // restore the workspace dirty flag
        GetCommandManager()->SetWorkspaceDirtyFlag(m_oldWorkspaceDirtyFlag);

        return true;
    }


    // init the syntax of the command
    void CommandAdjustActorInstance::InitSyntax()
    {
        GetSyntax().ReserveParameters(8);
        GetSyntax().AddRequiredParameter("actorInstanceID", "The actor instance identification number of the actor instance to work on.", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddParameter("pos", "The position of the actor instance.", MCore::CommandSyntax::PARAMTYPE_VECTOR3, "0.0,0.0,0.0");
        GetSyntax().AddParameter("rot", "The rotation of the actor instance.", MCore::CommandSyntax::PARAMTYPE_VECTOR4, "0.0,0.0,0.0,1.0");
        GetSyntax().AddParameter("scale", "The scale of the actor instance.", MCore::CommandSyntax::PARAMTYPE_VECTOR3, "0.0,0.0,0.0");
        GetSyntax().AddParameter("lodLevel", "The LOD level. Values higher than [GetNumLODLevels()-1] will be clamped to the maximum LOD.", MCore::CommandSyntax::PARAMTYPE_INT, "0");
        GetSyntax().AddParameter("isVisible", "The visibility flag. In case of true the actor instance is getting updated, in case of false the OnUpdate() will be skipped.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "true");
        GetSyntax().AddParameter("doRender", "This flag specifies if the actor instance is getting rendered or not. In case of true the actor instance is rendered, in case of false it will not be visible.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "true");
    }


    // get the description
    const char* CommandAdjustActorInstance::GetDescription() const
    {
        return "This command can be used to adjust the attributes of the currently selected actor instance.";
    }


    //--------------------------------------------------------------------------------
    // CommandRemoveActorInstance
    //--------------------------------------------------------------------------------

    // constructor
    CommandRemoveActorInstance::CommandRemoveActorInstance(MCore::Command* orgCommand)
        : MCore::Command("RemoveActorInstance", orgCommand)
    {
    }


    // destructor
    CommandRemoveActorInstance::~CommandRemoveActorInstance()
    {
    }


    // execute
    bool CommandRemoveActorInstance::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        const uint32 actorInstanceID = parameters.GetValueAsInt("actorInstanceID", MCORE_INVALIDINDEX32);

        // find the actor intance based on the given id
        EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().FindActorInstanceByID(actorInstanceID);
        if (actorInstance == nullptr)
        {
            outResult = AZStd::string::format("Cannot remove actor instance. Actor instance ID %i is not valid.", actorInstanceID);
            return false;
        }

        if (actorInstance->GetEntity())
        {
            outResult = AZStd::string::format("Cannot remove actor instance. Actor instance %i belongs to an entity.", actorInstanceID);
            return false;
        }

        // store the old values before removing the instance
        m_oldPosition            = actorInstance->GetLocalSpaceTransform().m_position;
        m_oldRotation            = actorInstance->GetLocalSpaceTransform().m_rotation;
        EMFX_SCALECODE
        (
            m_oldScale = actorInstance->GetLocalSpaceTransform().m_scale;
        )
        m_oldLodLevel            = actorInstance->GetLODLevel();
        m_oldIsVisible           = actorInstance->GetIsVisible();
        m_oldDoRender            = actorInstance->GetRender();

        // remove the actor instance from the selection
        if (GetCommandManager()->GetLockSelection())
        {
            GetCommandManager()->ExecuteCommandInsideCommand("ToggleLockSelection", outResult);
        }

        // get the id from the corresponding actor and save it for undo
        EMotionFX::Actor* actor = actorInstance->GetActor();
        m_oldActorId = actor->GetID();

        // get rid of the actor instance
        if (actorInstance)
        {
            actorInstance->Destroy();
        }

        // mark the workspace as dirty
        m_oldWorkspaceDirtyFlag = GetCommandManager()->GetWorkspaceDirtyFlag();
        GetCommandManager()->SetWorkspaceDirtyFlag(true);

        return true;
    }


    // undo the command
    bool CommandRemoveActorInstance::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // get the actor instance ID and check if it is still available
        const uint32 actorInstanceID = parameters.GetValueAsInt("actorInstanceID", MCORE_INVALIDINDEX32);
        if (EMotionFX::GetActorManager().FindActorInstanceByID(actorInstanceID))
        {
            outResult = AZStd::string::format("Cannot undo remove actor instance. Actor instance ID %i is already in use.", actorInstanceID);
            return false;
        }

        // create command group for creating/adjusting the actor instance
        AZStd::string commandString;
        MCore::CommandGroup commandGroup("Undo remove actor instance", 2);

        commandString = AZStd::string::format("CreateActorInstance -actorID %i -actorInstanceID %i", m_oldActorId, actorInstanceID);
        commandGroup.AddCommandString(commandString.c_str());

        commandString = AZStd::string::format("AdjustActorInstance -actorInstanceID %i -pos \"%s\" -rot \"%s\" -scale \"%s\" -lodLevel %zu -isVisible \"%s\" -doRender \"%s\"",
            actorInstanceID,
            AZStd::to_string(m_oldPosition).c_str(),
            AZStd::to_string(m_oldRotation).c_str(),
            AZStd::to_string(m_oldScale).c_str(),
            m_oldLodLevel,
            AZStd::to_string(m_oldIsVisible).c_str(),
            AZStd::to_string(m_oldDoRender).c_str()
            );
        commandGroup.AddCommandString(commandString);

        // execute the command group
        bool result = GetCommandManager()->ExecuteCommandGroupInsideCommand(commandGroup, outResult);

        // restore the workspace dirty flag
        GetCommandManager()->SetWorkspaceDirtyFlag(m_oldWorkspaceDirtyFlag);

        return result;
    }


    // init the syntax of the command
    void CommandRemoveActorInstance::InitSyntax()
    {
        GetSyntax().ReserveParameters(1);
        GetSyntax().AddRequiredParameter("actorInstanceID", "The actor instance identification number of the actor instance to work on.", MCore::CommandSyntax::PARAMTYPE_INT);
    }


    // get the description
    const char* CommandRemoveActorInstance::GetDescription() const
    {
        return "This command can be used to ";
    }

    //-------------------------------------------------------------------------------------
    // Helper Functions
    //-------------------------------------------------------------------------------------
    void CloneActorInstance(EMotionFX::ActorInstance* actorInstance, MCore::CommandGroup* commandGroup)
    {
        if (!actorInstance)
        {
            AZ_Error("EMotionFX", false, "Cannot clone invalid instance.");
            return;
        }

        const AZ::Vector3& pos = actorInstance->GetLocalSpaceTransform().m_position;
        const AZ::Quaternion& rot = actorInstance->GetLocalSpaceTransform().m_rotation;
        #ifndef EMFX_SCALE_DISABLED
            const AZ::Vector3& scale = actorInstance->GetLocalSpaceTransform().m_scale;
        #else
            const AZ::Vector3 scale = AZ::Vector3::CreateOne();
        #endif

        AZ::Locale::ScopedSerializationLocale localeScope;  // make sure '%f' uses the "C" Locale.

        const AZStd::string command = AZStd::string::format("CreateActorInstance -actorID %i -xPos %f -yPos %f -zPos %f -xScale %f -yScale %f -zScale %f -rot \"%s\"",
            actorInstance->GetActor()->GetID(),
            static_cast<float>(pos.GetX()), static_cast<float>(pos.GetY()), static_cast<float>(pos.GetZ()),
            static_cast<float>(scale.GetX()), static_cast<float>(scale.GetY()), static_cast<float>(scale.GetZ()),
            AZStd::to_string(rot).c_str());

        CommandSystem::GetCommandManager()->ExecuteCommandOrAddToGroup(command, commandGroup, /*executeInsideCommand=*/false);
    }


    // slots for cloning all selected actor instances
    void CloneSelectedActorInstances()
    {
        // get the selection and number of selected actor instances
        const SelectionList& selection = GetCommandManager()->GetCurrentSelection();
        const size_t numActorInstances = selection.GetNumSelectedActorInstances();

        // create the command group
        MCore::CommandGroup commandGroup("Clone actor instances", numActorInstances);

        // unselect all actors and actor instances
        commandGroup.AddCommandString("Unselect -actorInstanceID SELECT_ALL -actorID SELECT_ALL");

        // iterate over the selected instances and clone them
        for (size_t i = 0; i < numActorInstances; ++i)
        {
            // get the current actor instance
            EMotionFX::ActorInstance* actorInstance = selection.GetActorInstance(i);
            if (actorInstance == nullptr)
            {
                continue;
            }

            // add the clone command
            CloneActorInstance(actorInstance, &commandGroup);
        }

        // execute the commandgroup
        AZStd::string outResult;
        if (GetCommandManager()->ExecuteCommandGroup(commandGroup, outResult, true) == false)
        {
            MCore::LogError(outResult.c_str());
        }
    }


    // call reset to bind pose
    void ResetToBindPose()
    {
        // execute the command
        AZStd::string outResult;
        if (GetCommandManager()->ExecuteCommand("ResetToBindPose", outResult) == false)
        {
            if (outResult.empty() == false)
            {
                MCore::LogError(outResult.c_str());
            }
        }
    }


    // slot for removing all selected actor instances
    void RemoveSelectedActorInstances()
    {
        // get the selection and number of selected actor instances
        const SelectionList& selection = GetCommandManager()->GetCurrentSelection();
        const size_t numActorInstances = selection.GetNumSelectedActorInstances();

        // create the command group
        MCore::CommandGroup commandGroup("Remove actor instances", numActorInstances);
        AZStd::string tempString;

        // iterate over the selected instances and remove them
        for (size_t i = 0; i < numActorInstances; ++i)
        {
            // get the current actor instance
            EMotionFX::ActorInstance* actorInstance = selection.GetActorInstance(i);
            if (actorInstance == nullptr)
            {
                continue;
            }

            // Do not remove any runtime instance from the manager using the commands.
            if (actorInstance->GetIsOwnedByRuntime())
            {
                continue;
            }

            // Do not remove the any instances owned by an entity from the manager using the commands.
            if (actorInstance->GetEntity())
            {
                continue;
            }

            tempString = AZStd::string::format("RemoveActorInstance -actorInstanceID %i", actorInstance->GetID());
            commandGroup.AddCommandString(tempString.c_str());
        }

        // execute the commandgroup
        if (GetCommandManager()->ExecuteCommandGroup(commandGroup, tempString) == false)
        {
            MCore::LogError(tempString.c_str());
        }
    }


    // slot for making actor instances invisible
    void MakeSelectedActorInstancesInvisible()
    {
        // get the selection and number of selected actor instances
        const SelectionList& selection = GetCommandManager()->GetCurrentSelection();
        const size_t numActorInstances = selection.GetNumSelectedActorInstances();

        // create the command group
        AZStd::string outResult;
        AZStd::string command;
        MCore::CommandGroup commandGroup("Hide actor instances", numActorInstances * 2);

        // iterate over the selected instances
        for (size_t i = 0; i < numActorInstances; ++i)
        {
            // get the current actor instance
            EMotionFX::ActorInstance* actorInstance = selection.GetActorInstance(i);
            if (actorInstance == nullptr)
            {
                continue;
            }

            command = AZStd::string::format("Unselect -actorInstanceID %i", actorInstance->GetID());
            commandGroup.AddCommandString(command.c_str());

            command = AZStd::string::format("AdjustActorInstance -actorInstanceID %i -doRender false", actorInstance->GetID());
            commandGroup.AddCommandString(command.c_str());
        }

        // execute the commandgroup
        if (GetCommandManager()->ExecuteCommandGroup(commandGroup, outResult) == false)
        {
            if (outResult.empty() == false)
            {
                MCore::LogError(outResult.c_str());
            }
        }
    }


    // slot for making actor instances visible
    void MakeSelectedActorInstancesVisible()
    {
        // get the selection and number of selected actor instances
        const SelectionList& selection = GetCommandManager()->GetCurrentSelection();
        const size_t numActorInstances = selection.GetNumSelectedActorInstances();

        // create the command group
        AZStd::string outResult;
        AZStd::string command;
        MCore::CommandGroup commandGroup("Unhide actor instances", numActorInstances * 2);

        // iterate over the selected instances
        for (size_t i = 0; i < numActorInstances; ++i)
        {
            // get the current actor instance
            EMotionFX::ActorInstance* actorInstance = selection.GetActorInstance(i);
            if (actorInstance == nullptr)
            {
                continue;
            }

            command = AZStd::string::format("AdjustActorInstance -actorInstanceID %i -doRender true", actorInstance->GetID());
            commandGroup.AddCommandString(command.c_str());
        }

        // execute the commandgroup
        if (GetCommandManager()->ExecuteCommandGroup(commandGroup, outResult) == false)
        {
            if (outResult.empty() == false)
            {
                MCore::LogError(outResult.c_str());
            }
        }
    }


    // unselect the currently selected actors
    void UnselectSelectedActorInstances()
    {
        // get the selection and number of selected actor instances
        SelectionList selection = GetCommandManager()->GetCurrentSelection();
        const size_t numActorInstances = selection.GetNumSelectedActorInstances();

        // create the command group
        AZStd::string outResult;
        AZStd::string command;
        MCore::CommandGroup commandGroup("Unselect all actor instances", numActorInstances + 1);

        // iterate over the selected instances and clone them
        for (size_t i = 0; i < numActorInstances; ++i)
        {
            // get the current actor instance
            EMotionFX::ActorInstance* actorInstance = selection.GetActorInstance(i);
            if (actorInstance == nullptr)
            {
                continue;
            }

            // add the remove command
            command = AZStd::string::format("Unselect -actorInstanceID %i", actorInstance->GetID());
            commandGroup.AddCommandString(command.c_str());
        }

        // disable selection lock, if actor has been deselected
        if (GetCommandManager()->GetLockSelection())
        {
            commandGroup.AddCommandString("ToggleLockSelection");
        }

        // execute the commandgroup
        if (GetCommandManager()->ExecuteCommandGroup(commandGroup, outResult) == false)
        {
            MCore::LogError(outResult.c_str());
        }
    }
} // namespace CommandSystem
