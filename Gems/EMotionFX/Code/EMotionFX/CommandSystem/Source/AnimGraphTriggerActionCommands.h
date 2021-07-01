/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/optional.h>
#include <MCore/Source/Command.h>
#include <EMotionFX/Source/AnimGraphStateTransition.h>
#include <EMotionFX/Source/AnimGraphTriggerAction.h>
#include <EMotionFX/CommandSystem/Source/CommandSystemConfig.h>
#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/CommandSystem/Source/ParameterMixins.h>


namespace CommandSystem
{
    void AddTransitionAction(const EMotionFX::AnimGraphStateTransition* transition, const AZ::TypeId& actionType,
        const AZStd::optional<AZStd::string>& contents = AZStd::nullopt, const AZStd::optional<size_t>& insertAt = AZStd::nullopt,
        MCore::CommandGroup* commandGroup = nullptr, bool executeInsideCommand = false);

    void AddTransitionAction(AZ::u32 animGraphId, const char* transitionIdString, const AZ::TypeId& actionType,
        const AZStd::optional<AZStd::string>& contents = AZStd::nullopt, const AZStd::optional<size_t>& insertAt = AZStd::nullopt,
        MCore::CommandGroup* commandGroup = nullptr, bool executeInsideCommand = false);

    class CommandAnimGraphAddTransitionAction
        : public MCore::Command
        , public EMotionFX::ParameterMixinTransitionId
    {
    public:
        AZ_RTTI(CommandAnimGraphAddTransitionAction, "{EBC2F137-6473-4D7B-AD59-BB044CFC880E}", MCore::Command, ParameterMixinTransitionId)
        AZ_CLASS_ALLOCATOR_DECL

        CommandAnimGraphAddTransitionAction(MCore::Command* orgCommand = nullptr);

        bool Execute(const MCore::CommandLine& parameters, AZStd::string& outResult) override;
        bool Undo(const MCore::CommandLine& parameters, AZStd::string& outResult) override;

        static const char* s_commandName;
        void InitSyntax() override;
        bool SetCommandParameters(const MCore::CommandLine& parameters) override;

        bool GetIsUndoable() const override { return true; }
        const char* GetHistoryName() const override { return "Add trigger action to transition"; }
        const char* GetDescription() const override;
        MCore::Command* Create() override { return aznew CommandAnimGraphAddTransitionAction(this); }

    private:
        size_t          m_oldActionIndex;
        bool            m_oldDirtyFlag;
        AZStd::string   m_oldContents;
    };


    void RemoveTransitionAction(const EMotionFX::AnimGraphStateTransition* transition, const size_t actionIndex,
        MCore::CommandGroup* commandGroup = nullptr, bool executeInsideCommand = false);

    class CommandAnimGraphRemoveTransitionAction
        : public MCore::Command
        , public EMotionFX::ParameterMixinTransitionId
    {
    public:
        AZ_RTTI(CommandAnimGraphRemoveTransitionAction, "{19219257-24E5-4216-AF4F-8375DA6F3F74}", MCore::Command, ParameterMixinTransitionId)
        AZ_CLASS_ALLOCATOR_DECL

        CommandAnimGraphRemoveTransitionAction(MCore::Command* orgCommand = nullptr);

        bool Execute(const MCore::CommandLine& parameters, AZStd::string& outResult) override;
        bool Undo(const MCore::CommandLine& parameters, AZStd::string& outResult) override;

        static const char* s_commandName;
        void InitSyntax() override;
        bool SetCommandParameters(const MCore::CommandLine& parameters) override;

        bool GetIsUndoable() const override { return true; }
        const char* GetHistoryName() const override { return "Remove trigger action from transition"; }
        const char* GetDescription() const override;
        MCore::Command* Create() override { return aznew CommandAnimGraphRemoveTransitionAction(this); }

    private:
        AZ::TypeId      m_oldActionType;
        size_t          m_oldActionIndex;
        AZStd::string   m_oldContents;
        bool            m_oldDirtyFlag;
    };


    void AddStateAction(const EMotionFX::AnimGraphNode* state, const AZ::TypeId& actionType,
        const AZStd::optional<AZStd::string>& contents = AZStd::nullopt, const AZStd::optional<size_t>& insertAt = AZStd::nullopt,
        MCore::CommandGroup* commandGroup = nullptr, bool executeInsideCommand = false);

    class CommandAnimGraphAddStateAction
        : public MCore::Command
        , public EMotionFX::ParameterMixinAnimGraphId
        , public EMotionFX::ParameterMixinAnimGraphNodeId
    {
    public:
        AZ_RTTI(CommandAnimGraphAddStateAction, "{0644552D-D4B5-4396-A031-BC85EB3B0D96}", MCore::Command, ParameterMixinAnimGraphId, ParameterMixinAnimGraphNodeId)
        AZ_CLASS_ALLOCATOR_DECL

        CommandAnimGraphAddStateAction(MCore::Command* orgCommand = nullptr);

        bool Execute(const MCore::CommandLine& parameters, AZStd::string& outResult) override;
        bool Undo(const MCore::CommandLine& parameters, AZStd::string& outResult) override;

        static const char* s_commandName;
        void InitSyntax() override;
        bool SetCommandParameters(const MCore::CommandLine& parameters) override;

        bool GetIsUndoable() const override { return true; }
        const char* GetHistoryName() const override { return "Add trigger action to state"; }
        const char* GetDescription() const override;
        MCore::Command* Create() override { return aznew CommandAnimGraphAddStateAction(this); }

    private:
        size_t          m_oldActionIndex;
        bool            m_oldDirtyFlag;
        AZStd::string   m_oldContents;
    };


    void RemoveStateAction(const EMotionFX::AnimGraphNode* state, const size_t actionIndex,
        MCore::CommandGroup* commandGroup = nullptr, bool executeInsideCommand = false);

    class CommandAnimGraphRemoveStateAction
        : public MCore::Command
        , public EMotionFX::ParameterMixinAnimGraphId
        , public EMotionFX::ParameterMixinAnimGraphNodeId
    {
    public:
        AZ_RTTI(CommandAnimGraphRemoveStateAction, "{704C69D3-59BC-4832-A870-D371A7D8EA3E}", MCore::Command, ParameterMixinAnimGraphId, ParameterMixinAnimGraphNodeId)
        AZ_CLASS_ALLOCATOR_DECL

        CommandAnimGraphRemoveStateAction(MCore::Command* orgCommand = nullptr);

        bool Execute(const MCore::CommandLine& parameters, AZStd::string& outResult) override;
        bool Undo(const MCore::CommandLine& parameters, AZStd::string& outResult) override;

        static const char* s_commandName;
        void InitSyntax() override;
        bool SetCommandParameters(const MCore::CommandLine& parameters) override;

        bool GetIsUndoable() const override { return true; }
        const char* GetHistoryName() const override { return "Remove trigger action from state"; }
        const char* GetDescription() const override;
        MCore::Command* Create() override { return aznew CommandAnimGraphRemoveStateAction(this); }

    private:
        AZ::TypeId      m_oldActionType;
        size_t          m_oldActionIndex;
        AZStd::string   m_oldContents;
        bool            m_oldDirtyFlag;
    };
} // namespace CommandSystem
