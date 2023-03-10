/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AnimGraphConditionCommands.h" 
#include "AnimGraphConnectionCommands.h"

#include <MCore/Source/ReflectionSerializer.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphObjectFactory.h>
#include <EMotionFX/Source/AnimGraphStateMachine.h>
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>

namespace CommandSystem
{
    AZ_CLASS_ALLOCATOR_IMPL(CommandAddTransitionCondition, EMotionFX::CommandAllocator)
    const char* CommandAddTransitionCondition::s_commandName = "AnimGraphAddCondition";
    const char* CommandAddTransitionCondition::s_conditionTypeParameterName = "conditionType";
    const char* CommandAddTransitionCondition::s_insertAtParameterName = "insertAt";

    CommandAddTransitionCondition::CommandAddTransitionCondition(MCore::Command* orgCommand)
        : MCore::Command(s_commandName, orgCommand)
    {
    }

    CommandAddTransitionCondition::CommandAddTransitionCondition(
        AZ::u32 animGraphId,
        EMotionFX::AnimGraphConnectionId transitionId,
        AZ::TypeId conditionType,
        AZStd::optional<size_t> insertAt,
        AZStd::optional<AZStd::string> contents,
        Command* orgCommand)
        : MCore::Command(s_commandName, orgCommand)
        , m_insertAt(insertAt)
        , m_conditionType(conditionType)
    {
        SetAnimGraphId(animGraphId);
        SetTransitionId(transitionId);
        SetContents(contents);
    }

    void CommandAddTransitionCondition::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<CommandAddTransitionCondition,
                            MCore::Command,
                            EMotionFX::ParameterMixinTransitionId,
                            EMotionFX::ParameterMixinSerializedContents>()
            ->Version(1)
            ->Field("insertAt", &CommandAddTransitionCondition::m_insertAt)
            ->Field("conditionType", &CommandAddTransitionCondition::m_conditionType)
            ;
    }

    bool CommandAddTransitionCondition::Execute([[maybe_unused]] const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        EMotionFX::AnimGraphStateTransition* transition = GetTransition(outResult);
        if (!transition)
        {
            return false;
        }
        EMotionFX::AnimGraph* animGraph = transition->GetAnimGraph();

        EMotionFX::AnimGraphTransitionCondition* newCondition = nullptr;
        if (m_conditionType)
        {
            newCondition = azdynamic_cast<EMotionFX::AnimGraphTransitionCondition*>(EMotionFX::AnimGraphObjectFactory::Create(m_conditionType.value(), animGraph));
        }
        if (!newCondition)
        {
            outResult = AZStd::string::format("Condition object invalid. The given transition type is either invalid or no object has been registered with type %s.",
                m_conditionType.value().ToString<AZStd::string>().c_str());
            return false;
        }

        // Deserialize the contents directly, else we might be overwriting things in the end.
        if (m_contents)
        {
            MCore::ReflectionSerializer::Deserialize(newCondition, m_contents.value());
        }

        // Redo mode
        if (m_oldContents)
        {
            MCore::ReflectionSerializer::Deserialize(newCondition, m_oldContents.value());
        }

        // Get the location and add the new condition.
        if (m_insertAt && m_insertAt.value() < transition->GetNumConditions())
        {
            transition->InsertCondition(newCondition, m_insertAt.value());
        }
        else
        {
            transition->AddCondition(newCondition);
        }

        // store information for undo
        m_oldConditionIndex = transition->FindConditionIndex(newCondition);
        AZ_Assert(m_oldConditionIndex.IsSuccess(), "We should be able to find the newly added condition index.");

        // save the current dirty flag and tell the anim graph that something got changed
        m_oldDirtyFlag = animGraph->GetDirtyFlag();
        animGraph->SetDirtyFlag(true);

        // set the command result to the transition id and return success
        outResult = m_transitionId.ToString().c_str();
        m_oldContents.reset();

        newCondition->Reinit();
        animGraph->RecursiveInvalidateUniqueDatas();

        return true;
    }

    bool CommandAddTransitionCondition::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZ_UNUSED(parameters);

        EMotionFX::AnimGraphStateTransition* transition = GetTransition(outResult);
        if (!transition)
        {
            return false;
        }
        EMotionFX::AnimGraph* animGraph = transition->GetAnimGraph();

        if (!m_oldConditionIndex.IsSuccess())
        {
            outResult = "Cannot remove condition as its former index is invalid.";
            return false;
        }
        EMotionFX::AnimGraphTransitionCondition* condition = transition->GetCondition(m_oldConditionIndex.GetValue());

        // store the attributes string for redo
        m_oldContents = MCore::ReflectionSerializer::Serialize(condition).GetValue();

        CommandRemoveTransitionCondition* removeConditionCommand = aznew CommandRemoveTransitionCondition(animGraph->GetID(), transition->GetId(), m_oldConditionIndex.GetValue());
        if (!GetCommandManager()->ExecuteCommandInsideCommand(removeConditionCommand, outResult))
        {
            return false;
        }

        // set the dirty flag back to the old value
        animGraph->SetDirtyFlag(m_oldDirtyFlag.value());
        return true;
    }

    const char* CommandAddTransitionCondition::GetDescription() const
    {
        return "Add a new a transition condition to a state machine transition.";
    }

    void CommandAddTransitionCondition::InitSyntax()
    {
        GetSyntax().ReserveParameters(5);

        MCore::CommandSyntax& syntax = GetSyntax();
        ParameterMixinTransitionId::InitSyntax(syntax);
        ParameterMixinSerializedContents::InitSyntax(syntax);

        syntax.AddRequiredParameter(s_conditionTypeParameterName, "The type id of the transition condition to add.", MCore::CommandSyntax::PARAMTYPE_STRING);
        syntax.AddParameter(s_insertAtParameterName, "The index at which the transition condition will be added.", MCore::CommandSyntax::PARAMTYPE_INT, "-1");
    }

    bool CommandAddTransitionCondition::SetCommandParameters(const MCore::CommandLine& parameters)
    {
        ParameterMixinTransitionId::SetCommandParameters(parameters);
        ParameterMixinSerializedContents::SetCommandParameters(parameters);

        if (parameters.CheckIfHasParameter(s_conditionTypeParameterName))
        {
            const AZStd::string& typeIdString = parameters.GetValue(s_conditionTypeParameterName, this);
            m_conditionType = AZ::TypeId::CreateString(typeIdString.c_str(), typeIdString.size());
        }

        if (parameters.CheckIfHasParameter(s_insertAtParameterName))
        {
            m_insertAt = parameters.GetValueAsInt(s_insertAtParameterName, this);
        }

        return true;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    AZ_CLASS_ALLOCATOR_IMPL(CommandRemoveTransitionCondition, EMotionFX::CommandAllocator)
    const char* CommandRemoveTransitionCondition::s_commandName = "AnimGraphRemoveCondition";

    CommandRemoveTransitionCondition::CommandRemoveTransitionCondition(MCore::Command* orgCommand)
        : MCore::Command(s_commandName, orgCommand)
    {
    }

    CommandRemoveTransitionCondition::CommandRemoveTransitionCondition(AZ::u32 animGraphId, EMotionFX::AnimGraphConnectionId transitionId, size_t conditionIndex, Command* orgCommand)
        : MCore::Command(s_commandName, orgCommand)
    {
        SetAnimGraphId(animGraphId);
        SetTransitionId(transitionId);
        SetConditionIndex(conditionIndex);
    }

    void CommandRemoveTransitionCondition::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<CommandAddTransitionCondition,
            MCore::Command,
            EMotionFX::ParameterMixinTransitionId,
            EMotionFX::ParameterMixinSerializedContents>()
            ->Version(1);
    }

    bool CommandRemoveTransitionCondition::Execute([[maybe_unused]] const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        EMotionFX::AnimGraphTransitionCondition* condition = GetCondition(outResult);
        if (!condition)
        {
            return false;
        }
        EMotionFX::AnimGraphStateTransition* transition = condition->GetTransition();
        EMotionFX::AnimGraph* animGraph = transition->GetAnimGraph();

        // remove all unique datas for the condition
        animGraph->RemoveAllObjectData(condition, true);

        // store information for undo
        m_oldConditionType = azrtti_typeid(condition);
        m_oldContents = MCore::ReflectionSerializer::Serialize(condition).GetValue();

        // remove the transition condition
        transition->RemoveCondition(GetConditionIndex().value());

        // save the current dirty flag and tell the anim graph that something got changed
        m_oldDirtyFlag = animGraph->GetDirtyFlag();
        animGraph->SetDirtyFlag(true);

        animGraph->RecursiveInvalidateUniqueDatas();

        return true;
    }

    bool CommandRemoveTransitionCondition::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZ_UNUSED(parameters);

        EMotionFX::AnimGraphStateTransition* transition = GetTransition(outResult);
        if (!transition)
        {
            return false;
        }
        EMotionFX::AnimGraph* animGraph = transition->GetAnimGraph();

        CommandAddTransitionCondition* addConditionCommand = aznew CommandAddTransitionCondition(
            animGraph->GetID(),
            transition->GetId(),
            m_oldConditionType.value(),
            /*insertAt=*/GetConditionIndex().value(),
            m_oldContents.value());
        if (!GetCommandManager()->ExecuteCommandInsideCommand(addConditionCommand, outResult))
        {
            return false;
        }

        // set the dirty flag back to the old value
        animGraph->SetDirtyFlag(m_oldDirtyFlag.value());
        return true;
    }

    const char* CommandRemoveTransitionCondition::GetDescription() const
    {
        return "Remove a transition condition from a state machine transition.";
    }

    void CommandRemoveTransitionCondition::InitSyntax()
    {
        GetSyntax().ReserveParameters(3);
        MCore::CommandSyntax& syntax = GetSyntax();
        ParameterMixinConditionIndex::InitSyntax(syntax);
    }

    bool CommandRemoveTransitionCondition::SetCommandParameters(const MCore::CommandLine& parameters)
    {
        ParameterMixinConditionIndex::SetCommandParameters(parameters);
        return true;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    AZ_CLASS_ALLOCATOR_IMPL(CommandAdjustTransitionCondition, EMotionFX::CommandAllocator)
    const char* CommandAdjustTransitionCondition::s_commandName = "AnimGraphAdjustCondition";

    CommandAdjustTransitionCondition::CommandAdjustTransitionCondition(MCore::Command* orgCommand)
        : MCore::Command(s_commandName, orgCommand)
    {
    }

    CommandAdjustTransitionCondition::CommandAdjustTransitionCondition(AZ::u32 animGraphId, EMotionFX::AnimGraphConnectionId transitionId, size_t conditionIndex, const AZStd::string& attributesString, Command* orgCommand)
        : MCore::Command(s_commandName, orgCommand)
    {
        SetAnimGraphId(animGraphId);
        SetTransitionId(transitionId);
        SetConditionIndex(conditionIndex);
        SetAttributesString(attributesString);
    }

    void CommandAdjustTransitionCondition::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<CommandAdjustTransitionCondition,
            MCore::Command,
            EMotionFX::ParameterMixinConditionIndex,
            EMotionFX::ParameterMixinAttributesString>()
            ->Version(1)
            ;
    }

    bool CommandAdjustTransitionCondition::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZ_UNUSED(parameters);

        EMotionFX::AnimGraphTransitionCondition* condition = GetCondition(outResult);
        if (!condition)
        {
            return false;
        }
        EMotionFX::AnimGraphStateTransition* transition = condition->GetTransition();
        EMotionFX::AnimGraph* animGraph = transition->GetAnimGraph();

        AZ::Outcome<AZStd::string> serializedCondition = MCore::ReflectionSerializer::Serialize(condition);
        if (serializedCondition)
        {
            m_oldContents = serializedCondition.GetValue();
        }

        if (m_attributesString.has_value())
        {
            MCore::ReflectionSerializer::Deserialize(condition, MCore::CommandLine(m_attributesString.value()));
        }

        // save the current dirty flag and tell the anim graph that something got changed
        m_oldDirtyFlag = animGraph->GetDirtyFlag();
        animGraph->SetDirtyFlag(true);

        condition->Reinit();
        animGraph->RecursiveInvalidateUniqueDatas();

        return true;
    }

    bool CommandAdjustTransitionCondition::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZ_UNUSED(parameters);

        EMotionFX::AnimGraphTransitionCondition* condition = GetCondition(outResult);
        if (!condition)
        {
            return false;
        }
        EMotionFX::AnimGraphStateTransition* transition = condition->GetTransition();
        EMotionFX::AnimGraph* animGraph = transition->GetAnimGraph();

        MCore::ReflectionSerializer::Deserialize(condition, m_oldContents.value());

        condition->Reinit();
        animGraph->RecursiveInvalidateUniqueDatas();

        // set the dirty flag back to the old value
        animGraph->SetDirtyFlag(m_oldDirtyFlag.value());
        return true;
    }

    const char* CommandAdjustTransitionCondition::GetDescription() const
    {
        return "Remove a transition condition from a state machine transition.";
    }

    void CommandAdjustTransitionCondition::InitSyntax()
    {
        GetSyntax().ReserveParameters(4);
        MCore::CommandSyntax& syntax = GetSyntax();
        ParameterMixinConditionIndex::InitSyntax(syntax);
        ParameterMixinAttributesString::InitSyntax(syntax);
    }

    bool CommandAdjustTransitionCondition::SetCommandParameters(const MCore::CommandLine& parameters)
    {
        ParameterMixinConditionIndex::SetCommandParameters(parameters);
        ParameterMixinAttributesString::SetCommandParameters(parameters);
        return true;
    }
} // namesapce EMotionFX
