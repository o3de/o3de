/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AnimGraphGroupParameterCommands.h"
#include "AnimGraphParameterCommands.h"
#include "CommandManager.h"
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/BlendTreeParameterNode.h>
#include <EMotionFX/Source/Parameter/ParameterFactory.h>
#include <MCore/Source/Random.h>

namespace CommandSystem
{
    //--------------------------------------------------------------------------------
    // CommandAnimGraphAdjustGroupParameter
    //--------------------------------------------------------------------------------
    CommandAnimGraphAdjustGroupParameter::CommandAnimGraphAdjustGroupParameter(MCore::Command* orgCommand)
        : MCore::Command("AnimGraphAdjustGroupParameter", orgCommand)
    {
    }


    CommandAnimGraphAdjustGroupParameter::~CommandAnimGraphAdjustGroupParameter()
    {
    }

    CommandAnimGraphAdjustGroupParameter::Action CommandAnimGraphAdjustGroupParameter::GetAction(const MCore::CommandLine& parameters)
    {
        // Do we want to add new parameters to the group, remove some from it or replace it entirely.
        const AZStd::string& actionString = parameters.GetValue("action", this);

        if (actionString == "add")
        {
            return ACTION_ADD;
        }
        else if (actionString == "clear" || actionString == "remove")
        {
            return ACTION_REMOVE;
        }

        return ACTION_NONE;
    }


    bool CommandAnimGraphAdjustGroupParameter::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // Get the parameter name.
        const AZStd::string& name = parameters.GetValue("name", this);
        mOldName = name;

        // Find the anim graph by using the id from command parameter.
        const uint32 animGraphID = parameters.GetValueAsInt("animGraphID", this);
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
        if (!animGraph)
        {
            outResult = AZStd::string::format("Cannot adjust group parameter '%s'. Anim graph id '%d' is not valid.", name.c_str(), animGraphID);
            return false;
        }

        // Find the group parameter index and get a pointer to the group parameter.
        EMotionFX::GroupParameter* groupParameter = animGraph->FindGroupParameterByName(name);
        if (!groupParameter)
        {
            outResult = AZStd::string::format("Cannot adjust group parameter '%s'. group parameter not found.", name.c_str());
            return false;
        }

        if (parameters.CheckIfHasParameter("parameterNames"))
        {
            // Do we want to add new parameters to the group, remove some from it or replace it entirely.
            const Action action = GetAction(parameters);

            // Get the parameter string and tokenize it.
            const AZStd::string& parametersString = parameters.GetValue("parameterNames", this);

            AZStd::vector<AZStd::string> parameterNames;
            AzFramework::StringFunc::Tokenize(parametersString.c_str(), parameterNames, ";", false, true);

            // Ensure that none of the new children are parents of the group they're being added to
            if (action == ACTION_ADD)
            {
                for (const AZStd::string& newChildName : parameterNames)
                {
                    const EMotionFX::Parameter* newChildParameter = animGraph->FindParameterByName(newChildName);
                    if (const auto* newChildGroupParameter = azrtti_cast<const EMotionFX::GroupParameter*>(newChildParameter); newChildGroupParameter)
                    {
                        if (newChildGroupParameter->FindRelativeParameterIndex(groupParameter).IsSuccess())
                        {
                            outResult = AZStd::string::format(
                                "Cannot set parameter '%s' to be a child of '%s' because '%s' is a child of '%s'",
                                newChildGroupParameter->GetName().c_str(),
                                groupParameter->GetName().c_str(),
                                groupParameter->GetName().c_str(),
                                newChildGroupParameter->GetName().c_str()
                            );
                            return false;
                        }
                    }
                }
            }

            const size_t numParameters = parameterNames.size();
            mOldGroupParameterNames.resize(numParameters);

            EMotionFX::ValueParameterVector valueParametersBeforeChange = animGraph->RecursivelyGetValueParameters();

            // Iterate through all parameter names.
            for (size_t i = 0; i < numParameters; ++i)
            {
                const EMotionFX::Parameter* parameter = animGraph->FindParameterByName(parameterNames[i]);
                if (!parameter)
                {
                    mOldGroupParameterNames[i].clear();
                    continue;
                }

                // Save the group parameter (for undo) to which the parameter belonged before command execution.
                const EMotionFX::GroupParameter* parentParameter = animGraph->FindParentGroupParameter(parameter);
                mOldGroupParameterNames[i] = parentParameter ? parentParameter->GetName() : "";

                // Make sure the parameter is not in any other group.
                animGraph->TakeParameterFromParent(parameter);

                // Add the parameter to the desired group.
                // Note: If the group parameter is nullptr, the parameter will move back to the Default group.
                // The remove action will move the parameter back to the default group
                animGraph->AddParameter(const_cast<EMotionFX::Parameter*>(parameter),
                    action == ACTION_ADD ? groupParameter : nullptr);
            }

            EMotionFX::ValueParameterVector valueParametersAfterChange = animGraph->RecursivelyGetValueParameters();

            AZStd::vector<EMotionFX::AnimGraphObject*> affectedObjects;
            animGraph->RecursiveCollectObjectsOfType(azrtti_typeid<EMotionFX::ObjectAffectedByParameterChanges>(), affectedObjects);
            EMotionFX::GetAnimGraphManager().RecursiveCollectObjectsAffectedBy(animGraph, affectedObjects);

            for (EMotionFX::AnimGraphObject* affectedObject : affectedObjects)
            {
                EMotionFX::ObjectAffectedByParameterChanges* affectedObjectByParameterChanges = azdynamic_cast<EMotionFX::ObjectAffectedByParameterChanges*>(affectedObject);
                affectedObjectByParameterChanges->ParameterOrderChanged(valueParametersBeforeChange, valueParametersAfterChange);
            }

            // Update the parameter values for all instances as their type might have changed.
            const size_t numInstances = animGraph->GetNumAnimGraphInstances();
            for (size_t i = 0; i < numInstances; ++i)
            {
                EMotionFX::AnimGraphInstance* animGraphInstance = animGraph->GetAnimGraphInstance(i);
                animGraphInstance->ReInitParameterValues();
            }
        }

        // Set the new name.
        const AZStd::string& newName = parameters.GetValue("newName", this);
        if (!newName.empty())
        {
            if (!animGraph->RenameParameter(groupParameter, newName))
            {
                outResult = AZStd::string::format("Cannot adjust group parameter '%s'. Newname already belongs to a different parameter.", name.c_str());
                return false;
            }
        }

        m_oldDescription = groupParameter->GetDescription();
        if (parameters.CheckIfHasParameter("description"))
        {
            groupParameter->SetDescription(parameters.GetValue("description", this));
        }

        // save the current dirty flag and tell the anim graph that something got changed
        mOldDirtyFlag = animGraph->GetDirtyFlag();
        animGraph->SetDirtyFlag(true);

        animGraph->RecursiveInvalidateUniqueDatas();
        return true;
    }


    bool CommandAnimGraphAdjustGroupParameter::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // Find the anim graph by using the id from command parameter.
        const uint32 animGraphID = parameters.GetValueAsInt("animGraphID", this);
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
        if (!animGraph)
        {
            outResult = AZStd::string::format("Cannot undo adjust group parameter. Anim graph id '%d' is not valid.", animGraphID);
            return false;
        }

        MCore::CommandGroup commandGroup;

        // Undo the group name as first step. All commands afterwards have to use mOldName as group name.
        if (parameters.CheckIfHasParameter("newName"))
        {
            const AZStd::string& newName = parameters.GetValue("newName", this);

            const AZStd::string command = AZStd::string::format("AnimGraphAdjustGroupParameter -animGraphID %i -name \"%s\" -newName \"%s\"",
                    animGraph->GetID(), newName.c_str(), mOldName.c_str());

            commandGroup.AddCommandString(command);
        }

        if (parameters.CheckIfHasParameter("description"))
        {
            const AZStd::string command = AZStd::string::format("AnimGraphAdjustGroupParameter -animGraphID %i -name \"%s\" -description \"%s\"",
                animGraph->GetID(), mOldName.c_str(), m_oldDescription.c_str());
            commandGroup.AddCommandString(command);
        }

        if (parameters.CheckIfHasParameter("parameterNames"))
        {
            // Do we want to add new parameters to the group, remove some from it or replace it entirely.
            const Action action = GetAction(parameters);

            const AZStd::string& parametersString = parameters.GetValue("parameterNames", this);

            AZStd::vector<AZStd::string> parameterNames;
            AzFramework::StringFunc::Tokenize(parametersString.c_str(), parameterNames, ";", false, true);

            const size_t parameterCount = parameterNames.size();
            AZ_Assert(parameterCount == mOldGroupParameterNames.size(), "The number of parameter names has to match the saved group parameter info for undo.");

            for (size_t i = 0; i < parameterCount; ++i)
            {
                const AZStd::string& parameterName = parameterNames[i];
                const AZStd::string& oldGroupName = mOldGroupParameterNames[i];

                switch (action)
                {
                case ACTION_ADD:
                {
                    if (oldGroupName.empty())
                    {
                        // An empty old group name means that the parameter was in the Default group before, so in this case just remove the parameter from the group.
                        const AZStd::string command = AZStd::string::format("AnimGraphAdjustGroupParameter -animGraphID %i -name \"%s\" -action \"remove\" -parameterNames \"%s\"",
                                animGraph->GetID(), mOldName.c_str(), parameterName.c_str());

                        commandGroup.AddCommandString(command);
                    }
                    else
                    {
                        // Add the parameter to the old group which auto removes it from all other groups.
                        const AZStd::string command = AZStd::string::format("AnimGraphAdjustGroupParameter -animGraphID %i -name \"%s\" -action \"add\" -parameterNames \"%s\"",
                                animGraph->GetID(), oldGroupName.c_str(), parameterName.c_str());

                        commandGroup.AddCommandString(command);
                    }

                    break;
                }

                case ACTION_REMOVE:
                {
                    const AZStd::string command = AZStd::string::format("AnimGraphAdjustGroupParameter -animGraphID %i -name \"%s\" -action \"add\" -parameterNames \"%s\"",
                            animGraph->GetID(), oldGroupName.c_str(), parameterName.c_str());

                    commandGroup.AddCommandString(command);

                    break;
                }
                }
            }
        }

        // Execute the command group.
        AZStd::string result;
        if (!GetCommandManager()->ExecuteCommandGroupInsideCommand(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }

        // Set the dirty flag back to the old value.
        animGraph->SetDirtyFlag(mOldDirtyFlag);
        return true;
    }


    void CommandAnimGraphAdjustGroupParameter::InitSyntax()
    {
        GetSyntax().ReserveParameters(6);
        GetSyntax().AddRequiredParameter("animGraphID", "The id of the blend set the group parameter belongs to.", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddParameter("name",            "The name of the group parameter to adjust.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("newName",     "The new name of the group parameter.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("parameterNames",  "A list of parameter names that should be added/removed to/from the group parameter.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("action",          "The action to perform with the parameters passed to the command.", MCore::CommandSyntax::PARAMTYPE_STRING, "select");
        GetSyntax().AddParameter("description", "The description of the parameter group.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("updateUI", "Setting this to true will trigger a refresh of the parameter UI.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "true");
    }


    const char* CommandAnimGraphAdjustGroupParameter::GetDescription() const
    {
        return "This command can be used to adjust the group parameters of the given anim graph.";
    }

    //--------------------------------------------------------------------------------
    // CommandAnimGraphAddGroupParameter
    //--------------------------------------------------------------------------------
    CommandAnimGraphAddGroupParameter::CommandAnimGraphAddGroupParameter(MCore::Command* orgCommand)
        : MCore::Command("AnimGraphAddGroupParameter", orgCommand)
    {
    }


    CommandAnimGraphAddGroupParameter::~CommandAnimGraphAddGroupParameter()
    {
    }


    bool CommandAnimGraphAddGroupParameter::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // Find the anim graph by using the id from command parameter.
        const uint32 animGraphID = parameters.GetValueAsInt("animGraphID", this);
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
        if (!animGraph)
        {
            outResult = AZStd::string::format("Cannot add group parameter. Anim graph id '%d' is not valid.", animGraphID);
            return false;
        }

        AZStd::string valueString;
        if (parameters.CheckIfHasParameter("name"))
        {
            parameters.GetValue("name", this, &valueString);
        }
        else
        {
            // generate a unique parameter name
            valueString = MCore::GenerateUniqueString("Parameter", [&](const AZStd::string& value)
                    {
                        return (animGraph->FindGroupParameterByName(value) == nullptr);
                    });
        }

        if (animGraph->FindParameterByName(valueString))
        {
            outResult = AZStd::string::format("There is already a parameter with the name '%s', please use a unique name.", valueString.c_str());
            return false;
        }

        // add new group parameter to the anim graph
        EMotionFX::Parameter* groupParameter = EMotionFX::ParameterFactory::Create(azrtti_typeid<EMotionFX::GroupParameter>());
        groupParameter->SetName(valueString);

        const AZStd::string& parentName = parameters.GetValue("parent", this);
        const EMotionFX::GroupParameter* parentGroupParameter = animGraph->FindGroupParameterByName(parentName);

        // if the index parameter is specified and valid insert the group parameter at the given position, else just add it to the end
        const uint32 index = parameters.GetValueAsInt("index", this);
        if (index != MCORE_INVALIDINDEX32)
        {
            animGraph->InsertParameter(index, groupParameter, parentGroupParameter);
        }
        else
        {
            animGraph->AddParameter(groupParameter, parentGroupParameter);
        }

        // save the current dirty flag and tell the anim graph that something got changed
        mOldDirtyFlag   = animGraph->GetDirtyFlag();
        mOldName        = groupParameter->GetName();
        animGraph->SetDirtyFlag(true);

        animGraph->RecursiveInvalidateUniqueDatas();
        return true;
    }


    bool CommandAnimGraphAddGroupParameter::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // Find the anim graph by using the id from command parameter.
        const uint32 animGraphID = parameters.GetValueAsInt("animGraphID", this);
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
        if (!animGraph)
        {
            outResult = AZStd::string::format("Cannot undo add group parameter. Anim graph id '%d' is not valid.", animGraphID);
            return false;
        }

        // Construct and execute the command.
        const AZStd::string command = AZStd::string::format("AnimGraphRemoveGroupParameter -animGraphID %i -name \"%s\"", animGraphID, mOldName.c_str());

        AZStd::string result;
        if (!GetCommandManager()->ExecuteCommandInsideCommand(command, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }

        // Set the dirty flag back to the old value.
        animGraph->SetDirtyFlag(mOldDirtyFlag);
        return true;
    }


    void CommandAnimGraphAddGroupParameter::InitSyntax()
    {
        GetSyntax().ReserveParameters(5);
        GetSyntax().AddRequiredParameter("animGraphID", "The id of the blend set the group parameter belongs to.", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddParameter("name", "The name of the group parameter.", MCore::CommandSyntax::PARAMTYPE_STRING, "Unnamed group parameter");
        GetSyntax().AddParameter("index", "The index position where the new group parameter should be added to, relative to the parent.", MCore::CommandSyntax::PARAMTYPE_INT, "-1");
        GetSyntax().AddParameter("parent", "The parent group parameter where the new parameter should be added to.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("updateUI", "Setting this to true will trigger a refresh of the parameter UI.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "true");
    }


    const char* CommandAnimGraphAddGroupParameter::GetDescription() const
    {
        return "This command can be used to add a new group parameter to the given anim graph.";
    }

    //--------------------------------------------------------------------------------
    // CommandAnimGraphRemoveGroupParameter
    //--------------------------------------------------------------------------------
    CommandAnimGraphRemoveGroupParameter::CommandAnimGraphRemoveGroupParameter(MCore::Command* orgCommand)
        : MCore::Command("AnimGraphRemoveGroupParameter", orgCommand)
    {
    }


    CommandAnimGraphRemoveGroupParameter::~CommandAnimGraphRemoveGroupParameter()
    {
    }


    bool CommandAnimGraphRemoveGroupParameter::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // Get the parameter name.
        const AZStd::string& name = parameters.GetValue("name", this);

        // Find the anim graph by using the id from command parameter.
        const uint32 animGraphID = parameters.GetValueAsInt("animGraphID", this);
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
        if (!animGraph)
        {
            outResult = AZStd::string::format("Cannot remove group parameter. Anim graph id '%d' is not valid.", animGraphID);
            return false;
        }

        // find the parameter and remove it
        const EMotionFX::Parameter* parameter = animGraph->FindParameterByName(name);
        if (!parameter)
        {
            outResult = AZStd::string::format("Cannot remove group parameter from anim graph. Group parameter '%s' was not found.", name.c_str());
            return false;
        }
        if (azrtti_typeid(parameter) != azrtti_typeid<EMotionFX::GroupParameter>())
        {
            outResult = AZStd::string::format("Cannot remove group parameter from anim graph. Parameter '%s' is not a group parameter.", name.c_str());
            return false;
        }

        // read out information for the command undo
        mOldName = parameter->GetName();
        const EMotionFX::GroupParameter* parentGroup = animGraph->FindParentGroupParameter(parameter);
        if (parentGroup)
        {
            mOldParent = parentGroup->GetName();
            mOldIndex = parentGroup->FindParameterIndex(parameter).GetValue();
        }
        else
        {
            mOldParent = "";
            mOldIndex = animGraph->FindParameterIndex(parameter).GetValue();
        }

        mOldParameterNames.clear();

        // Collect all child parameters and move them to the default group. Keep the child hierarchy as it is.
        // Add the immediate child ones to mOldParameterNames so they get moved back on undo
        const EMotionFX::GroupParameter* groupParameter = static_cast<const EMotionFX::GroupParameter*>(parameter);
        const EMotionFX::ParameterVector childParameters = groupParameter->RecursivelyGetChildParameters();
        AZStd::vector<const EMotionFX::GroupParameter*> childParents;

        const size_t childParameterCount = childParameters.size();
        childParents.resize(childParameterCount);

        EMotionFX::ValueParameterVector valueParametersBeforeChange = animGraph->RecursivelyGetValueParameters();

        // childParameters is sorted from root to leaf, first we iterate from leaf to root getting the parents
        // and taking the parameters out of them
        for (int i = static_cast<int>(childParameterCount) - 1; i >= 0; --i)
        {
            childParents[i] = animGraph->FindParentGroupParameter(childParameters[i]);
            animGraph->TakeParameterFromParent(childParameters[i]);
        }

        // Now we iterate root to child and add the parameters to the right parent
        for (size_t i = 0; i < childParameterCount; ++i)
        {
            const EMotionFX::GroupParameter* parent = childParents[i];
            if (parent == groupParameter)
            {
                mOldParameterNames += childParameters[i]->GetName() + ";";
                animGraph->AddParameter(childParameters[i]); // add to default group
            }
            else
            {
                animGraph->AddParameter(childParameters[i], parent); // add to default group
            }
        }
        if (!mOldParameterNames.empty())
        {
            mOldParameterNames.pop_back(); // remove trailing ";"
        }

        // remove the group parameter
        animGraph->RemoveParameter(const_cast<EMotionFX::Parameter*>(parameter));

        EMotionFX::ValueParameterVector valueParametersAfterChange = animGraph->RecursivelyGetValueParameters();

        if (valueParametersBeforeChange != valueParametersAfterChange)
        {
            AZStd::vector<EMotionFX::AnimGraphObject*> affectedObjects;
            animGraph->RecursiveCollectObjectsOfType(azrtti_typeid<EMotionFX::ObjectAffectedByParameterChanges>(), affectedObjects);
            EMotionFX::GetAnimGraphManager().RecursiveCollectObjectsAffectedBy(animGraph, affectedObjects);

            for (EMotionFX::AnimGraphObject* affectedObject : affectedObjects)
            {
                EMotionFX::ObjectAffectedByParameterChanges* affectedObjectByParameterChanges = azdynamic_cast<EMotionFX::ObjectAffectedByParameterChanges*>(affectedObject);
                affectedObjectByParameterChanges->ParameterOrderChanged(valueParametersBeforeChange, valueParametersAfterChange);
            }
        }

        // save the current dirty flag and tell the anim graph that something got changed
        mOldDirtyFlag = animGraph->GetDirtyFlag();
        animGraph->SetDirtyFlag(true);

        animGraph->RecursiveInvalidateUniqueDatas();
        return true;
    }


    bool CommandAnimGraphRemoveGroupParameter::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // Find the anim graph by using the id from command parameter.
        const uint32 animGraphID = parameters.GetValueAsInt("animGraphID", this);
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
        if (!animGraph)
        {
            outResult = AZStd::string::format("Cannot undo remove group parameter. Anim graph id '%d' is not valid.", animGraphID);
            return false;
        }

        // Construct the command group.
        AZStd::string command;
        MCore::CommandGroup commandGroup;
        const AZStd::string& updateWindow = parameters.GetValue("updateUI", this);

        command = AZStd::string::format("AnimGraphAddGroupParameter -animGraphID %i -name \"%s\" -index %zu -parent \"%s\" -updateUI %s",
                animGraph->GetID(),
                mOldName.c_str(),
                mOldIndex,
                mOldParent.c_str(),
                updateWindow.c_str());
        commandGroup.AddCommandString(command);

        if (!mOldParameterNames.empty())
        {
            command = AZStd::string::format("AnimGraphAdjustGroupParameter -animGraphID %i -name \"%s\" -parameterNames \"%s\" -action \"add\" -updateUI %s",
                animGraph->GetID(),
                mOldName.c_str(),
                mOldParameterNames.c_str(),
                updateWindow.c_str());
            commandGroup.AddCommandString(command);
        }
        
        // Execute the command group.
        AZStd::string result;
        if (!GetCommandManager()->ExecuteCommandGroupInsideCommand(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }

        // Set the dirty flag back to the old value.
        animGraph->SetDirtyFlag(mOldDirtyFlag);
        return true;
    }


    void CommandAnimGraphRemoveGroupParameter::InitSyntax()
    {
        GetSyntax().ReserveParameters(3);
        GetSyntax().AddRequiredParameter("animGraphID", "The id of the blend set the group parameter belongs to.", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddRequiredParameter("name", "The name of the group parameter to remove.", MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddParameter("updateUI", "Setting this to true will trigger a refresh of the parameter UI.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "true");
    }


    const char* CommandAnimGraphRemoveGroupParameter::GetDescription() const
    {
        return "This command can be used to remove a group parameter from the given anim graph.";
    }


    //--------------------------------------------------------------------------------
    // Helper functions
    //--------------------------------------------------------------------------------
    void RemoveGroupParameter(EMotionFX::AnimGraph* animGraph, const EMotionFX::GroupParameter* groupParameter, bool removeParameters, MCore::CommandGroup* commandGroup, bool updateUI)
    {
        // Create the command group and construct the remove group parameter command.
        MCore::CommandGroup internalCommandGroup("Remove group parameter");
        MCore::CommandGroup* commandGroupToUse = commandGroup ? commandGroup : &internalCommandGroup;

        if (removeParameters)
        {
            AZStd::vector<AZStd::string> parameterNamesToBeRemoved;

            // Add all parameters from the group to the array of parameters to be removed.
            const EMotionFX::ValueParameterVector valueParameters = groupParameter->RecursivelyGetChildValueParameters();
            for (const EMotionFX::ValueParameter* valueParameter : valueParameters)
            {
                parameterNamesToBeRemoved.emplace_back(valueParameter->GetName());
            }

            // Construct remove parameter commands for all elements in the array.
            BuildRemoveParametersCommandGroup(animGraph, parameterNamesToBeRemoved, commandGroupToUse);
        }

        const AZStd::string command = AZStd::string::format("AnimGraphRemoveGroupParameter -animGraphID %i -name \"%s\" -updateUI %s", animGraph->GetID(), groupParameter->GetName().c_str(), updateUI ? "true" : "false");
        commandGroupToUse->AddCommandString(command.c_str());

        // Execute the internal command group.
        if (!commandGroup)
        {
            AZStd::string result;
            if (!GetCommandManager()->ExecuteCommandGroup(internalCommandGroup, result))
            {
                AZ_Error("EMotionFX", false, result.c_str());
            }
        }
    }


    void ClearGroupParameters(EMotionFX::AnimGraph* animGraph, MCore::CommandGroup* commandGroup)
    {
        MCore::CommandGroup internalCommandGroup("Clear group parameters");
        MCore::CommandGroup* commandGroupToUse = commandGroup ? commandGroup : &internalCommandGroup;

        // Construct remove group parameter commands for all groups and add them to the command group.
        const EMotionFX::GroupParameterVector parameters = animGraph->RecursivelyGetGroupParameters();
        for (uint32 i = 0; i < parameters.size(); ++i)
        {
            const bool updateUi = (i == 0 || i == parameters.size() - 1);
            RemoveGroupParameter(animGraph, parameters[i], false, commandGroupToUse, updateUi);
        }

        // Execute the command group.
        if (!commandGroup)
        {
            AZStd::string result;
            if (!GetCommandManager()->ExecuteCommandGroup(internalCommandGroup, result))
            {
                AZ_Error("EMotionFX", false, result.c_str());
            }
        }
    }


    void MoveGroupParameterCommand(EMotionFX::AnimGraph* animGraph, uint32 moveFrom, uint32 moveTo)
    {
        // Get the group parameter to move.
        const EMotionFX::Parameter* parameter = animGraph->FindParameter(moveFrom);

        MCore::CommandGroup commandGroup("Move command group");

        // 1: Remove the group parameter that we want to move.
        AZStd::string command;
        command = AZStd::string::format("AnimGraphRemoveGroupParameter -animGraphID %i -name \"%s\"", animGraph->GetID(), parameter->GetName().c_str());
        commandGroup.AddCommandString(command);

        // 2: Add a new group parameter at the desired position.
        command = AZStd::string::format("AnimGraphAddGroupParameter -animGraphID %i -name \"%s\" -index %d", animGraph->GetID(), parameter->GetName().c_str(), moveTo);
        commandGroup.AddCommandString(command);

        // 3: Move all parameters to the new group.
        AZ_Assert(azrtti_typeid(parameter) == azrtti_typeid<EMotionFX::GroupParameter>(), "Expected paramater group");
        AZStd::string parameterNamesString;
        const EMotionFX::ParameterVector childParameters = static_cast<const EMotionFX::GroupParameter*>(parameter)->RecursivelyGetChildParameters();
        for (const EMotionFX::Parameter* childParameter : childParameters)
        {
            parameterNamesString += childParameter->GetName() + ";";
        }
        if (!parameterNamesString.empty())
        {
            parameterNamesString.pop_back();
        }
        command = AZStd::string::format("AnimGraphAdjustGroupParameter -animGraphID %i -name \"%s\" -parameterNames \"%s\" -action \"add\"", animGraph->GetID(), parameter->GetName().c_str(), parameterNamesString.c_str());
        commandGroup.AddCommandString(command);

        // Execute the command group.
        AZStd::string result;
        if (!GetCommandManager()->ExecuteCommandGroup(commandGroup, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }
    }
} // namespace CommandSystem
