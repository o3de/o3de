/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "CommandSystemConfig.h"
#include <MCore/Source/Command.h>
#include <MCore/Source/CommandGroup.h>
#include <EMotionFX/Source/AnimGraphNodeGroup.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/CommandSystem/Source/ParameterMixins.h>

namespace CommandSystem
{
    // adjust a node group
    class CommandAnimGraphAdjustNodeGroup
        : public MCore::Command
        , public EMotionFX::ParameterMixinAnimGraphId
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        static constexpr inline AZStd::string_view s_commandName = "AnimGraphAdjustNodeGroup";

        enum class NodeAction
        {
            Add,
            Remove,
            Replace
        };

        explicit CommandAnimGraphAdjustNodeGroup(
            MCore::Command* orgCommand = nullptr,
            AZ::u32 animGraphId = MCORE_INVALIDINDEX32,
            AZStd::string name = AZStd::string{},
            AZStd::optional<bool> visible = AZStd::nullopt,
            AZStd::optional<AZStd::string> newName = AZStd::nullopt,
            AZStd::optional<AZStd::vector<AZStd::string>> nodeNames = AZStd::nullopt,
            AZStd::optional<NodeAction> nodeAction = AZStd::nullopt,
            AZStd::optional<AZ::u32> color = AZStd::nullopt,
            AZStd::optional<bool> updateUI = AZStd::nullopt
        );
        bool Execute(const MCore::CommandLine& parameters, AZStd::string& outResult) override;
        bool Undo(const MCore::CommandLine& parameters, AZStd::string& outResult) override;
        void InitSyntax() override;
        bool SetCommandParameters(const MCore::CommandLine& parameters) override;
        bool GetIsUndoable() const override
        {
            return true;
        }
        const char* GetHistoryName() const override
        {
            return "Adjust anim graph node group";
        }
        const char* GetDescription() const override;
        MCore::Command* Create() override
        {
            return aznew CommandAnimGraphAdjustNodeGroup(this);
        }

        static AZStd::vector<AZStd::string> GenerateNodeNameVector(EMotionFX::AnimGraph* animGraph, const AZStd::vector<EMotionFX::AnimGraphNodeId>& nodeIDs);
        static AZStd::vector<EMotionFX::AnimGraphNodeId> CollectNodeIdsFromGroup(EMotionFX::AnimGraphNodeGroup* nodeGroup);

    private:
        AZStd::string m_name;
        AZStd::optional<bool> m_isVisible;
        AZStd::optional<AZStd::string> m_newName;
        AZStd::optional<AZStd::vector<AZStd::string>> m_nodeNames;
        AZStd::optional<NodeAction> m_nodeAction;
        AZStd::optional<AZ::u32> m_color;
        AZStd::optional<bool> m_updateUI;

        bool m_oldIsVisible;
        AZ::u32 m_oldColor;
        AZStd::vector<EMotionFX::AnimGraphNodeId> m_oldNodeIds;
        bool m_oldDirtyFlag;
    };

    // add node group
    MCORE_DEFINECOMMAND_START(CommandAnimGraphAddNodeGroup, "Add anim graph node group", true)
    bool                    m_oldDirtyFlag;
    AZStd::string           m_oldName;
    MCORE_DEFINECOMMAND_END


    // remove a node group
    MCORE_DEFINECOMMAND_START(CommandAnimGraphRemoveNodeGroup, "Remove anim graph node group", true)
    AZStd::string                               m_oldName;
    bool                                        m_oldIsVisible;
    AZ::u32                                     m_oldColor;
    AZStd::vector<EMotionFX::AnimGraphNodeId>   m_oldNodeIds;
    bool                                        m_oldDirtyFlag;
    MCORE_DEFINECOMMAND_END

    // helper function
    COMMANDSYSTEM_API void ClearNodeGroups(EMotionFX::AnimGraph* animGraph, MCore::CommandGroup* commandGroup = nullptr);
} // namespace CommandSystem
