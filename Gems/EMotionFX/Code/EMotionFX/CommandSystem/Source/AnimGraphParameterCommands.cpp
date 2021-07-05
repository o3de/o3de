/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/CommandSystem/Source/AnimGraphParameterCommands.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphConnectionCommands.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>

#include <AzCore/std/sort.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphParameterCondition.h>
#include <EMotionFX/Source/AnimGraphTagCondition.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/BlendTreeParameterNode.h>
#include <EMotionFX/Source/Parameter/Parameter.h>
#include <EMotionFX/Source/Parameter/ParameterFactory.h>
#include <EMotionFX/Source/Parameter/ValueParameter.h>
#include <MCore/Source/AttributeFactory.h>
#include <MCore/Source/LogManager.h>
#include <MCore/Source/ReflectionSerializer.h>

namespace CommandSystem
{
    //-------------------------------------------------------------------------------------
    // Create a anim graph parameter
    //-------------------------------------------------------------------------------------
    CommandAnimGraphCreateParameter::CommandAnimGraphCreateParameter(MCore::Command* orgCommand)
        : MCore::Command("AnimGraphCreateParameter", orgCommand)
    {
    }


    CommandAnimGraphCreateParameter::~CommandAnimGraphCreateParameter() = default;


    bool CommandAnimGraphCreateParameter::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // Get the parameter name.
        AZStd::string name;
        parameters.GetValue("name", this, name);

        // Find the anim graph by using the id from command parameter.
        const uint32 animGraphID = parameters.GetValueAsInt("animGraphID", this);
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
        if (!animGraph)
        {
            outResult = AZStd::string::format("Cannot add parameter '%s' to anim graph. Anim graph id '%i' is not valid.", name.c_str(), animGraphID);
            return false;
        }

        // Check if the parameter name is unique and not used by any other parameter yet.
        if (animGraph->FindParameterByName(name))
        {
            outResult = AZStd::string::format("There is already a parameter with the name '%s', please use a another, unique name.", name.c_str());
            return false;
        }

        // Get the data type and check if it is a valid one.
        AZ::Outcome<AZStd::string> valueString = parameters.GetValueIfExists("type", this);
        if (!valueString.IsSuccess())
        {
            outResult = AZStd::string::format("The type was not specified. Please use -help or use the command browser to see a list of valid options.");
            return false;
        }
        const AZ::TypeId parameterType(valueString.GetValue().c_str(), valueString.GetValue().size());

        // Create the new parameter based on the dialog settings.
        AZStd::unique_ptr<EMotionFX::Parameter> newParam(EMotionFX::ParameterFactory::Create(parameterType));

        if (!newParam)
        {
            outResult = AZStd::string::format("Could not construct parameter '%s'", name.c_str());
            return false;
        }

        newParam->SetName(name);

        // Description.
        AZStd::string description;
        parameters.GetValue("description", this, description);
        newParam->SetDescription(description);

        valueString = parameters.GetValueIfExists("minValue", this);
        if (valueString.IsSuccess())
        {
            if (!MCore::ReflectionSerializer::DeserializeIntoMember(newParam.get(), "minValue", valueString.GetValue()))
            {
                outResult = AZStd::string::format("Failed to initialize minimum value from string '%s'", valueString.GetValue().c_str());
                return false;
            }
        }
        valueString = parameters.GetValueIfExists("maxValue", this);
        if (valueString.IsSuccess())
        {
            if (!MCore::ReflectionSerializer::DeserializeIntoMember(newParam.get(), "maxValue", valueString.GetValue()))
            {
                outResult = AZStd::string::format("Failed to initialize maximum value from string '%s'", valueString.GetValue().c_str());
                return false;
            }
        }
        valueString = parameters.GetValueIfExists("defaultValue", this);
        if (valueString.IsSuccess())
        {
            if (!MCore::ReflectionSerializer::DeserializeIntoMember(newParam.get(), "defaultValue", valueString.GetValue()))
            {
                outResult = AZStd::string::format("Failed to initialize default value from string '%s'", valueString.GetValue().c_str());
                return false;
            }
        }
        valueString = parameters.GetValueIfExists("contents", this);
        if (valueString.IsSuccess())
        {
            MCore::ReflectionSerializer::Deserialize(newParam.get(), valueString.GetValue());
        }

        // Check if the group parameter got specified.
        const EMotionFX::GroupParameter* parentGroupParameter = nullptr;
        valueString = parameters.GetValueIfExists("parent", this);
        if (valueString.IsSuccess())
        {
            // Find the group parameter index and get a pointer to the group parameter.
            parentGroupParameter = animGraph->FindGroupParameterByName(valueString.GetValue());
            if (!parentGroupParameter)
            {
                MCore::LogWarning("The group parameter named '%s' could not be found. The parameter cannot be added to the group.", valueString.GetValue().c_str());
            }
        }

        // The position inside the parameter array where the parameter should get added to.
        const int insertAtIndex = parameters.GetValueAsInt("index", this);
        const size_t parentGroupSize = parentGroupParameter ? parentGroupParameter->GetNumParameters() : animGraph->GetNumParameters();
        if (insertAtIndex != -1 && (insertAtIndex < 0 || insertAtIndex > static_cast<int>(parentGroupSize)))
        {
            outResult = AZStd::string::format("Cannot insert parameter at index '%d'. Index is out of range.", insertAtIndex);
            return false;
        }

        const bool paramResult = insertAtIndex == -1
            ? animGraph->AddParameter(newParam.get(), parentGroupParameter)
            : animGraph->InsertParameter(insertAtIndex, newParam.get(), parentGroupParameter);

        if (!paramResult)
        {
            outResult = AZStd::string::format("Could not add parameter: '%s.'", newParam->GetName().c_str());
            return false;
        }

        const AZ::Outcome<size_t> parameterIndex = animGraph->FindParameterIndex(newParam.get());
        AZ_Assert(parameterIndex.IsSuccess(), "Expected valid parameter index.");

        newParam.release(); // adding the parameter succeeded, release the memory because it is owned by the AnimGraph

        const EMotionFX::Parameter* param = animGraph->FindParameter(parameterIndex.GetValue());
        if (azrtti_typeid(param) != azrtti_typeid<EMotionFX::GroupParameter>())
        {
            const AZ::Outcome<size_t> valueParameterIndex = animGraph->FindValueParameterIndex(static_cast<const EMotionFX::ValueParameter*>(param));
            AZ_Assert(valueParameterIndex.IsSuccess(), "Expected valid value parameter index.");

            // Update all anim graph instances.
            const size_t numInstances = animGraph->GetNumAnimGraphInstances();
            for (size_t i = 0; i < numInstances; ++i)
            {
                EMotionFX::AnimGraphInstance* animGraphInstance = animGraph->GetAnimGraphInstance(i);
                animGraphInstance->InsertParameterValue(static_cast<uint32>(valueParameterIndex.GetValue()));
            }

            AZStd::vector<EMotionFX::AnimGraphObject*> affectedObjects;
            animGraph->RecursiveCollectObjectsOfType(azrtti_typeid<EMotionFX::ObjectAffectedByParameterChanges>(), affectedObjects);
            EMotionFX::GetAnimGraphManager().RecursiveCollectObjectsAffectedBy(animGraph, affectedObjects);

            for (EMotionFX::AnimGraphObject* affectedObject : affectedObjects)
            {
                EMotionFX::ObjectAffectedByParameterChanges* affectedObjectByParameterChanges = azdynamic_cast<EMotionFX::ObjectAffectedByParameterChanges*>(affectedObject);
                affectedObjectByParameterChanges->ParameterAdded(name);
            }
        }

        // Set the parameter name as command result.
        outResult = name.c_str();

        // Save the current dirty flag and tell the anim graph that something got changed.
        mOldDirtyFlag = animGraph->GetDirtyFlag();
        animGraph->SetDirtyFlag(true);

        return true;
    }


    bool CommandAnimGraphCreateParameter::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // Get the parameter name.
        AZStd::string name;
        parameters.GetValue("name", this, name);

        // Find the anim graph by using the id from command parameter.
        const uint32 animGraphID = parameters.GetValueAsInt("animGraphID", this);
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
        if (!animGraph)
        {
            outResult = AZStd::string::format("Cannot undo add parameter '%s' to anim graph. Anim graph id '%i' is not valid.", name.c_str(), animGraphID);
            return false;
        }

        const AZStd::string commandString = AZStd::string::format("AnimGraphRemoveParameter -animGraphID %i -name \"%s\"", animGraph->GetID(), name.c_str());

        AZStd::string result;
        if (!GetCommandManager()->ExecuteCommandInsideCommand(commandString, result))
        {
            AZ_Error("EMotionFX", false, result.c_str());
        }

        // Set the dirty flag back to the old value.
        animGraph->SetDirtyFlag(mOldDirtyFlag);
        return true;
    }


    void CommandAnimGraphCreateParameter::InitSyntax()
    {
        GetSyntax().ReserveParameters(9);
        GetSyntax().AddRequiredParameter("animGraphID", "The id of the anim graph.", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddRequiredParameter("type", "The type of this parameter (UUID).", MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddRequiredParameter("name", "The name of the parameter, which has to be unique inside the currently selected anim graph.", MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddParameter("description", "The description of the parameter.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("index", "The position where the parameter should be added. If the parameter is not specified it will get added to the end. This index is relative to the \"parent\" parameter", MCore::CommandSyntax::PARAMTYPE_INT, "-1");
        GetSyntax().AddParameter("contents", "The serialized contents of the parameter (in reflected XML).", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("parent", "The parent group name into which the parameter should be added. If not specified it will get added to the root group.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("updateUI", "Setting this to true will trigger a refresh of the parameter UI.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "true");
    }


    const char* CommandAnimGraphCreateParameter::GetDescription() const
    {
        return "This command creates a anim graph parameter with given settings. The name of the parameter is returned on success.";
    }

    //-------------------------------------------------------------------------------------
    // Remove a anim graph parameter
    //-------------------------------------------------------------------------------------
    CommandAnimGraphRemoveParameter::CommandAnimGraphRemoveParameter(MCore::Command* orgCommand)
        : MCore::Command("AnimGraphRemoveParameter", orgCommand)
    {
    }


    CommandAnimGraphRemoveParameter::~CommandAnimGraphRemoveParameter() = default;


    bool CommandAnimGraphRemoveParameter::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // Get the parameter name.
        parameters.GetValue("name", this, mName);

        // Find the anim graph by using the id from command parameter.
        const uint32 animGraphID = parameters.GetValueAsInt("animGraphID", this);
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
        if (!animGraph)
        {
            outResult = AZStd::string::format("Cannot remove parameter '%s' from anim graph. Anim graph id '%i' is not valid.", mName.c_str(), animGraphID);
            return false;
        }

        // Check if there is a parameter with the given name.
        const EMotionFX::Parameter* parameter = animGraph->FindParameterByName(mName);
        if (!parameter)
        {
            outResult = AZStd::string::format("Cannot remove parameter '%s' from anim graph. There is no parameter with the given name.", mName.c_str());
            return false;
        }
        AZ_Assert(azrtti_typeid(parameter) != azrtti_typeid<EMotionFX::GroupParameter>(), "CommmandAnimGraphRemoveParameter called for a group parameter");

        const EMotionFX::GroupParameter* parentGroup = animGraph->FindParentGroupParameter(parameter);

        const AZ::Outcome<size_t> parameterIndex = parentGroup ? parentGroup->FindRelativeParameterIndex(parameter) : animGraph->FindRelativeParameterIndex(parameter);
        AZ_Assert(parameterIndex.IsSuccess(), "Expected valid parameter index");

        // Store undo info before we remove it, so that we can recreate it later.
        mType = azrtti_typeid(parameter);
        mIndex = parameterIndex.GetValue();
        mParent = parentGroup ? parentGroup->GetName() : "";
        mContents = MCore::ReflectionSerializer::Serialize(parameter).GetValue();

        AZ::Outcome<size_t> valueParameterIndex = AZ::Failure();
        if (mType != azrtti_typeid<EMotionFX::GroupParameter>())
        {
            valueParameterIndex = animGraph->FindValueParameterIndex(static_cast<const EMotionFX::ValueParameter*>(parameter));
        }

        // Remove the parameter from the anim graph.
        if (animGraph->RemoveParameter(const_cast<EMotionFX::Parameter*>(parameter)))
        {
            // Remove the parameter from all corresponding anim graph instances if it is a value parameter
            if (mType != azrtti_typeid<EMotionFX::GroupParameter>())
            {
                AZStd::vector<EMotionFX::AnimGraphObject*> affectedObjects;
                animGraph->RecursiveCollectObjectsOfType(azrtti_typeid<EMotionFX::ObjectAffectedByParameterChanges>(), affectedObjects);
                EMotionFX::GetAnimGraphManager().RecursiveCollectObjectsAffectedBy(animGraph, affectedObjects);

                for (EMotionFX::AnimGraphObject* affectedObject : affectedObjects)
                {
                    EMotionFX::ObjectAffectedByParameterChanges* parameterDriven = azdynamic_cast<EMotionFX::ObjectAffectedByParameterChanges*>(affectedObject);
                    parameterDriven->ParameterRemoved(mName);
                }

                const size_t numInstances = animGraph->GetNumAnimGraphInstances();
                for (size_t i = 0; i < numInstances; ++i)
                {
                    EMotionFX::AnimGraphInstance* animGraphInstance = animGraph->GetAnimGraphInstance(i);
                    // Remove the parameter.
                    animGraphInstance->RemoveParameterValue(static_cast<uint32>(valueParameterIndex.GetValue()));
                }

                // Save the current dirty flag and tell the anim graph that something got changed.
                mOldDirtyFlag = animGraph->GetDirtyFlag();
                animGraph->SetDirtyFlag(true);
            }
            return true;
        }
        return false;
    }


    bool CommandAnimGraphRemoveParameter::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // Find the anim graph by using the id from command parameter.
        const uint32 animGraphID = parameters.GetValueAsInt("animGraphID", this);
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
        if (!animGraph)
        {
            outResult = AZStd::string::format("Cannot undo remove parameter '%s' from anim graph. Anim graph id '%i' is not valid.", mName.c_str(), animGraphID);
            return false;
        }

        const AZStd::string updateUI = parameters.GetValue("updateUI", this);

        // Execute the command to create the parameter again.
        AZStd::string commandString;

        commandString = AZStd::string::format("AnimGraphCreateParameter -animGraphID %i -name \"%s\" -index %zu -type \"%s\" -contents {%s} -parent \"%s\" -updateUI %s",
                animGraph->GetID(),
                mName.c_str(),
                mIndex,
                mType.ToString<AZStd::string>().c_str(),
                mContents.c_str(),
                mParent.c_str(),
                updateUI.c_str());

        // The parameter will be restored to the right parent group because the index is absolute

        // Execute the command.
        if (!GetCommandManager()->ExecuteCommandInsideCommand(commandString, outResult))
        {
            AZ_Error("EMotionFX", false, outResult.c_str());
            return false;
        }

        // Set the dirty flag back to the old value.
        animGraph->SetDirtyFlag(mOldDirtyFlag);
        return true;
    }


    void CommandAnimGraphRemoveParameter::InitSyntax()
    {
        GetSyntax().ReserveParameters(3);
        GetSyntax().AddRequiredParameter("animGraphID", "The id of the anim graph.", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddRequiredParameter("name", "The name of the parameter inside the currently selected anim graph.", MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddParameter("updateUI", "Setting this to true will trigger a refresh of the parameter UI.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "true");
    }


    const char* CommandAnimGraphRemoveParameter::GetDescription() const
    {
        return "This command removes a anim graph parameter with the specified name.";
    }


    //-------------------------------------------------------------------------------------
    // Adjust a anim graph parameter
    //-------------------------------------------------------------------------------------
    CommandAnimGraphAdjustParameter::CommandAnimGraphAdjustParameter(MCore::Command* orgCommand)
        : MCore::Command("AnimGraphAdjustParameter", orgCommand)
    {
    }


    CommandAnimGraphAdjustParameter::~CommandAnimGraphAdjustParameter() = default;


    bool CommandAnimGraphAdjustParameter::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // Get the parameter name.
        parameters.GetValue("name", this, mOldName);

        // Find the anim graph by using the id from command parameter.
        const uint32 animGraphID = parameters.GetValueAsInt("animGraphID", this);
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
        if (!animGraph)
        {
            outResult = AZStd::string::format("Cannot adjust parameter '%s'. Anim graph with id '%d' not found.", mOldName.c_str(), animGraphID);
            return false;
        }

        const EMotionFX::Parameter* parameter = animGraph->FindParameterByName(mOldName);
        if (!parameter)
        {
            outResult = AZStd::string::format("There is no parameter with the name '%s'.", mOldName.c_str());
            return false;
        }
        AZ::Outcome<size_t> oldValueParameterIndex = AZ::Failure();
        if (azrtti_istypeof<EMotionFX::ValueParameter*>(parameter))
        {
            const EMotionFX::ValueParameter* valueParameter = static_cast<const EMotionFX::ValueParameter*>(parameter);
            oldValueParameterIndex = animGraph->FindValueParameterIndex(valueParameter);
        }

        const EMotionFX::GroupParameter* currentParent = animGraph->FindParentGroupParameter(parameter);

        // Store the undo info.
        mOldType = azrtti_typeid(parameter);
        mOldContents = MCore::ReflectionSerializer::Serialize(parameter).GetValue();

        // Get the new name and check if it is valid.
        AZStd::string newName;
        parameters.GetValue("newName", this, newName);
        if (!newName.empty())
        {
            if (newName == mOldName)
            {
                newName.clear();
            }
            else if (animGraph->FindParameterByName(newName))
            {
                outResult = AZStd::string::format("There is already a parameter with the name '%s', please use a unique name.", newName.c_str());
                return false;
            }
        }
        if (!newName.empty())
        {
            EMotionFX::Parameter* editableParameter = const_cast<EMotionFX::Parameter*>(parameter);
            animGraph->RenameParameter(editableParameter, newName);
        }

        // Get the data type and check if it is a valid one.
        bool changedType = false;
        AZ::Outcome<AZStd::string> valueString = parameters.GetValueIfExists("type", this);
        if (valueString.IsSuccess())
        {
            const AZ::TypeId type = AZ::TypeId::CreateString(valueString.GetValue().c_str(), valueString.GetValue().size());
            if (type.IsNull())
            {
                outResult = AZStd::string::format("The type is not a valid UUID type. Please use -help or use the command browser to see a list of valid options.");
                return false;
            }
            if (type != mOldType)
            {
                AZStd::unique_ptr<EMotionFX::Parameter> newParameter(EMotionFX::ParameterFactory::Create(type));
                newParameter->SetName(newName.empty() ? mOldName : newName);
                newParameter->SetDescription(parameter->GetDescription());

                const AZ::Outcome<size_t> paramIndexRelativeToParent = currentParent ? currentParent->FindRelativeParameterIndex(parameter) : animGraph->FindRelativeParameterIndex(parameter);
                AZ_Assert(paramIndexRelativeToParent.IsSuccess(), "Expected parameter to be in the parent");

                if (!animGraph->RemoveParameter(const_cast<EMotionFX::Parameter*>(parameter)))
                {
                    outResult = AZStd::string::format("Could not remove current parameter '%s' to change its type.", mOldName.c_str());
                    return false;
                }
                if (!animGraph->InsertParameter(paramIndexRelativeToParent.GetValue(), newParameter.get(), currentParent))
                {
                    outResult = AZStd::string::format("Could not insert new parameter '%s' to change its type.", newName.c_str());
                    return false;
                }
                parameter = newParameter.release();
                changedType = true;
            }
        }

        EMotionFX::Parameter* mutableParam = const_cast<EMotionFX::Parameter*>(parameter);
        // Get the value strings.
        valueString = parameters.GetValueIfExists("minValue", this);
        if (valueString.IsSuccess())
        {
            MCore::ReflectionSerializer::DeserializeIntoMember(mutableParam, "minValue", valueString.GetValue());
        }
        valueString = parameters.GetValueIfExists("maxValue", this);
        if (valueString.IsSuccess())
        {
            MCore::ReflectionSerializer::DeserializeIntoMember(mutableParam, "maxValue", valueString.GetValue());
        }
        valueString = parameters.GetValueIfExists("defaultValue", this);
        if (valueString.IsSuccess())
        {
            MCore::ReflectionSerializer::DeserializeIntoMember(mutableParam, "defaultValue", valueString.GetValue());
        }
        valueString = parameters.GetValueIfExists("description", this);
        if (valueString.IsSuccess())
        {
            MCore::ReflectionSerializer::DeserializeIntoMember(mutableParam, "description", valueString.GetValue());
        }
        valueString = parameters.GetValueIfExists("contents", this);
        if (valueString.IsSuccess())
        {
            MCore::ReflectionSerializer::Deserialize(mutableParam, valueString.GetValue());
        }

        if (azrtti_istypeof<EMotionFX::ValueParameter>(parameter))
        {
            const EMotionFX::ValueParameter* valueParameter = static_cast<const EMotionFX::ValueParameter*>(parameter);
            const AZ::Outcome<size_t> valueParameterIndex = animGraph->FindValueParameterIndex(valueParameter);
            AZ_Assert(valueParameterIndex.IsSuccess(), "Expect a valid value parameter index");

            // Update all corresponding anim graph instances.
            const size_t numInstances = animGraph->GetNumAnimGraphInstances();
            for (uint32 i = 0; i < numInstances; ++i)
            {
                EMotionFX::AnimGraphInstance* animGraphInstance = animGraph->GetAnimGraphInstance(i);
                // reinit the modified parameters
                if (mOldType != azrtti_typeid<EMotionFX::GroupParameter>())
                {
                    animGraphInstance->ReInitParameterValue(static_cast<uint32>(valueParameterIndex.GetValue()));
                }
                else
                {
                    animGraphInstance->AddMissingParameterValues();
                }
            }

            // Apply the name change., only required to do it if the type didn't change
            if (!changedType)
            {
                if (!newName.empty())
                {
                    AZStd::vector<EMotionFX::AnimGraphObject*> affectedObjects;
                    animGraph->RecursiveCollectObjectsOfType(azrtti_typeid<EMotionFX::ObjectAffectedByParameterChanges>(), affectedObjects);
                    EMotionFX::GetAnimGraphManager().RecursiveCollectObjectsAffectedBy(animGraph, affectedObjects);

                    for (EMotionFX::AnimGraphObject* affectedObject : affectedObjects)
                    {
                        EMotionFX::ObjectAffectedByParameterChanges* affectedObjectByParameterChanges = azdynamic_cast<EMotionFX::ObjectAffectedByParameterChanges*>(affectedObject);
                        affectedObjectByParameterChanges->ParameterRenamed(mOldName, newName);
                    }
                }
            }
            else
            {
                // Changed the type, should be treated as remove/add
                AZStd::vector<EMotionFX::AnimGraphObject*> affectedObjects;
                animGraph->RecursiveCollectObjectsOfType(azrtti_typeid<EMotionFX::ObjectAffectedByParameterChanges>(), affectedObjects);
                EMotionFX::GetAnimGraphManager().RecursiveCollectObjectsAffectedBy(animGraph, affectedObjects);

                for (EMotionFX::AnimGraphObject* affectedObject : affectedObjects)
                {
                    EMotionFX::ObjectAffectedByParameterChanges* affectedObjectByParameterChanges = azdynamic_cast<EMotionFX::ObjectAffectedByParameterChanges*>(affectedObject);
                    affectedObjectByParameterChanges->ParameterRemoved(mOldName);
                    affectedObjectByParameterChanges->ParameterAdded(newName);
                }
            }

            // Save the current dirty flag and tell the anim graph that something got changed.
            mOldDirtyFlag = animGraph->GetDirtyFlag();
            animGraph->SetDirtyFlag(true);
        }
        else if (mOldType != azrtti_typeid<EMotionFX::GroupParameter>())
        {
            AZ_Assert(oldValueParameterIndex.IsSuccess(), "Unable to find parameter index when changing parameter to a group");

            // Changed the type, should be treated as remove/add
            AZStd::vector<EMotionFX::AnimGraphObject*> affectedObjects;
            animGraph->RecursiveCollectObjectsOfType(azrtti_typeid<EMotionFX::ObjectAffectedByParameterChanges>(), affectedObjects);
            EMotionFX::GetAnimGraphManager().RecursiveCollectObjectsAffectedBy(animGraph, affectedObjects);

            for (EMotionFX::AnimGraphObject* affectedObject : affectedObjects)
            {
                EMotionFX::ObjectAffectedByParameterChanges* affectedObjectByParameterChanges = azdynamic_cast<EMotionFX::ObjectAffectedByParameterChanges*>(affectedObject);
                affectedObjectByParameterChanges->ParameterRemoved(mOldName);
            }

            // Save the current dirty flag and tell the anim graph that something got changed.
            mOldDirtyFlag = animGraph->GetDirtyFlag();
            animGraph->SetDirtyFlag(true);
        }

        return true;
    }


    bool CommandAnimGraphAdjustParameter::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        // Find the anim graph by using the id from command parameter.
        const uint32 animGraphID = parameters.GetValueAsInt("animGraphID", this);
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
        if (!animGraph)
        {
            outResult = AZStd::string::format("Cannot adjust parameter to anim graph. Anim graph id '%d' is not valid.", animGraphID);
            return false;
        }

        // get the name and check if it is unique
        AZStd::string name;
        AZStd::string newName;
        parameters.GetValue("name", this, name);
        parameters.GetValue("newName", this, newName);

        // Get the parameter index.
        AZ::Outcome<size_t> paramIndex = animGraph->FindParameterIndexByName(name.c_str());
        if (!paramIndex.IsSuccess())
        {
            paramIndex = animGraph->FindParameterIndexByName(newName.c_str());
        }

        // If the neither the former nor the new parameter exist, return.
        if (!paramIndex.IsSuccess())
        {
            if (newName.empty())
            {
                outResult = AZStd::string::format("There is no parameter with the name '%s'.", name.c_str());
            }
            else
            {
                outResult = AZStd::string::format("There is no parameter with the name '%s'.", newName.c_str());
            }

            return false;
        }

        // Construct and execute the command.
        const AZStd::string commandString = AZStd::string::format("AnimGraphAdjustParameter -animGraphID %i -name \"%s\" -newName \"%s\" -type \"%s\" -contents {%s}",
                animGraph->GetID(),
                newName.c_str(),
                name.c_str(),
                mOldType.ToString<AZStd::string>().c_str(),
                mOldContents.c_str());

        if (!GetCommandManager()->ExecuteCommandInsideCommand(commandString, outResult))
        {
            AZ_Error("EMotionFX", false, outResult.c_str());
        }

        animGraph->SetDirtyFlag(mOldDirtyFlag);
        return true;
    }


    void CommandAnimGraphAdjustParameter::InitSyntax()
    {
        GetSyntax().ReserveParameters(10);
        GetSyntax().AddRequiredParameter("animGraphID", "The id of the anim graph.", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddRequiredParameter("name", "The name of the parameter inside the currently selected anim graph to modify.", MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddParameter("type", "The new type (UUID).", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("newName", "The new name of the parameter.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("defaultValue", "The new default value of the parameter.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("minValue", "The new minimum value of the parameter. In case of checkboxes or strings this parameter value will be ignored.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("maxValue", "The new maximum value of the parameter. In case of checkboxes or strings this parameter value will be ignored.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("description", "The new description of the parameter.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("contents", "The contents of the parameter (serialized reflected XML)", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("updateUI", "Setting this to true will trigger a refresh of the parameter UI.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "true");
    }


    const char* CommandAnimGraphAdjustParameter::GetDescription() const
    {
        return "This command adjusts a anim graph parameter with given settings.";
    }


    //-------------------------------------------------------------------------------------
    // Move a parameter to another position
    //-------------------------------------------------------------------------------------
    CommandAnimGraphMoveParameter::CommandAnimGraphMoveParameter(MCore::Command* orgCommand)
        : MCore::Command("AnimGraphMoveParameter", orgCommand)
    {
    }


    CommandAnimGraphMoveParameter::~CommandAnimGraphMoveParameter() = default;


    bool CommandAnimGraphMoveParameter::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZStd::string name;
        parameters.GetValue("name", this, name);

        // Find the anim graph by using the id from command parameter.
        const uint32 animGraphID = parameters.GetValueAsInt("animGraphID", this);
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
        if (!animGraph)
        {
            outResult = AZStd::string::format("Cannot swap parameters. Anim graph id '%i' is not valid.", animGraphID);
            return false;
        }

        AZ::Outcome<AZStd::string> parent = parameters.GetValueIfExists("parent", this);

        // if parent is empty, the parameter is being moved (or already in) the root group
        const EMotionFX::GroupParameter* destinationParent = animGraph->FindGroupParameterByName(parent.IsSuccess() ? parent.GetValue() : "");
        if (!destinationParent)
        {
            outResult = AZStd::string::format("Could not find destination parent \"%s\"", parent.GetValue().c_str());
            return false;
        }

        int32 destinationIndex = parameters.GetValueAsInt("index", this);
        const EMotionFX::ParameterVector& siblingDestinationParamenters = destinationParent->GetChildParameters();
        if (destinationIndex < 0 || destinationIndex > siblingDestinationParamenters.size())
        {
            outResult = AZStd::string::format("Index %d is out of bounds for parent \"%s\"", destinationIndex, parent.GetValue().c_str());
            return false;
        }

        const EMotionFX::Parameter* parameter = animGraph->FindParameterByName(name);
        if (!parameter)
        {
            outResult = AZStd::string::format("There is no parameter with the name '%s'.", name.c_str());
            return false;
        }

        // Get the current data for undo
        const EMotionFX::GroupParameter* currentParent = animGraph->FindParentGroupParameter(parameter);
        if (currentParent)
        {
            mOldParent = currentParent->GetName();
            mOldIndex = currentParent->FindRelativeParameterIndex(parameter).GetValue();
        }
        else
        {
            mOldParent.clear(); // means the root
            mOldIndex = animGraph->FindRelativeParameterIndex(parameter).GetValue();
        }

        EMotionFX::ValueParameterVector valueParametersBeforeChange = animGraph->RecursivelyGetValueParameters();

        // If the parameter being moved is a value parameter (not a group), we need to update the anim graph instances and
        // nodes. To do so, we need to get the absolute index of the parameter before and after the move.
        AZ::Outcome<size_t> valueIndexBeforeMove = AZ::Failure();
        AZ::Outcome<size_t> valueIndexAfterMove = AZ::Failure();
        const bool isValueParameter = azrtti_typeid(parameter) != azrtti_typeid<EMotionFX::GroupParameter>();
        if (isValueParameter)
        {
            valueIndexBeforeMove = animGraph->FindValueParameterIndex(static_cast<const EMotionFX::ValueParameter*>(parameter));
        }

        if (!animGraph->TakeParameterFromParent(parameter))
        {
            outResult = AZStd::string::format("Could not remove the parameter \"%s\" from its parent", name.c_str());
            return false;
        }
        animGraph->InsertParameter(destinationIndex, const_cast<EMotionFX::Parameter*>(parameter), destinationParent);

        if (isValueParameter)
        {
            valueIndexAfterMove = animGraph->FindValueParameterIndex(static_cast<const EMotionFX::ValueParameter*>(parameter));
        }

        if (!isValueParameter || valueIndexAfterMove.GetValue() == valueIndexBeforeMove.GetValue())
        {
            // Nothing else to do, the anim graph instances and nodes dont require an update
            return true;
        }

        // Remove the parameter from all corresponding anim graph instances if it is a value parameter
        const size_t numInstances = animGraph->GetNumAnimGraphInstances();
        for (size_t i = 0; i < numInstances; ++i)
        {
            EMotionFX::AnimGraphInstance* animGraphInstance = animGraph->GetAnimGraphInstance(i);
            // Move the parameter from original position to the new position
            animGraphInstance->MoveParameterValue(static_cast<uint32>(valueIndexBeforeMove.GetValue()), static_cast<uint32>(valueIndexAfterMove.GetValue()));
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

        // Save the current dirty flag and tell the anim graph that something got changed.
        mOldDirtyFlag = animGraph->GetDirtyFlag();
        animGraph->SetDirtyFlag(true);

        return true;
    }


    bool CommandAnimGraphMoveParameter::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZStd::string name;
        parameters.GetValue("name", this, name);

        // Find the anim graph by using the id from command parameter.
        const uint32 animGraphID = parameters.GetValueAsInt("animGraphID", this);
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(animGraphID);
        if (!animGraph)
        {
            outResult = AZStd::string::format("Cannot undo move parameters. Anim graph id '%i' is not valid.", animGraphID);
            return false;
        }

        AZStd::string commandString = AZStd::string::format("AnimGraphMoveParameter -animGraphID %i -name \"%s\" -index %zu",
                animGraphID,
                name.c_str(),
                mOldIndex);
        if (!mOldParent.empty())
        {
            commandString += AZStd::string::format(" -parent \"%s\"", mOldParent.c_str());
        }

        if (!GetCommandManager()->ExecuteCommandInsideCommand(commandString, outResult))
        {
            AZ_Error("EMotionFX", false, outResult.c_str());
            return false;
        }

        animGraph->SetDirtyFlag(mOldDirtyFlag);
        return true;
    }


    void CommandAnimGraphMoveParameter::InitSyntax()
    {
        GetSyntax().ReserveParameters(5);
        GetSyntax().AddRequiredParameter("animGraphID", "The id of the anim graph.", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddRequiredParameter("name", "The name of the parameter to move.", MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddRequiredParameter("index", "The new index of the parameter, relative to the new parent", MCore::CommandSyntax::PARAMTYPE_INT);
        GetSyntax().AddParameter("parent", "The new parent of the parameter.", MCore::CommandSyntax::PARAMTYPE_STRING, "");
        GetSyntax().AddParameter("updateUI", "Setting this to true will trigger a refresh of the parameter UI.", MCore::CommandSyntax::PARAMTYPE_BOOLEAN, "true");
    }


    const char* CommandAnimGraphMoveParameter::GetDescription() const
    {
        return "This command moves a parameter to another place in the parameter hierarchy.";
    }

    //--------------------------------------------------------------------------------
    // Helper functions
    //--------------------------------------------------------------------------------

    //--------------------------------------------------------------------------------
    // Construct create parameter command strings
    //--------------------------------------------------------------------------------
    void ConstructCreateParameterCommand(AZStd::string& outResult, EMotionFX::AnimGraph* animGraph, const EMotionFX::Parameter* parameter, uint32 insertAtIndex)
    {
        // Build the command string.
        AZStd::string parameterContents;
        parameterContents = MCore::ReflectionSerializer::Serialize(parameter).GetValue();

        outResult = AZStd::string::format("AnimGraphCreateParameter -animGraphID %i -type \"%s\" -name \"%s\" -contents {%s}",
                animGraph->GetID(),
                azrtti_typeid(parameter).ToString<AZStd::string>().c_str(),
                parameter->GetName().c_str(),
                parameterContents.c_str());

        if (insertAtIndex != MCORE_INVALIDINDEX32)
        {
            outResult += AZStd::string::format(" -index \"%i\"", insertAtIndex);
        }
    }


    //--------------------------------------------------------------------------------
    // Remove and or clear parameter helper functions
    //--------------------------------------------------------------------------------
    void ClearParametersCommand(EMotionFX::AnimGraph* animGraph, MCore::CommandGroup* commandGroup)
    {
        const EMotionFX::ValueParameterVector parameters = animGraph->RecursivelyGetValueParameters();
        if (parameters.empty())
        {
            return;
        }

        // Iterate through all parameters and fill the parameter name array.
        AZStd::vector<AZStd::string> parameterNames;
        parameterNames.reserve(static_cast<size_t>(parameters.size()));
        for (const EMotionFX::ValueParameter* parameter : parameters)
        {
            parameterNames.push_back(parameter->GetName());
        }

        // Is the command group parameter set?
        if (!commandGroup)
        {
            // Create and fill the command group.
            AZStd::string outResult;
            MCore::CommandGroup internalCommandGroup("Clear parameters");
            BuildRemoveParametersCommandGroup(animGraph, parameterNames, &internalCommandGroup);

            // Execute the command group.
            if (!GetCommandManager()->ExecuteCommandGroup(internalCommandGroup, outResult))
            {
                AZ_Error("EMotionFX", false, outResult.c_str());
            }
        }
        else
        {
            // Use the given parameter command group.
            BuildRemoveParametersCommandGroup(animGraph, parameterNames, commandGroup);
        }
    }

    void RemoveConnectionsForParameter(EMotionFX::AnimGraph* animGraph, const char* parameterName, MCore::CommandGroup& commandGroup)
    {
        AZStd::vector<EMotionFX::AnimGraphNode*> parameterNodes;
        animGraph->RecursiveCollectNodesOfType(azrtti_typeid<EMotionFX::BlendTreeParameterNode>(), &parameterNodes);

        AZStd::vector<AZStd::pair<EMotionFX::BlendTreeConnection*, EMotionFX::AnimGraphNode*>> outgoingConnectionsFromThisPort;
        for (const EMotionFX::AnimGraphNode* parameterNode : parameterNodes)
        {
            const AZ::u32 sourcePortIndex = parameterNode->FindOutputPortIndex(parameterName);
            parameterNode->CollectOutgoingConnections(outgoingConnectionsFromThisPort, sourcePortIndex); // outgoingConnectionsFromThisPort will be cleared inside the function.
            const size_t numConnections = outgoingConnectionsFromThisPort.size();

            for (uint32 i = 0; i < numConnections; ++i)
            {
                const EMotionFX::AnimGraphNode* targetNode = outgoingConnectionsFromThisPort[i].second;
                const EMotionFX::BlendTreeConnection* connection = outgoingConnectionsFromThisPort[i].first;
                const bool updateUniqueData = (i == 0 || i == numConnections - 1);
                DeleteNodeConnection(&commandGroup, targetNode, connection, updateUniqueData);
            }
        }
    }

    // Remove all connections linked to parameter nodes inside blend trees for the parameters about to be removed back to front.
    void RemoveConnectionsForParameters(EMotionFX::AnimGraph* animGraph, const AZStd::vector<AZStd::string>& parameterNames, MCore::CommandGroup& commandGroup)
    {
        const size_t numValueParameters = animGraph->GetNumValueParameters();
        for (size_t i = 0; i < numValueParameters; ++i)
        {
            const size_t valueParameterIndex = numValueParameters - i - 1;
            const EMotionFX::ValueParameter* valueParameter = animGraph->FindValueParameter(valueParameterIndex);
            AZ_Assert(valueParameter, "Value parameter with index %d not found.", valueParameterIndex);

            if (AZStd::find(parameterNames.begin(), parameterNames.end(), valueParameter->GetName().c_str()) != parameterNames.end())
            {
                RemoveConnectionsForParameter(animGraph, valueParameter->GetName().c_str(), commandGroup);
            }
        }
    }

    bool BuildRemoveParametersCommandGroup(EMotionFX::AnimGraph* animGraph, const AZStd::vector<AZStd::string>& parameterNamesToRemove, MCore::CommandGroup* commandGroup)
    {
        // Make sure the anim graph is valid and that the parameter names array at least contains a single one.
        if (!animGraph || parameterNamesToRemove.empty())
        {
            return false;
        }

        // Create the command group.
        AZStd::string outResult;
        AZStd::string commandString;

        MCore::CommandGroup internalCommandGroup(commandString.c_str());
        MCore::CommandGroup* usedCommandGroup = commandGroup;
        if (!commandGroup)
        {
            if (parameterNamesToRemove.size() == 1)
            {
                commandString = AZStd::string::format("Remove parameter '%s'", parameterNamesToRemove[0].c_str());
            }
            else
            {
                commandString = AZStd::string::format("Remove %zu parameters", parameterNamesToRemove.size());
            }

            usedCommandGroup = &internalCommandGroup;
        }

        // 1. Remove all connections linked to parameter nodes inside blend trees for the parameters about to be removed back to front.
        RemoveConnectionsForParameters(animGraph, parameterNamesToRemove, *usedCommandGroup);

        // 2. Inform all objects affected that we are going to remove a parameter and let them make sure to add all necessary commands to prepare for it.
        AZStd::vector<EMotionFX::AnimGraphObject*> affectedObjects;
        animGraph->RecursiveCollectObjectsOfType(azrtti_typeid<EMotionFX::ObjectAffectedByParameterChanges>(), affectedObjects);
        EMotionFX::GetAnimGraphManager().RecursiveCollectObjectsAffectedBy(animGraph, affectedObjects);
        for (EMotionFX::AnimGraphObject* object : affectedObjects)
        {
            EMotionFX::ObjectAffectedByParameterChanges* affectedObject = azdynamic_cast<EMotionFX::ObjectAffectedByParameterChanges*>(object);
            AZ_Assert(affectedObject != nullptr, "Can't cast object. Object must be inherited from ObjectAffectedByParameterChanges.");

            for (const AZStd::string& parameterName : parameterNamesToRemove)
            {
                affectedObject->BuildParameterRemovedCommands(*usedCommandGroup, parameterName);
            }
        }

        // 3. Remove the actual parameters.
        size_t numIterations = parameterNamesToRemove.size();
        for (uint32 i = 0; i < numIterations; ++i)
        {
            commandString = AZStd::string::format("AnimGraphRemoveParameter -animGraphID %i -name \"%s\"", animGraph->GetID(), parameterNamesToRemove[i].c_str());
            if (i != 0 && i != numIterations - 1)
            {
                commandString += " -updateUI false";
            }
            usedCommandGroup->AddCommandString(commandString);
        }

        
        if (!commandGroup)
        {
            if (!GetCommandManager()->ExecuteCommandGroup(internalCommandGroup, outResult))
            {
                AZ_Error("EMotionFX", false, outResult.c_str());
                return false;
            }
        }

        return true;
    }
} // namespace CommandSystem
