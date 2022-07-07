/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include the required headers
#include "SelectionCommands.h"
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/MotionManager.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <MCore/Source/LogManager.h>


namespace CommandSystem
{
    const char* CommandSelect::s_SelectCmdName = "Select";
    const char* CommandUnselect::s_unselectCmdName = "Unselect";
    const char* CommandClearSelection::s_clearSelectionCmdName = "ClearSelection";
    const char* CommandToggleLockSelection::s_toggleLockSelectionCmdName = "ToggleLockSelection";

    CommandUnselect::CommandUnselect(MCore::Command * orgCommand)
        : MCore::Command(s_unselectCmdName, orgCommand) 
    { }

    CommandClearSelection::CommandClearSelection(MCore::Command * orgCommand)
        : MCore::Command(s_clearSelectionCmdName, orgCommand)
    { }

    CommandToggleLockSelection::CommandToggleLockSelection(MCore::Command * orgCommand)
        : MCore::Command(s_toggleLockSelectionCmdName, orgCommand)
    { }

    void SelectActorInstancesUsingCommands(const AZStd::vector<EMotionFX::ActorInstance*>& selectedActorInstances)
    {
        SelectionList& selection = GetCommandManager()->GetCurrentSelection();
        const size_t numSelectedActorInstances = selectedActorInstances.size();

        // check if the current selection is equal to the desired actor instances selection list
        bool nothingChanged = true;
        for (size_t i = 0; i < numSelectedActorInstances; ++i)
        {
            EMotionFX::ActorInstance* actorInstance = selectedActorInstances[i];
            if (selection.CheckIfHasActorInstance(actorInstance) == false)
            {
                nothingChanged = false;
                break;
            }
        }
        for (size_t i = 0; i < selection.GetNumSelectedActorInstances(); ++i)
        {
            EMotionFX::ActorInstance* actorInstance = selection.GetActorInstance(i);
            if (AZStd::find(begin(selectedActorInstances), end(selectedActorInstances), actorInstance) == end(selectedActorInstances))
            {
                nothingChanged = false;
                break;
            }
        }

        if (nothingChanged == false)
        {
            // create our command group
            AZStd::string outResult;
            MCore::CommandGroup commandGroup("Select actor instances");

            // clear the old selection
            commandGroup.AddCommandString("Unselect -actorInstanceID SELECT_ALL -actorID SELECT_ALL");

            // add the newly selected actor instances
            AZStd::string commandString;
            for (size_t a = 0; a < numSelectedActorInstances; ++a)
            {
                EMotionFX::ActorInstance* actorInstance = selectedActorInstances[a];
                commandString = AZStd::string::format("Select -actorInstanceID %i -actorID %i", actorInstance->GetID(), actorInstance->GetActor()->GetID());
                commandGroup.AddCommandString(commandString.c_str());
            }

            // execute the group command
            if (GetCommandManager()->ExecuteCommandGroup(commandGroup, outResult) == false)
            {
                MCore::LogError(outResult.c_str());
            }
        }
    }


    bool CheckIfHasMotionSelectionParameter(const MCore::CommandLine& parameters)
    {
        if (parameters.CheckIfHasParameter("motionName")   )
        {
            return true;
        }
        if (parameters.CheckIfHasParameter("motionIndex")  )
        {
            return true;
        }

        return false;
    }


    bool CheckIfHasAnimGraphSelectionParameter(const MCore::CommandLine& parameters)
    {
        if (parameters.CheckIfHasParameter("animGraphIndex")  )
        {
            return true;
        }
        if (parameters.CheckIfHasParameter("animGraphID")     )
        {
            return true;
        }

        return false;
    }


    bool CheckIfHasActorSelectionParameter(const MCore::CommandLine& parameters, bool ignoreInstanceParameters)
    {
        if (parameters.CheckIfHasParameter("actorID")   )
        {
            return true;
        }
        if (parameters.CheckIfHasParameter("actorName"))
        {
            return true;
        }
        if (ignoreInstanceParameters == false)
        {
            if (parameters.CheckIfHasParameter("actorInstance"))
            {
                return true;
            }
            if (parameters.CheckIfHasParameter("actorInstanceID")  )
            {
                return true;
            }
        }

        return false;
    }

    //--------------------------------------------------------------------------------
    // CommandSelect
    //--------------------------------------------------------------------------------

    // constructor
    CommandSelect::CommandSelect(MCore::Command* orgCommand)
        : MCore::Command(s_SelectCmdName, orgCommand)
    {
    }


    // destructor
    CommandSelect::~CommandSelect()
    {
    }


    // the actual selection routine used by the select and the unselect command
    bool CommandSelect::Select(Command* command, const MCore::CommandLine& parameters, AZStd::string& outResult, bool unselect)
    {
        // if we are in selection lock mode return directly
        //if (GetCommandManager()->GetLockSelection())
        //  return false;

        SelectionList& selection        = GetCommandManager()->GetCurrentSelection();
        const size_t numActors          = EMotionFX::GetActorManager().GetNumActors();
        const size_t numActorInstances  = EMotionFX::GetActorManager().GetNumActorInstances();
        const size_t numMotions         = EMotionFX::GetMotionManager().GetNumMotions();
        const size_t numAnimGraphs     = EMotionFX::GetAnimGraphManager().GetNumAnimGraphs();

        AZStd::string valueString;

        // add the actor with the given index to the selection list
        if (parameters.CheckIfHasParameter("actorID"))
        {
            parameters.GetValue("actorID", command, &valueString);
            if (AzFramework::StringFunc::Equal(valueString.c_str(), "SELECT_ALL", false /* no case */))
            {
                // iterate through all available actors and add them to the selection
                for (size_t i = 0; i < numActors; ++i)
                {
                    EMotionFX::Actor* actor = EMotionFX::GetActorManager().GetActor(i);
                    
                    if (unselect == false)
                    {
                        selection.AddActor(actor);
                    }
                    else
                    {
                        selection.RemoveActor(actor);
                    }
                }
            }
            else
            {
                const uint32 actorID = parameters.GetValueAsInt("actorID", command);

                // find the actor based on the given id
                EMotionFX::Actor* actor = EMotionFX::GetActorManager().FindActorByID(actorID);
                if (actor == nullptr)
                {
                    outResult = AZStd::string::format("Cannot select actor. Actor ID %i is not valid.", actorID);
                    return false;
                }

                if (unselect == false)
                {
                    selection.AddActor(actor);
                }
                else
                {
                    selection.RemoveActor(actor);
                }
            }
        }


        // add the actor with the given name to the selection list
        if (parameters.CheckIfHasParameter("actorName"))
        {
            // get the actor name and check if it is valid
            parameters.GetValue("actorName", command, &valueString);
            if (valueString.empty())
            {
                outResult = "Actor name is empty. Cannot select actors with empty name.";
                return false;
            }

            // iterate through all available actors and add them to the selection
            for (size_t i = 0; i < numActors; ++i)
            {
                EMotionFX::Actor* actor = EMotionFX::GetActorManager().GetActor(i);

                if (AzFramework::StringFunc::Equal(valueString.c_str(), actor->GetName(), false /* no case */))
                {
                    if (unselect == false)
                    {
                        selection.AddActor(actor);
                    }
                    else
                    {
                        selection.RemoveActor(actor);
                    }
                }
            }
        }


        // add the actor instance with the given index to the selection list
        if (parameters.CheckIfHasParameter("actorInstanceID"))
        {
            parameters.GetValue("actorInstanceID", command, &valueString);
            if (AzFramework::StringFunc::Equal(valueString.c_str(), "SELECT_ALL", false /* no case */))
            {
                // iterate through all available actor instances and add them to the selection
                for (size_t i = 0; i < numActorInstances; ++i)
                {
                    EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().GetActorInstance(i);

                    if (actorInstance->GetIsOwnedByRuntime())
                    {
                        continue;
                    }

                    if (unselect == false)
                    {
                        selection.AddActorInstance(actorInstance);
                    }
                    else
                    {
                        selection.RemoveActorInstance(actorInstance);
                    }
                }
            }
            else
            {
                // get the actor instance from the string and check if it is valid
                const uint32 actorInstanceID = parameters.GetValueAsInt("actorInstanceID", command);
                EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().FindActorInstanceByID(actorInstanceID);
                if (actorInstance == nullptr)
                {
                    outResult = AZStd::string::format("Actor instance ID %i is not valid. There are no actor instances registered in the actor manager with the given ID.", actorInstanceID);
                    return false;
                }

                if (actorInstance->GetIsOwnedByRuntime())
                {
                    return false;
                }

                if (unselect == false)
                {
                    selection.AddActorInstance(actorInstance);
                }
                else
                {
                    selection.RemoveActorInstance(actorInstance);
                }
            }
        }


        // add the motion with the given name to the selection list
        if (parameters.CheckIfHasParameter("motionName"))
        {
            // get the motion name and check if it is valid
            parameters.GetValue("motionName", command, &valueString);
            if (valueString.empty())
            {
                outResult = "Motion name is empty. Cannot select motions with empty name.";
                return false;
            }

            // iterate through all available motions and add them to the selection
            for (size_t i = 0; i < numMotions; ++i)
            {
                // get the current motion
                EMotionFX::Motion* motion = EMotionFX::GetMotionManager().GetMotion(i);

                if (motion->GetIsOwnedByRuntime())
                {
                    continue;
                }

                // compare the motion name to the parameter
                if (AzFramework::StringFunc::Equal(valueString.c_str(), motion->GetName(), false /* no case */))
                {
                    if (unselect == false)
                    {
                        selection.AddMotion(motion);
                    }
                    else
                    {
                        selection.RemoveMotion(motion);
                    }
                }
            }
        }

        // add the motion with the given name to the selection list
        if (parameters.CheckIfHasParameter("motionIndex"))
        {
            parameters.GetValue("motionIndex", command, &valueString);
            if (AzFramework::StringFunc::Equal(valueString.c_str(), "SELECT_ALL", false /* no case */))
            {
                // iterate through all available motions and add them to the selection
                for (size_t i = 0; i < numMotions; ++i)
                {
                    // get the current motion
                    EMotionFX::Motion* motion = EMotionFX::GetMotionManager().GetMotion(i);

                    if (motion->GetIsOwnedByRuntime())
                    {
                        continue;
                    }

                    if (unselect == false)
                    {
                        selection.AddMotion(motion);
                    }
                    else
                    {
                        selection.RemoveMotion(motion);
                    }
                }
            }
            else
            {
                // get the motion index from the string and check if it is valid
                const size_t motionIndex = parameters.GetValueAsInt("motionIndex", command);
                if (motionIndex >= numMotions)
                {
                    if (numMotions == 0)
                    {
                        outResult = "Motion index is not valid. There is no motion registered in the motion library.";
                    }
                    else
                    {
                        outResult = AZStd::string::format("Motion index '%zu' is not valid. Valid range is [0, %zu].", motionIndex, numMotions - 1);
                    }

                    return false;
                }

                // get the given motion from the motion library and add/remove it to the selection
                EMotionFX::Motion* motion = EMotionFX::GetMotionManager().GetMotion(motionIndex);

                if (motion->GetIsOwnedByRuntime())
                {
                    return false;
                }

                if (unselect == false)
                {
                    selection.AddMotion(motion);
                }
                else
                {
                    selection.RemoveMotion(motion);
                }
            }
        }


        // add the anim graph with the given name to the selection list
        if (parameters.CheckIfHasParameter("animGraphIndex"))
        {
            parameters.GetValue("animGraphIndex", command, &valueString);
            if (AzFramework::StringFunc::Equal(valueString.c_str(), "SELECT_ALL", false /* no case */))
            {
                // iterate through all available motions and add them to the selection
                for (size_t i = 0; i < numAnimGraphs; ++i)
                {
                    // get the current anim graph
                    EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().GetAnimGraph(i);

                    if (animGraph->GetIsOwnedByRuntime())
                    {
                        continue;
                    }

                    if (unselect == false)
                    {
                        selection.AddAnimGraph(animGraph);
                    }
                    else
                    {
                        selection.RemoveAnimGraph(animGraph);
                    }
                }
            }
            else
            {
                // get the anim graph index from the string and check if it is valid
                const size_t animGraphIndex = parameters.GetValueAsInt("animGraphIndex", command);
                if (animGraphIndex >= numAnimGraphs)
                {
                    if (numAnimGraphs == 0)
                    {
                        outResult = "Anim graph index is not valid. There is no anim graph registered in the anim graph manager.";
                    }
                    else
                    {
                        outResult = AZStd::string::format("Anim graph index '%zu' is not valid. Valid range is [0, %zu].", animGraphIndex, numAnimGraphs - 1);
                    }

                    return false;
                }

                // get the given anim graph from the motion library and add/remove it to the selection
                EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().GetAnimGraph(animGraphIndex);

                if (animGraph->GetIsOwnedByRuntime())
                {
                    return false;
                }

                if (unselect == false)
                {
                    selection.AddAnimGraph(animGraph);
                }
                else
                {
                    selection.RemoveAnimGraph(animGraph);
                }
            }
        }


        // add the anim graph with the given id to the selection list
        if (parameters.CheckIfHasParameter("animGraphID"))
        {
            parameters.GetValue("animGraphID", command, &valueString);
            if (AzFramework::StringFunc::Equal(valueString.c_str(), "SELECT_ALL", false /* no case */))
            {
                // iterate through all available motions and add them to the selection
                for (size_t i = 0; i < numAnimGraphs; ++i)
                {
                    // get the current anim graph
                    EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().GetAnimGraph(i);

                    if (animGraph->GetIsOwnedByRuntime())
                    {
                        continue;
                    }

                    if (unselect == false)
                    {
                        selection.AddAnimGraph(animGraph);
                    }
                    else
                    {
                        selection.RemoveAnimGraph(animGraph);
                    }
                }
            }
            else
            {
                // get the anim graph id from the string and check if it is valid
                const uint32            animGraphID    = parameters.GetValueAsInt("animGraphID", command);
                EMotionFX::AnimGraph*  animGraph      = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
                if (animGraph == nullptr)
                {
                    outResult = AZStd::string::format("Anim graph id '%i' is not valid. Cannot find anim graph with the given id.", animGraphID);
                    return false;
                }

                if (animGraph->GetIsOwnedByRuntime())
                {
                    return false;
                }

                if (unselect == false)
                {
                    selection.AddAnimGraph(animGraph);
                }
                else
                {
                    selection.RemoveAnimGraph(animGraph);
                }
            }
        }

        return true;
    }


    // execute the command
    bool CommandSelect::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // store the old selection list for undo
        m_data = GetCommandManager()->GetCurrentSelection();

        // selection add mode
        return Select(this, parameters, outResult, false);
    }


    // undo the command
    bool CommandSelect::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        MCORE_UNUSED(parameters);
        MCORE_UNUSED(outResult);

        // restore the old selection and return success
        GetCommandManager()->SetCurrentSelection(m_data);
        return true;
    }


    // init the syntax of the command
    void CommandSelect::InitSyntax()
    {
        GetSyntax().ReserveParameters(11);
        GetSyntax().AddParameter("actorName",              ".",    MCore::CommandSyntax::PARAMTYPE_STRING, "unnamed");
        GetSyntax().AddParameter("actorID",                ".",    MCore::CommandSyntax::PARAMTYPE_STRING, "SELECT_ALL");
        GetSyntax().AddParameter("actorInstanceID",        ".",    MCore::CommandSyntax::PARAMTYPE_STRING, "SELECT_ALL");
        GetSyntax().AddParameter("motionName",             ".",    MCore::CommandSyntax::PARAMTYPE_STRING, "unnamed");
        GetSyntax().AddParameter("motionIndex",            ".",    MCore::CommandSyntax::PARAMTYPE_STRING, "SELECT_ALL");
        GetSyntax().AddParameter("motionInstanceIndex",    ".",    MCore::CommandSyntax::PARAMTYPE_STRING, "SELECT_ALL");
        GetSyntax().AddParameter("nodeName",               ".",    MCore::CommandSyntax::PARAMTYPE_STRING, "unnamed");
        GetSyntax().AddParameter("nodeIndex",              ".",    MCore::CommandSyntax::PARAMTYPE_STRING, "SELECT_ALL");
        GetSyntax().AddParameter("animGraphIndex",        ".",    MCore::CommandSyntax::PARAMTYPE_STRING, "SELECT_ALL");
        GetSyntax().AddParameter("animGraphID",           ".",    MCore::CommandSyntax::PARAMTYPE_STRING, "SELECT_ALL");
    }


    // get the description
    const char* CommandSelect::GetDescription() const
    {
        return "Select a given item.";
    }


    //--------------------------------------------------------------------------------
    // CommandUnselect
    //--------------------------------------------------------------------------------

    // destructor
    CommandUnselect::~CommandUnselect()
    {
    }


    // execute the command
    bool CommandUnselect::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // store the old selection list for undo
        m_data = GetCommandManager()->GetCurrentSelection();

        // unselect mode
        return CommandSelect::Select(this, parameters, outResult, true);
    }


    // undo the command
    bool CommandUnselect::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        MCORE_UNUSED(parameters);
        MCORE_UNUSED(outResult);

        // restore the old selection and return success
        GetCommandManager()->SetCurrentSelection(m_data);
        return true;
    }


    // init the syntax of the command
    void CommandUnselect::InitSyntax()
    {
        GetSyntax().ReserveParameters(11);
        GetSyntax().AddParameter("actorName",              ".",    MCore::CommandSyntax::PARAMTYPE_STRING, "unnamed");
        GetSyntax().AddParameter("actorID",                ".",    MCore::CommandSyntax::PARAMTYPE_STRING, "SELECT_ALL");
        GetSyntax().AddParameter("actorInstanceID",        ".",    MCore::CommandSyntax::PARAMTYPE_STRING, "SELECT_ALL");
        GetSyntax().AddParameter("motionName",             ".",    MCore::CommandSyntax::PARAMTYPE_STRING, "unnamed");
        GetSyntax().AddParameter("motionIndex",            ".",    MCore::CommandSyntax::PARAMTYPE_STRING, "SELECT_ALL");
        GetSyntax().AddParameter("motionInstanceIndex",    ".",    MCore::CommandSyntax::PARAMTYPE_STRING, "SELECT_ALL");
        GetSyntax().AddParameter("nodeName",               ".",    MCore::CommandSyntax::PARAMTYPE_STRING, "unnamed");
        GetSyntax().AddParameter("nodeIndex",              ".",    MCore::CommandSyntax::PARAMTYPE_STRING, "SELECT_ALL");
        GetSyntax().AddParameter("animGraphIndex",        ".",    MCore::CommandSyntax::PARAMTYPE_STRING, "SELECT_ALL");
        GetSyntax().AddParameter("animGraphID",           ".",    MCore::CommandSyntax::PARAMTYPE_STRING, "SELECT_ALL");
    }


    // get the description
    const char* CommandUnselect::GetDescription() const
    {
        return "Unselect a given item.";
    }


    //--------------------------------------------------------------------------------
    // CommandClearSelection
    //--------------------------------------------------------------------------------

    // destructor
    CommandClearSelection::~CommandClearSelection()
    {
    }


    // execute the command
    bool CommandClearSelection::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        MCORE_UNUSED(parameters);
        MCORE_UNUSED(outResult);

        // get the current selection, store it for the undo function and unselect everything
        SelectionList& selection = GetCommandManager()->GetCurrentSelection();
        m_data = selection;

        // if we are in selection lock mode return directly
        //if (GetCommandManager()->GetLockSelection())
        //  return false;

        // clear the selection
        selection.Clear();
        MCORE_ASSERT(selection.GetIsEmpty());

        return true;
    }


    // undo the command
    bool CommandClearSelection::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        MCORE_UNUSED(parameters);
        MCORE_UNUSED(outResult);

        // restore the old selection and return success
        GetCommandManager()->SetCurrentSelection(m_data);
        return true;
    }


    // init the syntax of the command
    void CommandClearSelection::InitSyntax()
    {
    }


    // get the description
    const char* CommandClearSelection::GetDescription() const
    {
        return "This command can be used to unselect all objects.";
    }


    //--------------------------------------------------------------------------------
    // CommandToggleLockSelection
    //--------------------------------------------------------------------------------

    // destructor
    CommandToggleLockSelection::~CommandToggleLockSelection()
    {
    }


    // execute the command
    bool CommandToggleLockSelection::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        MCORE_UNUSED(parameters);
        MCORE_UNUSED(outResult);

        // store the selection locked flag for the undo function
        m_data = GetCommandManager()->GetLockSelection();

        // toggle the flag
        GetCommandManager()->SetLockSelection(!m_data);

        return true;
    }


    // undo the command
    bool CommandToggleLockSelection::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        MCORE_UNUSED(parameters);
        MCORE_UNUSED(outResult);

        // restore the old selection locked flag and return success
        GetCommandManager()->SetLockSelection(m_data);
        return true;
    }


    // init the syntax of the command
    void CommandToggleLockSelection::InitSyntax()
    {
    }


    // get the description
    const char* CommandToggleLockSelection::GetDescription() const
    {
        return "This command can be used to (un)lock the selection.";
    }
} // namespace CommandSystem
