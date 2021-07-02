/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/Memory.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/optional.h>
#include <MCore/Source/Command.h>
#include <EMotionFX/Source/AnimGraphObjectIds.h>


namespace AZ
{
    class ReflectContext;
}

namespace EMotionFX
{
    class Actor;
    class AnimGraph;
    class AnimGraphNode;
    class AnimGraphStateTransition;
    class AnimGraphTransitionCondition;

    class ParameterMixinActorId
    {
    public:
        AZ_RTTI(ParameterMixinActorId, "{EE5FAA4B-FC04-4323-820F-FFE46EFC8038}")
        AZ_CLASS_ALLOCATOR_DECL

        ParameterMixinActorId() = default;
        explicit ParameterMixinActorId(AZ::u32 actorId);
        virtual ~ParameterMixinActorId() = default;

        static void Reflect(AZ::ReflectContext* context);

        static const char* s_actorIdParameterName;
        void InitSyntax(MCore::CommandSyntax& syntax, bool isParameterRequired = true);
        bool SetCommandParameters(const MCore::CommandLine& parameters);

        void SetActorId(AZ::u32 actorId) { m_actorId = actorId; }
        AZ::u32 GetActorId() const { return m_actorId; }

        Actor* GetActor(const MCore::Command* command, AZStd::string& outResult) const;
    protected:
        AZ::u32 m_actorId = MCORE_INVALIDINDEX32;
    };

    class ParameterMixinJointName
    {
    public:
        AZ_RTTI(ParameterMixinJointName, "{9EFF81B2-4720-449F-8B7E-59C9F437E7E3}")
        AZ_CLASS_ALLOCATOR_DECL

        ParameterMixinJointName() = default;
        explicit ParameterMixinJointName(const AZStd::string& jointName);
        virtual ~ParameterMixinJointName() = default;

        static void Reflect(AZ::ReflectContext* context);

        static const char* s_jointNameParameterName;
        void InitSyntax(MCore::CommandSyntax& syntax, bool isParameterRequired = true);
        bool SetCommandParameters(const MCore::CommandLine& parameters);

        void SetJointName(const AZStd::string& jointName) { m_jointName = jointName; }
        const AZStd::string& GetJointName() const { return m_jointName; }
    protected:
        AZStd::string m_jointName;
    };

    class ParameterMixinAnimGraphId
    {
    public:
        AZ_RTTI(ParameterMixinAnimGraphId, "{3F48199E-6566-471F-A7EA-ADF67CAC4DCD}")
        AZ_CLASS_ALLOCATOR_DECL

        virtual ~ParameterMixinAnimGraphId() = default;

        static void Reflect(AZ::ReflectContext* context);

        static const char* s_parameterName;
        void InitSyntax(MCore::CommandSyntax& syntax, bool isParameterRequired = true);
        bool SetCommandParameters(const MCore::CommandLine& parameters);

        void SetAnimGraphId(AZ::u32 animGraphId) { m_animGraphId = animGraphId; }
        AZ::u32 GetAnimGraphId() const { return m_animGraphId; }

        AnimGraph* GetAnimGraph(AZStd::string& outResult) const;

    protected:
        AZ::u32 m_animGraphId = MCORE_INVALIDINDEX32;
    };

    class ParameterMixinTransitionId
        : public ParameterMixinAnimGraphId
    {
    public:
        AZ_RTTI(ParameterMixinTransitionId, "{B70F34AB-CE3B-4AF2-B6D8-2D38F1CEBBC8}", ParameterMixinAnimGraphId)
        AZ_CLASS_ALLOCATOR_DECL

        virtual ~ParameterMixinTransitionId() = default;

        static void Reflect(AZ::ReflectContext* context);

        static const char* s_parameterName;
        void InitSyntax(MCore::CommandSyntax& syntax, bool isParameterRequired = true);
        bool SetCommandParameters(const MCore::CommandLine& parameters);

        void SetTransitionId(AnimGraphConnectionId transitionId) { m_transitionId = transitionId; }
        AnimGraphConnectionId GetTransitionId() const { return m_transitionId; }

        AnimGraphStateTransition* GetTransition(const AnimGraph* animGraph, AZStd::string& outResult) const;
        AnimGraphStateTransition* GetTransition(AZStd::string& outResult) const;

    protected:
        AnimGraphConnectionId m_transitionId;
    };

    class ParameterMixinConditionIndex
        : public ParameterMixinTransitionId
    {
    public:
        AZ_RTTI(ParameterMixinConditionIndex, "{A0E27C1F-FA15-4F9C-888A-3166187ABBCC}", ParameterMixinTransitionId)
        AZ_CLASS_ALLOCATOR_DECL

        virtual ~ParameterMixinConditionIndex() = default;

        static void Reflect(AZ::ReflectContext* context);

        static const char* s_parameterName;
        void InitSyntax(MCore::CommandSyntax& syntax, bool isParameterRequired = true);
        bool SetCommandParameters(const MCore::CommandLine& parameters);

        void SetConditionIndex(size_t index) { m_conditionIndex = index; }
        AZStd::optional<size_t> GetConditionIndex() const { return m_conditionIndex; }

        AnimGraphTransitionCondition* GetCondition(const AnimGraph* animGraph, const AnimGraphStateTransition* transition, AZStd::string& outResult) const;
        AnimGraphTransitionCondition* GetCondition(AZStd::string& outResult) const;

    protected:
        AZStd::optional<size_t> m_conditionIndex;
    };

    class ParameterMixinAnimGraphNodeId
    {
    public:
        AZ_RTTI(ParameterMixinAnimGraphNodeId, "{D5329E34-B20C-43D2-A169-A0E446CC244E}")
        AZ_CLASS_ALLOCATOR_DECL

        virtual ~ParameterMixinAnimGraphNodeId() = default;

        static void Reflect(AZ::ReflectContext* context);

        static const char* s_parameterName;
        void InitSyntax(MCore::CommandSyntax& syntax, bool isParameterRequired = true);
        bool SetCommandParameters(const MCore::CommandLine& parameters);

        void SetNodeId(AnimGraphNodeId nodeId) { m_nodeId = nodeId; }
        AnimGraphNodeId GetNodeId() const { return m_nodeId; }

        AnimGraphNode* GetNode(const AnimGraph* animGraph, const MCore::Command* command, AZStd::string& outResult) const;

    protected:
        AnimGraphNodeId m_nodeId;
    };

    class ParameterMixinAttributesString
    {
    public:
        AZ_RTTI(ParameterMixinAttributesString, "{3A20FAA8-F882-4119-98D9-AA853155EECC}")
        AZ_CLASS_ALLOCATOR_DECL

        virtual ~ParameterMixinAttributesString() = default;

        static void Reflect(AZ::ReflectContext* context);

        void InitSyntax(MCore::CommandSyntax& syntax, bool isParameterRequired = true);
        bool SetCommandParameters(const MCore::CommandLine& parameters);

        void SetAttributesString(const AZStd::optional<AZStd::string>& attributesString) { m_attributesString = attributesString; }
        const AZStd::optional<AZStd::string>& GetAttributesString() const { return m_attributesString; }

        static const char* s_parameterName;

    protected:
        AZStd::optional<AZStd::string> m_attributesString = AZStd::nullopt;
    };

    class ParameterMixinSerializedContents
    {
    public:
        AZ_RTTI(ParameterMixinSerializedContents, "{D4B6F9DD-404E-46D3-8AD8-7F3F6A42992B}")
        AZ_CLASS_ALLOCATOR_DECL

        virtual ~ParameterMixinSerializedContents() = default;

        static void Reflect(AZ::ReflectContext* context);

        void InitSyntax(MCore::CommandSyntax& syntax, bool isParameterRequired = true);
        bool SetCommandParameters(const MCore::CommandLine& parameters);

        void SetContents(const AZStd::optional<AZStd::string>& contents) { m_contents = contents; }
        const AZStd::optional<AZStd::string>& GetContents() const { return m_contents; }

        static const char* s_parameterName;

    protected:
        AZStd::optional<AZStd::string> m_contents = AZStd::nullopt;
    };

    class ParameterMixinSerializedMembers
    {
    public:
        AZ_RTTI(ParameterMixinSerializedMembers, "{1D462965-2A60-46F5-B4A1-6F903E4DF53C}")
        AZ_CLASS_ALLOCATOR_DECL

        virtual ~ParameterMixinSerializedMembers() = default;

        static void Reflect(AZ::ReflectContext* context);

        void InitSyntax(MCore::CommandSyntax& syntax, bool isParameterRequired = true);
        bool SetCommandParameters(const MCore::CommandLine& parameters);

        void SetSerializedMembers(const AZStd::optional<AZStd::string>& serializedMembers) { m_serializedMembers = serializedMembers; }
        const AZStd::optional<AZStd::string>& GetSerializedMembers() const { return m_serializedMembers; }

        static const char* s_parameterName;

    protected:
        AZStd::optional<AZStd::string> m_serializedMembers = AZStd::nullopt;
    };
} // namespace EMotionFX
