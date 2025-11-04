/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(ParameterMixinActorId, EMotionFX::CommandAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(ParameterMixinJointName, EMotionFX::CommandAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(ParameterMixinAnimGraphId, EMotionFX::CommandAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(ParameterMixinTransitionId, EMotionFX::CommandAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(ParameterMixinConditionIndex, EMotionFX::CommandAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(ParameterMixinAnimGraphNodeId, EMotionFX::CommandAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(ParameterMixinAttributesString, EMotionFX::CommandAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(ParameterMixinSerializedContents, EMotionFX::CommandAllocator)

    const char* ParameterMixinActorId::s_actorIdParameterName = "actorId";
    const char* ParameterMixinJointName::s_jointNameParameterName = "jointName";
    const char* ParameterMixinAnimGraphId::s_parameterName = "animGraphId";
    const char* ParameterMixinTransitionId::s_parameterName = "transitionId";
    const char* ParameterMixinConditionIndex::s_parameterName = "conditionIndex";
    const char* ParameterMixinAnimGraphNodeId::s_parameterName = "nodeId";
    const char* ParameterMixinAttributesString::s_parameterName = "attributesString";
    const char* ParameterMixinSerializedContents::s_parameterName = "contents";

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    ParameterMixinActorId::ParameterMixinActorId(AZ::u32 actorId)
        : m_actorId(actorId)
    {
    }

    void ParameterMixinActorId::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<ParameterMixinActorId>()
            ->Version(1)
            ->Field("actorId", &ParameterMixinActorId::m_actorId)
        ;
    }

    void ParameterMixinActorId::InitSyntax(MCore::CommandSyntax& syntax, bool isParameterRequired)
    {
        const char* description = "The id of the actor.";
        if (isParameterRequired)
        {
            syntax.AddRequiredParameter(s_actorIdParameterName, description, MCore::CommandSyntax::PARAMTYPE_INT);
        }
        else
        {
            syntax.AddParameter(s_actorIdParameterName, description, MCore::CommandSyntax::PARAMTYPE_INT, "-1");
        }
    }

    bool ParameterMixinActorId::SetCommandParameters(const MCore::CommandLine& parameters)
    {
        m_actorId = parameters.GetValueAsInt(s_actorIdParameterName, -1);
        return true;
    }

    Actor* ParameterMixinActorId::GetActor(const MCore::Command* command, AZStd::string& outResult) const
    {
        Actor* result = GetEMotionFX().GetActorManager()->FindActorByID(m_actorId);
        if (!result)
        {
            outResult = AZStd::string::format("%s: Actor with id '%d' does not exist.", command->GetName(), m_actorId);
        }

        return result;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    ParameterMixinJointName::ParameterMixinJointName(const AZStd::string& jointName)
        : m_jointName(jointName)
    {
    }

    void ParameterMixinJointName::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<ParameterMixinJointName>()
            ->Version(1)
            ->Field("jointName", &ParameterMixinJointName::m_jointName)
            ;
    }

    void ParameterMixinJointName::InitSyntax(MCore::CommandSyntax& syntax, bool isParameterRequired)
    {
        const char* description = "The name of the joint in the skeleton.";
        if (isParameterRequired)
        {
            syntax.AddRequiredParameter(s_jointNameParameterName, description, MCore::CommandSyntax::PARAMTYPE_STRING);
        }
        else
        {
            syntax.AddParameter(s_jointNameParameterName, description, MCore::CommandSyntax::PARAMTYPE_STRING, "-1");
        }
    }

    bool ParameterMixinJointName::SetCommandParameters(const MCore::CommandLine& parameters)
    {
        parameters.GetValue(s_jointNameParameterName, "", m_jointName);
        return true;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    void ParameterMixinAnimGraphId::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<ParameterMixinAnimGraphId>()
            ->Version(1)
            ->Field("animGraphId", &ParameterMixinAnimGraphId::m_animGraphId)
            ;
    }

    void ParameterMixinAnimGraphId::InitSyntax(MCore::CommandSyntax& syntax, bool isParameterRequired)
    {
        const char* description = "The id of the anim graph.";
        if (isParameterRequired)
        {
            syntax.AddRequiredParameter(s_parameterName, description, MCore::CommandSyntax::PARAMTYPE_INT);
        }
        else
        {
            syntax.AddParameter(s_parameterName, description, MCore::CommandSyntax::PARAMTYPE_INT, "-1");
        }
    }

    bool ParameterMixinAnimGraphId::SetCommandParameters(const MCore::CommandLine& parameters)
    {
        m_animGraphId = parameters.GetValueAsInt(s_parameterName, -1);
        return true;
    }

    AnimGraph* ParameterMixinAnimGraphId::GetAnimGraph(AZStd::string& outResult) const
    {
        EMotionFX::AnimGraph* animGraph = EMotionFX::GetAnimGraphManager().FindAnimGraphByID(m_animGraphId);
        if (!animGraph)
        {
            outResult = AZStd::string::format("The anim graph with id '%d' does not exist.", m_animGraphId);
            return nullptr;
        }

        return animGraph;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    void ParameterMixinTransitionId::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<ParameterMixinTransitionId, ParameterMixinAnimGraphId>()
            ->Version(1)
            ->Field("transitionId", &ParameterMixinTransitionId::m_transitionId)
            ;
    }

    void ParameterMixinTransitionId::InitSyntax(MCore::CommandSyntax& syntax, bool isParameterRequired)
    {
        ParameterMixinAnimGraphId::InitSyntax(syntax);

        const char* description = "The id of the transition.";
        if (isParameterRequired)
        {
            syntax.AddRequiredParameter(s_parameterName, description, MCore::CommandSyntax::PARAMTYPE_STRING);
        }
        else
        {
            syntax.AddParameter(s_parameterName, description, MCore::CommandSyntax::PARAMTYPE_STRING, "");
        }
    }

    bool ParameterMixinTransitionId::SetCommandParameters(const MCore::CommandLine& parameters)
    {
        ParameterMixinAnimGraphId::SetCommandParameters(parameters);

        if (parameters.CheckIfHasParameter(s_parameterName))
        {
            AZStd::string transitionIdParameter;
            parameters.GetValue(s_parameterName, "", transitionIdParameter);

            m_transitionId = AnimGraphConnectionId::CreateFromString(transitionIdParameter);
        }
        else
        {
            m_transitionId = AnimGraphConnectionId::InvalidId;
        }

        return true;
    }

    AnimGraphStateTransition* ParameterMixinTransitionId::GetTransition(const AnimGraph* animGraph, AZStd::string& outResult) const
    {
        if (!animGraph)
        {
            outResult = "Cannot get transition. Anim graph is invalid.";
            return nullptr;
        }

        if (!m_transitionId.IsValid())
        {
            outResult = "Cannot get transition. Transition id is invalid.";
            return nullptr;
        }

        AnimGraphStateTransition* result = animGraph->RecursiveFindTransitionById(m_transitionId);
        if (!result)
        {
            outResult = AZStd::string::format("Cannot find transition with id '%s' in anim graph '%s'.", m_transitionId.ToString().c_str(), animGraph->GetFileName());
            return nullptr;
        }

        return result;
    }

    AnimGraphStateTransition* ParameterMixinTransitionId::GetTransition(AZStd::string& outResult) const
    {
        EMotionFX::AnimGraph* animGraph = GetAnimGraph(outResult);
        if (!animGraph)
        {
            return nullptr;
        }

        return GetTransition(animGraph, outResult);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    void ParameterMixinConditionIndex::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<ParameterMixinConditionIndex, ParameterMixinTransitionId>()
            ->Version(1)
            ->Field("conditionIndex", &ParameterMixinConditionIndex::m_conditionIndex)
            ;
    }

    void ParameterMixinConditionIndex::InitSyntax(MCore::CommandSyntax& syntax, bool isParameterRequired)
    {
        ParameterMixinTransitionId::InitSyntax(syntax);

        const char* description = "The index of the transition condition.";
        if (isParameterRequired)
        {
            syntax.AddRequiredParameter(s_parameterName, description, MCore::CommandSyntax::PARAMTYPE_STRING);
        }
        else
        {
            syntax.AddParameter(s_parameterName, description, MCore::CommandSyntax::PARAMTYPE_STRING, "");
        }
    }

    bool ParameterMixinConditionIndex::SetCommandParameters(const MCore::CommandLine& parameters)
    {
        ParameterMixinTransitionId::SetCommandParameters(parameters);

        if (parameters.CheckIfHasParameter(s_parameterName))
        {
            const AZ::u32 conditionIndex = parameters.GetValueAsInt(s_parameterName, MCORE_INVALIDINDEX32);
            if (conditionIndex != MCORE_INVALIDINDEX32)
            {
                m_conditionIndex = static_cast<size_t>(conditionIndex);
            }
        }

        return true;
    }

    AnimGraphTransitionCondition* ParameterMixinConditionIndex::GetCondition([[maybe_unused]] const AnimGraph* animGraph, const AnimGraphStateTransition* transition, AZStd::string& outResult) const
    {
        if (!m_conditionIndex.has_value())
        {
            outResult = "Cannot get transition condition. Condition index is not set.";
            return nullptr;
        }

        const size_t numConditions = transition->GetNumConditions();
        if (m_conditionIndex.value() >= numConditions)
        {
            outResult = AZStd::string::format("Cannot get transition condition at index %zu. The transition only has %zu conditions and the index is out of range.", m_conditionIndex.value(), numConditions);
            return nullptr;
        }

        return transition->GetCondition(m_conditionIndex.value());
    }

    AnimGraphTransitionCondition* ParameterMixinConditionIndex::GetCondition(AZStd::string& outResult) const
    {
        AnimGraphStateTransition* transition = GetTransition(outResult);
        if (!transition)
        {
            return nullptr;
        }

        return GetCondition(transition->GetAnimGraph(), transition, outResult);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    void ParameterMixinAnimGraphNodeId::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<ParameterMixinAnimGraphNodeId>()
            ->Version(1)
            ->Field("nodeId", &ParameterMixinAnimGraphNodeId::m_nodeId)
            ;
    }

    void ParameterMixinAnimGraphNodeId::InitSyntax(MCore::CommandSyntax& syntax, bool isParameterRequired)
    {
        const char* description = "The id of the node.";
        if (isParameterRequired)
        {
            syntax.AddRequiredParameter(s_parameterName, description, MCore::CommandSyntax::PARAMTYPE_STRING);
        }
        else
        {
            syntax.AddParameter(s_parameterName, description, MCore::CommandSyntax::PARAMTYPE_STRING, "");
        }
    }

    bool ParameterMixinAnimGraphNodeId::SetCommandParameters(const MCore::CommandLine& parameters)
    {
        if (parameters.CheckIfHasParameter(s_parameterName))
        {
            AZStd::string nodeIdParameter;
            parameters.GetValue(s_parameterName, "", nodeIdParameter);

            m_nodeId = AnimGraphNodeId::CreateFromString(nodeIdParameter);
        }
        else
        {
            m_nodeId = AnimGraphNodeId::InvalidId;
        }

        return true;
    }

    AnimGraphNode* ParameterMixinAnimGraphNodeId::GetNode(const AnimGraph* animGraph, const MCore::Command* command, AZStd::string& outResult) const
    {
        AZ_UNUSED(command);
        if (!animGraph)
        {
            outResult = "Cannot get node. Anim graph is invalid.";
            return nullptr;
        }

        if (!m_nodeId.IsValid())
        {
            outResult = "Cannot get node. Node id is invalid.";
            return nullptr;
        }

        AnimGraphNode* result = animGraph->RecursiveFindNodeById(m_nodeId);
        if (!result)
        {
            outResult = AZStd::string::format("Cannot find node with id '%s' in anim graph '%s'.", m_nodeId.ToString().c_str(), animGraph->GetFileName());
            return nullptr;
        }

        return result;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    void ParameterMixinAttributesString::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<ParameterMixinAttributesString>()
            ->Version(1)
            ->Field("attributesString", &ParameterMixinAttributesString::m_attributesString)
            ;
    }

    void ParameterMixinAttributesString::InitSyntax(MCore::CommandSyntax& syntax, bool isParameterRequired)
    {
        const char* description = "The attributes string.";
        if (isParameterRequired)
        {
            syntax.AddRequiredParameter(s_parameterName, description, MCore::CommandSyntax::PARAMTYPE_STRING);
        } else
        {
            syntax.AddParameter(s_parameterName, description, MCore::CommandSyntax::PARAMTYPE_STRING, "");
        }
    }

    bool ParameterMixinAttributesString::SetCommandParameters(const MCore::CommandLine& parameters)
    {
        if (parameters.CheckIfHasParameter(s_parameterName))
        {
            AZStd::string tempString;
            parameters.GetValue(s_parameterName, "", tempString);
            m_attributesString = tempString; // string to optional conversion
        }
        return true;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    void ParameterMixinSerializedContents::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<ParameterMixinSerializedContents>()
            ->Version(1)
            ->Field("contents", &ParameterMixinSerializedContents::m_contents)
            ;
    }

    void ParameterMixinSerializedContents::InitSyntax(MCore::CommandSyntax& syntax, bool isParameterRequired)
    {
        const char* description = "XML serialized contents.";
        if (isParameterRequired)
        {
            syntax.AddRequiredParameter(s_parameterName, description, MCore::CommandSyntax::PARAMTYPE_STRING);
        } else
        {
            syntax.AddParameter(s_parameterName, description, MCore::CommandSyntax::PARAMTYPE_STRING, "");
        }
    }

    bool ParameterMixinSerializedContents::SetCommandParameters(const MCore::CommandLine& parameters)
    {
        if (parameters.CheckIfHasParameter(s_parameterName))
        {
            AZStd::string tempString;
            parameters.GetValue(s_parameterName, "", tempString);
            m_contents = tempString; // string to optional conversion
        }
        return true;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    AZ_CLASS_ALLOCATOR_IMPL(ParameterMixinSerializedMembers, EMotionFX::CommandAllocator)
    const char* ParameterMixinSerializedMembers::s_parameterName = "serializedMembers";

    void ParameterMixinSerializedMembers::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<ParameterMixinSerializedMembers>()
            ->Version(1)
            ->Field("serializedMembers", &ParameterMixinSerializedMembers::m_serializedMembers)
            ;
    }

    void ParameterMixinSerializedMembers::InitSyntax(MCore::CommandSyntax& syntax, bool isParameterRequired)
    {
        const char* description = "Serialized member variables.";
        if (isParameterRequired)
        {
            syntax.AddRequiredParameter(s_parameterName, description, MCore::CommandSyntax::PARAMTYPE_STRING);
        }
        else
        {
            syntax.AddParameter(s_parameterName, description, MCore::CommandSyntax::PARAMTYPE_STRING, "");
        }
    }

    bool ParameterMixinSerializedMembers::SetCommandParameters(const MCore::CommandLine& parameters)
    {
        if (parameters.CheckIfHasParameter(s_parameterName))
        {
            AZStd::string tempString;
            parameters.GetValue(s_parameterName, "", tempString);
            m_serializedMembers = tempString; // string to optional conversion
        }
        return true;
    }
} // namespace EMotionFX
