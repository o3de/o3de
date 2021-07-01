/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/optional.h>
#include <EMotionFX/Source/AnimGraph.h>
#include "CommandSystemConfig.h"
#include <EMotionFX/Source/AnimGraph.h>
#include <MCore/Source/Command.h>
#include <MCore/Source/CommandLine.h>
#include "CommandManager.h"
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/BlendTreeConnection.h>
#include <EMotionFX/CommandSystem/Source/AnimGraphCopyPasteData.h>
#include <EMotionFX/CommandSystem/Source/ParameterMixins.h>


namespace CommandSystem
{
    // create a connection
    MCORE_DEFINECOMMAND_START(CommandAnimGraphCreateConnection, "Connect two anim graph nodes", true)
        uint32                              mAnimGraphID;
        EMotionFX::AnimGraphNodeId          mTargetNodeId;
        EMotionFX::AnimGraphNodeId          mSourceNodeId;
        EMotionFX::AnimGraphConnectionId    m_connectionId;
        AZ::TypeId                          mTransitionType;
        int32                               mStartOffsetX;
        int32                               mStartOffsetY;
        int32                               mEndOffsetX;
        int32                               mEndOffsetY;
        int32                               mSourcePort;
        int32                               mTargetPort;
        AZStd::string                       mSourcePortName;
        AZStd::string                       mTargetPortName;
        bool                                mOldDirtyFlag;
        bool                                mUpdateParamFlag;

    public:
        EMotionFX::AnimGraphConnectionId GetConnectionId() const{ return m_connectionId; }
        EMotionFX::AnimGraphNodeId GetTargetNodeId() const      { return mTargetNodeId; }
        EMotionFX::AnimGraphNodeId GetSourceNodeId() const      { return mSourceNodeId; }
        AZ::TypeId GetTransitionType() const                    { return mTransitionType; }
        int32 GetSourcePort() const                             { return mSourcePort; }
        int32 GetTargetPort() const                             { return mTargetPort; }
        int32 GetStartOffsetX() const                           { return mStartOffsetX; }
        int32 GetStartOffsetY() const                           { return mStartOffsetY; }
        int32 GetEndOffsetX() const                             { return mEndOffsetX; }
        int32 GetEndOffsetY() const                             { return mEndOffsetY; }
    MCORE_DEFINECOMMAND_END


    // remove a connection
    MCORE_DEFINECOMMAND_START(CommandAnimGraphRemoveConnection, "Remove a anim graph connection", true)
        uint32                              mAnimGraphID;
        EMotionFX::AnimGraphNodeId          mTargetNodeId;
        AZStd::string                       mTargetNodeName;
        EMotionFX::AnimGraphNodeId          mSourceNodeId;
        AZStd::string                       mSourceNodeName;
        EMotionFX::AnimGraphConnectionId    m_connectionId;
        AZ::TypeId                          mTransitionType;
        int32                               mStartOffsetX;
        int32                               mStartOffsetY;
        int32                               mEndOffsetX;
        int32                               mEndOffsetY;
        int32                               mSourcePort;
        int32                               mTargetPort;
        bool                                mOldDirtyFlag;
        AZStd::string                       mOldContents;

    public:
        EMotionFX::AnimGraphNodeId GetTargetNodeID() const      { return mTargetNodeId; }
        EMotionFX::AnimGraphNodeId GetSourceNodeID() const      { return mSourceNodeId; }
        AZ::TypeId GetTransitionType() const                    { return mTransitionType; }
        int32 GetSourcePort() const                             { return mSourcePort; }
        int32 GetTargetPort() const                             { return mTargetPort; }
        int32 GetStartOffsetX() const                           { return mStartOffsetX; }
        int32 GetStartOffsetY() const                           { return mStartOffsetY; }
        int32 GetEndOffsetX() const                             { return mEndOffsetX; }
        int32 GetEndOffsetY() const                             { return mEndOffsetY; }
        EMotionFX::AnimGraphConnectionId GetConnectionId() const{ return m_connectionId; }
    MCORE_DEFINECOMMAND_END


    // adjust transition
    void AdjustTransition(const EMotionFX::AnimGraphStateTransition* transition,
        const AZStd::optional<bool>& isDisabled = AZStd::nullopt,
        const AZStd::optional<AZStd::string>& sourceNode = AZStd::nullopt, const AZStd::optional<AZStd::string>& targetNode = AZStd::nullopt,
        const AZStd::optional<AZ::s32>& startOffsetX = AZStd::nullopt, const AZStd::optional<AZ::s32>& startOffsetY = AZStd::nullopt,
        const AZStd::optional<AZ::s32>& endOffsetX = AZStd::nullopt, const AZStd::optional<AZ::s32>& endOffsetY = AZStd::nullopt,
        const AZStd::optional<AZStd::string>& attributesString = AZStd::nullopt, const AZStd::optional<AZStd::string>& serializedMembers = AZStd::nullopt,
        MCore::CommandGroup* commandGroup = nullptr, bool executeInsideCommand = false);

    class CommandAnimGraphAdjustTransition
        : public MCore::Command
        , public EMotionFX::ParameterMixinTransitionId
        , public EMotionFX::ParameterMixinAttributesString
        , public EMotionFX::ParameterMixinSerializedMembers
    {
    public:
        AZ_RTTI(CommandAnimGraphAdjustTransition, "{B7EA2F2E-8C89-435B-B75A-92840E0A81B1}",
            EMotionFX::ParameterMixinTransitionId,
            EMotionFX::ParameterMixinAttributesString)
        AZ_CLASS_ALLOCATOR_DECL

        CommandAnimGraphAdjustTransition(MCore::Command* orgCommand = nullptr);

        bool Execute(const MCore::CommandLine& parameters, AZStd::string& outResult) override;
        bool Undo(const MCore::CommandLine& parameters, AZStd::string& outResult) override;

        static const char* s_commandName;
        void InitSyntax() override;
        bool SetCommandParameters(const MCore::CommandLine& parameters) override;

        bool GetIsUndoable() const override { return true; }
        const char* GetHistoryName() const override { return "Adjust state transition"; }
        const char* GetDescription() const override;
        MCore::Command* Create() override { return aznew CommandAnimGraphAdjustTransition(this); }

        void RewindTransitionIfActive(EMotionFX::AnimGraphStateTransition* transition);

    private:
        AZ::Outcome<AZStd::string>          m_oldSerializedMembers; // Without actions and conditions.
        bool                                mOldDirtyFlag = false;
    };

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Helper functions
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    COMMANDSYSTEM_API void DeleteNodeConnection(MCore::CommandGroup* commandGroup, const EMotionFX::AnimGraphNode* targetNode, const EMotionFX::BlendTreeConnection* connection, bool updateUniqueData = true);

    COMMANDSYSTEM_API void CreateNodeConnection(MCore::CommandGroup* commandGroup, EMotionFX::AnimGraphNode* targetNode, EMotionFX::BlendTreeConnection* connection);

    COMMANDSYSTEM_API void DeleteConnection(MCore::CommandGroup* commandGroup, EMotionFX::AnimGraphNode* node, EMotionFX::BlendTreeConnection* connection, AZStd::vector<EMotionFX::BlendTreeConnection*>& connectionList);
    COMMANDSYSTEM_API void DeleteNodeConnections(MCore::CommandGroup* commandGroup, EMotionFX::AnimGraphNode* node, EMotionFX::AnimGraphNode* parentNode, AZStd::vector<EMotionFX::BlendTreeConnection*>& connectionList, bool recursive);
    COMMANDSYSTEM_API void RelinkConnectionTarget(MCore::CommandGroup* commandGroup, const uint32 animGraphID, const char* sourceNodeName, uint32 sourcePortNr, const char* oldTargetNodeName, uint32 oldTargetPortNr, const char* newTargetNodeName, uint32 newTargetPortNr);
    COMMANDSYSTEM_API void RelinkConnectionSource(MCore::CommandGroup* commandGroup, const uint32 animGraphID, const char* oldSourceNodeName, uint32 oldSourcePortNr, const char* newSourceNodeName, uint32 newSourcePortNr, const char* targetNodeName, uint32 targetPortNr);

    COMMANDSYSTEM_API void DeleteStateTransition(MCore::CommandGroup* commandGroup, EMotionFX::AnimGraphStateTransition* transition, AZStd::vector<EMotionFX::AnimGraphStateTransition*>& transitionList);
    COMMANDSYSTEM_API void DeleteStateTransitions(MCore::CommandGroup* commandGroup, EMotionFX::AnimGraphNode* state, EMotionFX::AnimGraphNode* parentNode, AZStd::vector<EMotionFX::AnimGraphStateTransition*>& transitionList, bool recursive);

    COMMANDSYSTEM_API void CopyBlendTreeConnection(MCore::CommandGroup* commandGroup, EMotionFX::AnimGraph* targetAnimGraph, EMotionFX::AnimGraphNode* targetNode, EMotionFX::BlendTreeConnection* connection,
        bool cutMode, AZStd::unordered_map<AZ::u64, AZ::u64>& convertedIds, AnimGraphCopyPasteData& copyPasteData);
    COMMANDSYSTEM_API void CopyStateTransition(MCore::CommandGroup* commandGroup, EMotionFX::AnimGraph* targetAnimGraph, EMotionFX::AnimGraphStateTransition* transition,
        bool cutMode, AZStd::unordered_map<AZ::u64, AZ::u64>& convertedIds, AnimGraphCopyPasteData& copyPasteData);

    COMMANDSYSTEM_API EMotionFX::AnimGraph* CommandsGetAnimGraph(const MCore::CommandLine& parameters, MCore::Command* command, AZStd::string& outResult);
} // namespace CommandSystem
