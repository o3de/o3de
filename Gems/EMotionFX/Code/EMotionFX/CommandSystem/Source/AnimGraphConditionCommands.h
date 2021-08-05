/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/std/optional.h>
#include <EMotionFX/Source/AnimGraphStateTransition.h>
#include <EMotionFX/Source/AnimGraphTransitionCondition.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/CommandSystem/Source/ParameterMixins.h>
#include <MCore/Source/Command.h>

namespace CommandSystem
{
    class CommandAddTransitionCondition
        : public MCore::Command
        , public EMotionFX::ParameterMixinTransitionId
        , public EMotionFX::ParameterMixinSerializedContents
    {
    public:
        AZ_RTTI(CommandAddTransitionCondition, "{617FB76A-4BE8-47EA-B7F1-2FD0B961E352}", MCore::Command, ParameterMixinTransitionId, EMotionFX::ParameterMixinSerializedContents)
        AZ_CLASS_ALLOCATOR_DECL

        CommandAddTransitionCondition(MCore::Command* orgCommand = nullptr);
        CommandAddTransitionCondition(
            AZ::u32 animGraphId,
            EMotionFX::AnimGraphConnectionId transitionId,
            AZ::TypeId conditionType,
            AZStd::optional<size_t> insertAt = AZStd::nullopt,
            AZStd::optional<AZStd::string> contents = AZStd::nullopt,
            Command* orgCommand = nullptr);

        static void Reflect(AZ::ReflectContext* context);
        bool Execute(const MCore::CommandLine& parameters, AZStd::string& outResult) override;
        bool Undo(const MCore::CommandLine& parameters, AZStd::string& outResult) override;

        static const char* s_commandName;
        static const char* s_insertAtParameterName;
        static const char* s_conditionTypeParameterName;

        bool GetIsUndoable() const override { return true; }
        const char* GetHistoryName() const override { return "Add a transition condition"; }
        const char* GetDescription() const override;
        MCore::Command* Create() override { return aznew CommandAddTransitionCondition(this); }

        void InitSyntax() override;
        bool SetCommandParameters(const MCore::CommandLine& parameters) override;

    private:
        AZStd::optional<size_t> m_insertAt;
        AZStd::optional<AZ::TypeId> m_conditionType;

        AZ::Outcome<size_t> m_oldConditionIndex;
        AZStd::optional<bool> m_oldDirtyFlag;
        AZStd::optional<AZStd::string> m_oldContents;
    };

    class CommandRemoveTransitionCondition
        : public MCore::Command
        , public EMotionFX::ParameterMixinConditionIndex
    {
    public:
        AZ_RTTI(CommandRemoveTransitionCondition, "{549FF52D-0A55-4094-A4F3-5A792B4D51CD}", MCore::Command, ParameterMixinConditionIndex)
        AZ_CLASS_ALLOCATOR_DECL

        CommandRemoveTransitionCondition(MCore::Command* orgCommand = nullptr);
        CommandRemoveTransitionCondition(AZ::u32 animGraphId, EMotionFX::AnimGraphConnectionId transitionId, size_t conditionIndex, Command* orgCommand = nullptr);

        static void Reflect(AZ::ReflectContext* context);
        bool Execute(const MCore::CommandLine& parameters, AZStd::string& outResult) override;
        bool Undo(const MCore::CommandLine& parameters, AZStd::string& outResult) override;

        static const char* s_commandName;

        bool GetIsUndoable() const override { return true; }
        const char* GetHistoryName() const override { return "Remove a transition condition"; }
        const char* GetDescription() const override;
        MCore::Command* Create() override { return aznew CommandRemoveTransitionCondition(this); }

        void InitSyntax() override;
        bool SetCommandParameters(const MCore::CommandLine& parameters) override;

    private:
         AZStd::optional<AZ::TypeId> m_oldConditionType;
         AZStd::optional<AZStd::string> m_oldContents;
         AZStd::optional<bool> m_oldDirtyFlag;
    };
    
    class CommandAdjustTransitionCondition
        : public MCore::Command
        , public EMotionFX::ParameterMixinConditionIndex
        , public EMotionFX::ParameterMixinAttributesString
    {
    public:
        AZ_RTTI(CommandAdjustTransitionCondition, "{D46E1922-FB4E-4FDD-8196-5980585ABE14}",
            MCore::Command,
            EMotionFX::ParameterMixinConditionIndex,
            EMotionFX::ParameterMixinAttributesString)
        AZ_CLASS_ALLOCATOR_DECL

        CommandAdjustTransitionCondition(MCore::Command* orgCommand = nullptr);
        CommandAdjustTransitionCondition(AZ::u32 animGraphId, EMotionFX::AnimGraphConnectionId transitionId, size_t conditionIndex, const AZStd::string& attributesString, Command* orgCommand = nullptr);

        static void Reflect(AZ::ReflectContext* context);
        bool Execute(const MCore::CommandLine& parameters, AZStd::string& outResult) override;
        bool Undo(const MCore::CommandLine& parameters, AZStd::string& outResult) override;

        static const char* s_commandName;

        bool GetIsUndoable() const override { return true; }
        const char* GetHistoryName() const override { return "Adjust a transition condition"; }
        const char* GetDescription() const override;
        MCore::Command* Create() override { return aznew CommandAdjustTransitionCondition(this); }

        void InitSyntax() override;
        bool SetCommandParameters(const MCore::CommandLine& parameters) override;

    private:
        AZStd::optional<AZStd::string> m_oldContents;
        AZStd::optional<bool> m_oldDirtyFlag;
    };
} // namespace CommandSystem
