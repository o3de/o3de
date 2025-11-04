/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphObjectFactory.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/TriggerActionSetup.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphTriggerActionCommands.h>
#include <MCore/Source/ReflectionSerializer.h>


namespace CommandSystem
{
    void AddTransitionAction(const EMotionFX::AnimGraphStateTransition* transition, const AZ::TypeId& actionType,
        const AZStd::optional<AZStd::string>& contents, const AZStd::optional<size_t>& insertAt,
        MCore::CommandGroup* commandGroup, bool executeInsideCommand)
    {
        AddTransitionAction(transition->GetAnimGraph()->GetID(), transition->GetId().ToString().c_str(),
            actionType, contents, insertAt,
            commandGroup, executeInsideCommand);
    }

    void AddTransitionAction(AZ::u32 animGraphId, const char* transitionIdString, const AZ::TypeId& actionType,
        const AZStd::optional<AZStd::string>& contents, const AZStd::optional<size_t>& insertAt,
        MCore::CommandGroup* commandGroup, bool executeInsideCommand)
    {
        AZStd::string command = AZStd::string::format("%s -%s %d -%s %s -actionType \"%s\"",
            CommandAnimGraphAddTransitionAction::s_commandName,
            EMotionFX::ParameterMixinAnimGraphId::s_parameterName,
            animGraphId,
            EMotionFX::ParameterMixinTransitionId::s_parameterName,
            transitionIdString,
            actionType.ToString<AZStd::string>().c_str());

        if (insertAt)
        {
            command += AZStd::string::format(" -insertAt %zu", insertAt.value());
        }

        if (contents)
        {
            command += AZStd::string::format(" -contents {");
            command += contents.value();
            command += "}";
        }

        CommandSystem::GetCommandManager()->ExecuteCommandOrAddToGroup(command, commandGroup, executeInsideCommand);
    }

    AZ_CLASS_ALLOCATOR_IMPL(CommandAnimGraphAddTransitionAction, EMotionFX::CommandAllocator)
    const char* CommandAnimGraphAddTransitionAction::s_commandName = "AnimGraphAddTransitionAction";

    CommandAnimGraphAddTransitionAction::CommandAnimGraphAddTransitionAction(MCore::Command* orgCommand)
        : MCore::Command(s_commandName, orgCommand)
        , m_oldActionIndex(InvalidIndex)
    {
    }

    bool CommandAnimGraphAddTransitionAction::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        EMotionFX::AnimGraph* animGraph = GetAnimGraph(outResult);
        if (!animGraph)
        {
            return false;
        }

        EMotionFX::AnimGraphStateTransition* transition = GetTransition(animGraph, outResult);
        if (!transition)
        {
            return false;
        }

        EMotionFX::TriggerActionSetup& actionSetup = transition->GetTriggerActionSetup();

        const AZ::Outcome<AZStd::string> actionTypeString = parameters.GetValueIfExists("actionType", this);
        const AZ::TypeId actionType = actionTypeString.IsSuccess() ? AZ::TypeId::CreateString(actionTypeString.GetValue().c_str()) : AZ::TypeId::CreateNull();

        EMotionFX::AnimGraphObject* newActionObject = EMotionFX::AnimGraphObjectFactory::Create(actionType, animGraph);
        if (!newActionObject)
        {
            outResult = AZStd::string::format("Action object invalid. The given action type is either invalid or no object has been registered with type %s.", actionType.ToString<AZStd::string>().c_str());
            return false;
        }

        AZ_Assert(azrtti_istypeof<EMotionFX::AnimGraphTriggerAction>(newActionObject), "Action object must be a trigger action.");
        EMotionFX::AnimGraphTriggerAction* newAction = static_cast<EMotionFX::AnimGraphTriggerAction*>(newActionObject);

        // Deserialize the contents directly, else we might be overwriting things in the end.
        if (parameters.CheckIfHasParameter("contents"))
        {
            const AZStd::string contents = parameters.GetValue("contents", this);
            MCore::ReflectionSerializer::Deserialize(newAction, contents);
        }

        // Redo mode
        if (!m_oldContents.empty())
        {
            MCore::ReflectionSerializer::Deserialize(newAction, m_oldContents);
        }

        // get the location where to add the new action
        size_t insertAt = InvalidIndex;
        if (parameters.CheckIfHasParameter("insertAt"))
        {
            insertAt = parameters.GetValueAsInt("insertAt", this);
        }

        // add it to the transition
        if (insertAt == InvalidIndex)
        {
            actionSetup.AddAction(newAction);
        }
        else
        {
            actionSetup.InsertAction(newAction, insertAt);
        }

        // store information for undo
        const AZ::Outcome<size_t> actionIndex = actionSetup.FindActionIndex(newAction);
        if (!actionIndex.IsSuccess())
        {
            return false;
        }
        m_oldActionIndex = actionIndex.GetValue();

        // save the current dirty flag and tell the anim graph that something got changed
        m_oldDirtyFlag = animGraph->GetDirtyFlag();
        animGraph->SetDirtyFlag(true);

        // set the command result to the transition id and return success
        outResult = m_transitionId.ToString().c_str();
        m_oldContents.clear();

        newAction->Reinit();
        animGraph->RecursiveInvalidateUniqueDatas();

        return true;
    }

    bool CommandAnimGraphAddTransitionAction::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZ_UNUSED(parameters);
        EMotionFX::AnimGraph* animGraph = GetAnimGraph(outResult);
        if (!animGraph)
        {
            return false;
        }

        EMotionFX::AnimGraphStateTransition* transition = GetTransition(animGraph, outResult);
        if (!transition)
        {
            return false;
        }

        EMotionFX::AnimGraphTriggerAction* action = transition->GetTriggerActionSetup().GetAction(m_oldActionIndex);

        // store the attributes string for redo
        m_oldContents = MCore::ReflectionSerializer::Serialize(action).GetValue();

        RemoveTransitionAction(transition, m_oldActionIndex, /*commandGroup*/nullptr, /*executeInsideCommand*/true);

        // set the dirty flag back to the old value
        animGraph->SetDirtyFlag(m_oldDirtyFlag);
        return true;
    }

    void CommandAnimGraphAddTransitionAction::InitSyntax()
    {
        GetSyntax().ReserveParameters(5);

        MCore::CommandSyntax& syntax = GetSyntax();
        ParameterMixinTransitionId::InitSyntax(syntax);

        GetSyntax().AddRequiredParameter("actionType", "The type id of the transition action to add.", MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddParameter("insertAt", "The index at which the transition action will be added.", MCore::CommandSyntax::PARAMTYPE_INT, "-1");
        GetSyntax().AddParameter("contents", "The serialized contents of the parameter (in reflected XML).", MCore::CommandSyntax::PARAMTYPE_STRING, "");
    }

    bool CommandAnimGraphAddTransitionAction::SetCommandParameters(const MCore::CommandLine& parameters)
    {
        ParameterMixinTransitionId::SetCommandParameters(parameters);
        return true;
    }

    const char* CommandAnimGraphAddTransitionAction::GetDescription() const
    {
        return "Add a new a transition action to a state machine transition.";
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    void RemoveTransitionAction(const EMotionFX::AnimGraphStateTransition* transition, const size_t actionIndex,
        MCore::CommandGroup* commandGroup, bool executeInsideCommand)
    {
        AZStd::string command = AZStd::string::format("%s -%s %i -%s %s -actionIndex %zu",
            CommandAnimGraphRemoveTransitionAction::s_commandName,
            EMotionFX::ParameterMixinAnimGraphId::s_parameterName, transition->GetAnimGraph()->GetID(),
            EMotionFX::ParameterMixinTransitionId::s_parameterName, transition->GetId().ToString().c_str(),
            actionIndex);

        CommandSystem::GetCommandManager()->ExecuteCommandOrAddToGroup(command, commandGroup, executeInsideCommand);
    }

    AZ_CLASS_ALLOCATOR_IMPL(CommandAnimGraphRemoveTransitionAction, EMotionFX::CommandAllocator)
    const char* CommandAnimGraphRemoveTransitionAction::s_commandName = "AnimGraphRemoveTransitionAction";

    CommandAnimGraphRemoveTransitionAction::CommandAnimGraphRemoveTransitionAction(MCore::Command* orgCommand)
        : MCore::Command(s_commandName, orgCommand)
    {
        m_oldActionType = AZ::TypeId::CreateNull();
        m_oldActionIndex = InvalidIndex;
    }

    bool CommandAnimGraphRemoveTransitionAction::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        EMotionFX::AnimGraph* animGraph = GetAnimGraph(outResult);
        if (!animGraph)
        {
            return false;
        }

        EMotionFX::AnimGraphStateTransition* transition = GetTransition(animGraph, outResult);
        if (!transition)
        {
            return false;
        }

        // get the transition action
        EMotionFX::TriggerActionSetup& actionSetup = transition->GetTriggerActionSetup();
        const uint32 actionIndex = parameters.GetValueAsInt("actionIndex", this);
        MCORE_ASSERT(actionIndex < actionSetup.GetNumActions());
        EMotionFX::AnimGraphTriggerAction* action = actionSetup.GetAction(actionIndex);

        // store information for undo
        m_oldActionType = azrtti_typeid(action);
        m_oldActionIndex = actionIndex;
        m_oldContents = MCore::ReflectionSerializer::Serialize(action).GetValue();

        // 1. Remove the unique data of the action for all anim graph instances.
        animGraph->RemoveAllObjectData(action, true);

        // 2. Remove the action object from the anim graph.
        actionSetup.RemoveAction(actionIndex);

        // save the current dirty flag and tell the anim graph that something got changed
        m_oldDirtyFlag = animGraph->GetDirtyFlag();
        animGraph->SetDirtyFlag(true);
        return true;
    }

    bool CommandAnimGraphRemoveTransitionAction::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZ_UNUSED(parameters);
        EMotionFX::AnimGraph* animGraph = GetAnimGraph(outResult);
        if (!animGraph)
        {
            return false;
        }

        EMotionFX::AnimGraphStateTransition* transition = GetTransition(animGraph, outResult);
        if (!transition)
        {
            return false;
        }

        AddTransitionAction(transition, m_oldActionType,
            m_oldContents, m_oldActionIndex,
            /*commandGroup*/nullptr, /*executeInsideCommand*/true);

        // set the dirty flag back to the old value
        animGraph->SetDirtyFlag(m_oldDirtyFlag);
        return true;
    }

    void CommandAnimGraphRemoveTransitionAction::InitSyntax()
    {
        GetSyntax().ReserveParameters(3);

        MCore::CommandSyntax& syntax = GetSyntax();
        ParameterMixinTransitionId::InitSyntax(syntax);

        GetSyntax().AddRequiredParameter("actionIndex", "The index of the transition action to remove.", MCore::CommandSyntax::PARAMTYPE_INT);
    }

    bool CommandAnimGraphRemoveTransitionAction::SetCommandParameters(const MCore::CommandLine& parameters)
    {
        ParameterMixinTransitionId::SetCommandParameters(parameters);
        return true;
    }

    const char* CommandAnimGraphRemoveTransitionAction::GetDescription() const
    {
        return "Remove a transition action from a state machine transition.";
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    void AddStateAction(const EMotionFX::AnimGraphNode* state, const AZ::TypeId& actionType,
        const AZStd::optional<AZStd::string>& contents, const AZStd::optional<size_t>& insertAt,
        MCore::CommandGroup* commandGroup, bool executeInsideCommand)
    {
        AZStd::string command = AZStd::string::format("%s -%s %d -%s %s -actionType \"%s\"",
            CommandAnimGraphAddStateAction::s_commandName,
            EMotionFX::ParameterMixinAnimGraphId::s_parameterName, state->GetAnimGraph()->GetID(),
            EMotionFX::ParameterMixinAnimGraphNodeId::s_parameterName, state->GetId().ToString().c_str(),
            actionType.ToString<AZStd::string>().c_str());

        if (insertAt)
        {
            command += AZStd::string::format(" -insertAt %zu", insertAt.value());
        }

        if (contents)
        {
            command += AZStd::string::format(" -contents {");
            command += contents.value();
            command += "}";
        }

        CommandSystem::GetCommandManager()->ExecuteCommandOrAddToGroup(command, commandGroup, executeInsideCommand);
    }

    AZ_CLASS_ALLOCATOR_IMPL(CommandAnimGraphAddStateAction, EMotionFX::CommandAllocator)
    const char* CommandAnimGraphAddStateAction::s_commandName = "AnimGraphAddStateAction";

    CommandAnimGraphAddStateAction::CommandAnimGraphAddStateAction(MCore::Command* orgCommand)
        : MCore::Command(s_commandName, orgCommand)
        , m_oldActionIndex(InvalidIndex)
    {
    }

    bool CommandAnimGraphAddStateAction::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        EMotionFX::AnimGraph* animGraph = GetAnimGraph(outResult);
        if (!animGraph)
        {
            return false;
        }

        EMotionFX::AnimGraphNode* node = GetNode(animGraph, this, outResult);
        if (!node)
        {
            return false;
        }

        // check if we are dealing with a state node
        if (!node->GetCanActAsState())
        {
            outResult = AZStd::string::format("Anim graph node with name '%s' is not a state.", node->GetName());
            return false;
        }

        EMotionFX::TriggerActionSetup& actionSetup = node->GetTriggerActionSetup();

        const AZ::Outcome<AZStd::string> actionTypeString = parameters.GetValueIfExists("actionType", this);
        const AZ::TypeId actionType = actionTypeString.IsSuccess() ? AZ::TypeId::CreateString(actionTypeString.GetValue().c_str()) : AZ::TypeId::CreateNull();

        EMotionFX::AnimGraphObject* newActionObject = EMotionFX::AnimGraphObjectFactory::Create(actionType, animGraph);
        if (!newActionObject)
        {
            outResult = AZStd::string::format("Action object invalid. The given action type is either invalid or no object has been registered with type %s.", actionType.ToString<AZStd::string>().c_str());
            return false;
        }

        AZ_Assert(azrtti_istypeof<EMotionFX::AnimGraphTriggerAction>(newActionObject), "Action object must be a trigger action.");
        EMotionFX::AnimGraphTriggerAction* newAction = static_cast<EMotionFX::AnimGraphTriggerAction*>(newActionObject);

        // Deserialize the contents directly, else we might be overwriting things in the end.
        if (parameters.CheckIfHasParameter("contents"))
        {
            const AZStd::string contents = parameters.GetValue("contents", this);
            MCore::ReflectionSerializer::Deserialize(newAction, contents);
        }

        // Redo mode
        if (!m_oldContents.empty())
        {
            MCore::ReflectionSerializer::Deserialize(newAction, m_oldContents);
        }

        // get the location where to add the new action
        size_t insertAt = InvalidIndex;
        if (parameters.CheckIfHasParameter("insertAt"))
        {
            insertAt = parameters.GetValueAsInt("insertAt", this);
        }

        // add it to the transition
        if (insertAt == InvalidIndex)
        {
            actionSetup.AddAction(newAction);
        }
        else
        {
            actionSetup.InsertAction(newAction, insertAt);
        }

        // store information for undo
        const AZ::Outcome<size_t> actionIndex = actionSetup.FindActionIndex(newAction);
        if (!actionIndex.IsSuccess())
        {
            return false;
        }
        m_oldActionIndex = actionIndex.GetValue();

        // save the current dirty flag and tell the anim graph that something got changed
        m_oldDirtyFlag = animGraph->GetDirtyFlag();
        animGraph->SetDirtyFlag(true);

        m_oldContents.clear();

        newAction->Reinit();
        animGraph->RecursiveInvalidateUniqueDatas();
        return true;
    }

    bool CommandAnimGraphAddStateAction::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZ_UNUSED(parameters);
        EMotionFX::AnimGraph* animGraph = GetAnimGraph(outResult);
        if (!animGraph)
        {
            return false;
        }

        EMotionFX::AnimGraphNode* node = GetNode(animGraph, this, outResult);
        if (!node)
        {
            return false;
        }

        // check if we are dealing with a state node
        if (!node->GetCanActAsState())
        {
            outResult = AZStd::string::format("Anim graph node with name '%s' is not a state.", node->GetName());
            return false;
        }

        // get the trigger action
        EMotionFX::AnimGraphTriggerAction* action = node->GetTriggerActionSetup().GetAction(m_oldActionIndex);

        // store the attributes string for redo
        m_oldContents = MCore::ReflectionSerializer::Serialize(action).GetValue();

        RemoveStateAction(node, m_oldActionIndex, /*commandGroup*/nullptr, /*executeInsideCommand*/true);

        // set the dirty flag back to the old value
        animGraph->SetDirtyFlag(m_oldDirtyFlag);
        return true;
    }

    void CommandAnimGraphAddStateAction::InitSyntax()
    {
        GetSyntax().ReserveParameters(5);

        MCore::CommandSyntax& syntax = GetSyntax();
        ParameterMixinAnimGraphId::InitSyntax(syntax);
        ParameterMixinAnimGraphNodeId::InitSyntax(syntax);

        GetSyntax().AddRequiredParameter("actionType", "The type id of the state action to add.", MCore::CommandSyntax::PARAMTYPE_STRING);
        GetSyntax().AddParameter("insertAt", "The index at which the state action will be added.", MCore::CommandSyntax::PARAMTYPE_INT, "-1");
        GetSyntax().AddParameter("contents", "The serialized contents of the parameter (in reflected XML).", MCore::CommandSyntax::PARAMTYPE_STRING, "");
    }

    bool CommandAnimGraphAddStateAction::SetCommandParameters(const MCore::CommandLine& parameters)
    {
        ParameterMixinAnimGraphId::SetCommandParameters(parameters);
        ParameterMixinAnimGraphNodeId::SetCommandParameters(parameters);
        return true;
    }

    const char* CommandAnimGraphAddStateAction::GetDescription() const
    {
        return "Add a new a state action to a state node.";
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    void RemoveStateAction(const EMotionFX::AnimGraphNode* state, const size_t actionIndex,
        MCore::CommandGroup* commandGroup, bool executeInsideCommand)
    {
        AZStd::string command = AZStd::string::format("%s -%s %i -%s %s -actionIndex %zu",
            CommandAnimGraphRemoveStateAction::s_commandName,
            EMotionFX::ParameterMixinAnimGraphId::s_parameterName, state->GetAnimGraph()->GetID(),
            EMotionFX::ParameterMixinAnimGraphNodeId::s_parameterName, state->GetId().ToString().c_str(),
            actionIndex);

        CommandSystem::GetCommandManager()->ExecuteCommandOrAddToGroup(command, commandGroup, executeInsideCommand);
    }

    AZ_CLASS_ALLOCATOR_IMPL(CommandAnimGraphRemoveStateAction, EMotionFX::CommandAllocator)
    const char* CommandAnimGraphRemoveStateAction::s_commandName = "AnimGraphRemoveStateAction";

    CommandAnimGraphRemoveStateAction::CommandAnimGraphRemoveStateAction(MCore::Command* orgCommand)
        : MCore::Command(s_commandName, orgCommand)
    {
        m_oldActionType   = AZ::TypeId::CreateNull();
        m_oldActionIndex  = InvalidIndex;
    }

    bool CommandAnimGraphRemoveStateAction::Execute(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        EMotionFX::AnimGraph* animGraph = GetAnimGraph(outResult);
        if (!animGraph)
        {
            return false;
        }

        EMotionFX::AnimGraphNode* node = GetNode(animGraph, this, outResult);
        if (!node)
        {
            return false;
        }

        // check if we are dealing with a state node
        if (!node->GetCanActAsState())
        {
            outResult = AZStd::string::format("Anim graph node with name '%s' is not a state.", node->GetName());
            return false;
        }

        // get the state action
        EMotionFX::TriggerActionSetup& actionSetup = node->GetTriggerActionSetup();
        const uint32 actionIndex = parameters.GetValueAsInt("actionIndex", this);
        AZ_Assert(actionIndex < actionSetup.GetNumActions(), "Action index should be within range.");
        EMotionFX::AnimGraphTriggerAction* action = actionSetup.GetAction(actionIndex);

        // store information for undo
        m_oldActionType   = azrtti_typeid(action);
        m_oldActionIndex  = actionIndex;
        m_oldContents     = MCore::ReflectionSerializer::Serialize(action).GetValue();

        // 1. Remove the unique data of the action for all anim graph instances.
        animGraph->RemoveAllObjectData(action, true);

        // 2. Remove the action object from the anim graph.
        actionSetup.RemoveAction(actionIndex);

        // save the current dirty flag and tell the anim graph that something got changed
        m_oldDirtyFlag = animGraph->GetDirtyFlag();
        animGraph->SetDirtyFlag(true);
        animGraph->RecursiveInvalidateUniqueDatas();
        return true;
    }

    bool CommandAnimGraphRemoveStateAction::Undo(const MCore::CommandLine& parameters, AZStd::string& outResult)
    {
        AZ_UNUSED(parameters);
        EMotionFX::AnimGraph* animGraph = GetAnimGraph(outResult);
        if (!animGraph)
        {
            return false;
        }

        EMotionFX::AnimGraphNode* node = GetNode(animGraph, this, outResult);
        if (!node)
        {
            return false;
        }

        AddStateAction(node, m_oldActionType,
            m_oldContents, m_oldActionIndex,
            /*commandGroup*/nullptr, /*executeInsideCommand*/true);

        // set the dirty flag back to the old value
        animGraph->SetDirtyFlag(m_oldDirtyFlag);
        return true;
    }

    void CommandAnimGraphRemoveStateAction::InitSyntax()
    {
        GetSyntax().ReserveParameters(3);

        MCore::CommandSyntax& syntax = GetSyntax();
        ParameterMixinAnimGraphId::InitSyntax(syntax);
        ParameterMixinAnimGraphNodeId::InitSyntax(syntax);

        GetSyntax().AddRequiredParameter("actionIndex", "The index of the state action to remove.", MCore::CommandSyntax::PARAMTYPE_INT);
    }

    bool CommandAnimGraphRemoveStateAction::SetCommandParameters(const MCore::CommandLine& parameters)
    {
        ParameterMixinAnimGraphId::SetCommandParameters(parameters);
        ParameterMixinAnimGraphNodeId::SetCommandParameters(parameters);
        return true;
    }

    const char* CommandAnimGraphRemoveStateAction::GetDescription() const
    {
        return "Remove a state action from a state node.";
    }
} // namesapce EMotionFX
