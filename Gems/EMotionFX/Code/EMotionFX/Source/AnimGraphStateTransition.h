/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/ReflectContext.h>
#include <EMotionFX/Source/AnimGraphObjectIds.h>
#include <EMotionFX/Source/AnimGraphObject.h>
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/TriggerActionSetup.h>


namespace EMotionFX
{
    // forward declarations
    class AnimGraphInstance;
    class AnimGraphStateMachine;
    class AnimGraphTransitionCondition;
    class AnimGraphTriggerAction;
    class AnimGraphStateMachine;
    class Transform;
    class Pose;

    class EMFX_API AnimGraphStateTransition
        : public AnimGraphObject
    {
    public:
        AZ_RTTI(AnimGraphStateTransition, "{E69C8C6E-7066-43DD-B1BF-0D2FFBDDF457}", AnimGraphObject)
        AZ_CLASS_ALLOCATOR_DECL

        enum EInterpolationType : AZ::u8
        {
            INTERPOLATIONFUNCTION_LINEAR        = 0,
            INTERPOLATIONFUNCTION_EASECURVE     = 1
        };

        enum EInterruptionMode : AZ::u8
        {
            AlwaysAllowed = 0,
            MaxBlendWeight = 1
        };

        enum EInterruptionBlendBehavior : AZ::u8
        {
            Continue = 0,
            Stop = 1
        };

        class EMFX_API UniqueData
            : public AnimGraphObjectData
        {
            EMFX_ANIMGRAPHOBJECTDATA_IMPLEMENT_LOADSAVE

        public:
            AZ_CLASS_ALLOCATOR_DECL

            UniqueData(AnimGraphObject* object, AnimGraphInstance* animGraphInstance);
            ~UniqueData() = default;

        public:
            AnimGraphNode* m_sourceNode = nullptr;
            float m_blendWeight = 0.0f;
            float m_blendProgress = 0.0f;
            float m_totalSeconds = 0.0f;
            bool m_isDone = false;
        };

        class StateFilterLocal final
        {
        public:
            AZ_RTTI(AnimGraphStateTransition::StateFilterLocal, "{591DDCDE-F85D-4F35-957F-F2428ADE8579}")
            AZ_CLASS_ALLOCATOR_DECL

            bool IsEmpty() const;
            void Clear();

            size_t GetNumStates() const;
            AnimGraphNodeId GetStateId(size_t index) const;
            AZStd::vector<AnimGraphNodeId> CollectStateIds() const;
            void SetStateIds(const AZStd::vector<AnimGraphNodeId>& stateIds);

            size_t GetNumGroups() const;
            const AZStd::string& GetGroupName(size_t index) const;
            const AZStd::vector<AZStd::string>& GetGroups() const;
            void SetGroups(const AZStd::vector<AZStd::string>& groups);

            // Collect all individual states as well as the ones coming from node groups.
            AZStd::vector<AnimGraphNodeId> CollectStates(AnimGraphStateMachine* stateMachine) const;

            bool Contains(AnimGraph* animGraph, AnimGraphNodeId stateId) const;

            static void Reflect(AZ::ReflectContext* context);

        private:
            AZStd::vector<AZ::u64> m_stateIds;
            AZStd::vector<AZStd::string> m_nodeGroupNames;
        };

        AnimGraphStateTransition();
        AnimGraphStateTransition(
            AnimGraphNode* source,
            AnimGraphNode* target,
            AZStd::vector<AnimGraphTransitionCondition*> conditions = {},
            float duration = 0.3f
        );
        ~AnimGraphStateTransition() override;

        void Reinit() override;
        void RecursiveReinit() override;
        bool InitAfterLoading(AnimGraph* animGraph) override;

        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void OnRemoveNode(AnimGraph* animGraph, AnimGraphNode* nodeToRemove) override;

        AnimGraphObjectData* CreateUniqueData(AnimGraphInstance* animGraphInstance) override { return aznew UniqueData(this, animGraphInstance); }
        void InvalidateUniqueData(AnimGraphInstance* animGraphInstance) override;

        void RecursiveCollectObjects(AZStd::vector<AnimGraphObject*>& outObjects) const override;
        void ExtractMotion(AnimGraphInstance* animGraphInstance, AnimGraphRefCountedData* sourceData, Transform* outTransform, Transform* outTransformMirrored) const;

        void OnStartTransition(AnimGraphInstance* animGraphInstance);
        void OnEndTransition(AnimGraphInstance* animGraphInstance);
        bool GetIsDone(AnimGraphInstance* animGraphInstance) const;
        float GetBlendWeight(AnimGraphInstance* animGraphInstance) const;

        void CalcTransitionOutput(AnimGraphInstance* animGraphInstance, const AnimGraphPose& from, const AnimGraphPose& to, AnimGraphPose* outputPose) const;

        bool CheckIfIsReady(AnimGraphInstance* animGraphInstance) const;

        void SetBlendTime(float blendTime);
        float GetBlendTime(AnimGraphInstance* animGraphInstance) const;

        const char* GetPaletteName() const override;
        AnimGraphObject::ECategory GetPaletteCategory() const override;

        bool GetIsStateTransitionNode() const;

        void SetPriority(AZ::u32 priority);
        AZ::u32 GetPriority() const;

        void SetCanBeInterrupted(bool canBeInterrupted);
        void SetCanBeInterruptedBy(const AZStd::vector<AZ::u64>& transitionIds) { m_canBeInterruptedByTransitionIds = transitionIds; }
        void SetCanBeInterruptedBy(const AZStd::vector<AnimGraphConnectionId>& transitionIds);
        bool CanBeInterruptedBy(const AnimGraphStateTransition* transition, AnimGraphInstance* animGraphInstance = nullptr) const;
        const AZStd::vector<AZ::u64>& GetCanBeInterruptedByTransitionIds() const { return m_canBeInterruptedByTransitionIds; }

        void SetInterruptionMode(EInterruptionMode mode) { m_interruptionMode = mode; }
        EInterruptionMode GetInterruptionMode() const { return m_interruptionMode; }

        void SetMaxInterruptionBlendWeight(float weight) { m_maxInterruptionBlendWeight = weight; }
        float GetMaxInterruptionBlendWeight() const { return m_maxInterruptionBlendWeight; }

        void SetInterruptionBlendBehavior(EInterruptionBlendBehavior blendBehavior) { m_interruptionBlendBehavior = blendBehavior; }
        EInterruptionBlendBehavior GetInterruptionBlendBehavior() const { return m_interruptionBlendBehavior; }

        void SetCanInterruptOtherTransitions(bool canInterruptOtherTransitions);
        bool GetCanInterruptOtherTransitions() const;
        bool GotInterrupted(AnimGraphInstance* animGraphInstance) const;

        void SetCanInterruptItself(bool canInterruptItself);
        bool GetCanInterruptItself() const;

        void SetIsDisabled(bool isDisabled);
        bool GetIsDisabled() const;

        void SetSyncMode(AnimGraphStateTransition::ESyncMode syncMode);
        ESyncMode GetSyncMode() const;

        void SetEventFilterMode(AnimGraphObject::EEventMode eventMode);
        EEventMode GetEventFilterMode() const;

        /**
         * Get the unique identification number for the transition.
         * @return The unique identification number.
         */
        AnimGraphConnectionId GetId() const                                                         { return m_id; }

        /**
         * Set the unique identification number for the transition.
         * @param[in] id The unique identification number.
         */
        void SetId(AnimGraphConnectionId id)                                                        { m_id = id; }

        /**
         * Set if the transition is a wildcard transition or not. A wildcard transition is a transition that will be used in case there is no other path from the current
         * to the destination state. It is basically a transition from all nodes to the destination node of the wildcard transition. A wildcard transition does not have a fixed source node.
         * @param[in] isWildcardTransition Pass true in case the transition is a wildcard transition, false if not.
         */
        void SetIsWildcardTransition(bool isWildcardTransition);

        void SetSourceNode(AnimGraphInstance* animGraphInstance, AnimGraphNode* sourceNode);
        void SetSourceNode(AnimGraphNode* node);
        AnimGraphNode* GetSourceNode(AnimGraphInstance* animGraphInstance) const;
        AnimGraphNode* GetSourceNode() const;
        AZ_FORCE_INLINE AnimGraphNodeId GetSourceNodeId() const                                     { return m_sourceNodeId; }

        void SetTargetNode(AnimGraphNode* node);
        AnimGraphNode* GetTargetNode() const;
        AZ_FORCE_INLINE AnimGraphNodeId GetTargetNodeId() const                                     { return m_targetNodeId; }

        void SetVisualOffsets(int32 startX, int32 startY, int32 endX, int32 endY);
        int32 GetVisualStartOffsetX() const;
        int32 GetVisualStartOffsetY() const;
        int32 GetVisualEndOffsetX() const;
        int32 GetVisualEndOffsetY() const;

        EExtractionMode GetExtractionMode() const;
        void SetExtractionMode(EExtractionMode mode);

        /**
         * Check if the transition is a wildcard transition. A wildcard transition is a transition that will be used in case there is no other path from the current
         * to the destination state. It is basically a transition from all nodes to the destination node of the wildcard transition. A wildcard transition does not have a fixed source node.
         * @result True in case the transition is a wildcard transition, false if not.
         */
        bool GetIsWildcardTransition() const                                                    { return m_isWildcardTransition; }

        bool CanWildcardTransitionFrom(AnimGraphNode* sourceNode) const;

        AnimGraphStateMachine* GetStateMachine() const;

        MCORE_INLINE size_t GetNumConditions() const                                            { return m_conditions.size(); }
        MCORE_INLINE AnimGraphTransitionCondition* GetCondition(size_t index) const            { return m_conditions[index]; }
        AZ::Outcome<size_t> FindConditionIndex(AnimGraphTransitionCondition* condition) const;

        void AddCondition(AnimGraphTransitionCondition* condition);
        void InsertCondition(AnimGraphTransitionCondition* condition, size_t index);
        void ReserveConditions(size_t numConditions);
        void RemoveCondition(size_t index, bool delFromMem = true);
        void RemoveAllConditions(bool delFromMem = true);
        void ResetConditions(AnimGraphInstance* animGraphInstance);

        TriggerActionSetup& GetTriggerActionSetup() { return m_actionSetup; }
        const TriggerActionSetup& GetTriggerActionSetup() const { return m_actionSetup; }

        void SetGroups(const AZStd::vector<AZStd::string>& groups);
        void SetStateIds(const AZStd::vector<AnimGraphNodeId>& stateIds);

        void SetInterpolationType(AnimGraphStateTransition::EInterpolationType interpolationType);
        EInterpolationType GetInterpolationType() { return m_interpolationType; }
        void SetEaseInSmoothness(float easeInSmoothness);
        void SetEaseOutSmoothness(float easeInSmoothness);

        // Returns an attribute string (MCore::CommandLine formatted) if this condition is affected by a convertion of
        // node ids. The method will return the attribute string that will be used to patch this condition on a command
        virtual void GetAttributeStringForAffectedNodeIds(const AZStd::unordered_map<AZ::u64, AZ::u64>& convertedIds, AZStd::string& attributesString) const;

        static void Reflect(AZ::ReflectContext* context);

    protected:
        float CalculateWeight(float linearWeight) const;

        AZ::Crc32 GetEaseInOutSmoothnessVisibility() const;
        AZ::Crc32 GetVisibilityHideWhenExitOrEntry() const;
        AZ::Crc32 GetVisibilityAllowedStates() const;
        AZ::Crc32 GetVisibilityInterruptionProperties() const;
        AZ::Crc32 GetVisibilityCanBeInterruptedBy() const;
        AZ::Crc32 GetVisibilityMaxInterruptionBlendWeight() const;

        AZStd::vector<AnimGraphTransitionCondition*>    m_conditions{};
        StateFilterLocal                                m_allowTransitionsFrom;

        TriggerActionSetup                              m_actionSetup;
        AnimGraphNode*                                  m_sourceNode = nullptr;
        AnimGraphNode*                                  m_targetNode = nullptr;
        AZ::u64                                         m_sourceNodeId = AnimGraphNodeId::InvalidId;
        AZ::u64                                         m_targetNodeId = AnimGraphNodeId::InvalidId;
        AZ::u64                                         m_id = AnimGraphConnectionId::Create();                        /**< The unique identification number. */

        float                                           m_transitionTime = 0.3f;
        float                                           m_easeInSmoothness = 0.0f;
        float                                           m_easeOutSmoothness = 1.0f;
        AZ::s32                                         m_startOffsetX = 0;
        AZ::s32                                         m_startOffsetY = 0;
        AZ::s32                                         m_endOffsetX = 0;
        AZ::s32                                         m_endOffsetY = 0;
        AZ::u32                                         m_priority = 0;
        AnimGraphObject::ESyncMode                      m_syncMode = AnimGraphObject::SYNCMODE_DISABLED;
        AnimGraphObject::EEventMode                     m_eventMode = AnimGraphObject::EVENTMODE_BOTHNODES;
        AnimGraphObject::EExtractionMode                m_extractionMode = AnimGraphObject::EXTRACTIONMODE_BLEND;
        EInterpolationType                              m_interpolationType = INTERPOLATIONFUNCTION_LINEAR;
        bool                                            m_isWildcardTransition = false;      /**< Flag which indicates if the state transition is a wildcard transition or not. */
        bool                                            m_isDisabled = false;
        bool                                            m_canBeInterruptedByOthers = false;
        AZStd::vector<AZ::u64>                          m_canBeInterruptedByTransitionIds{};
        float                                           m_maxInterruptionBlendWeight = 1.0f;
        bool                                            m_canInterruptOtherTransitions = false;
        bool                                            m_allowSelfInterruption = false;
        EInterruptionBlendBehavior                      m_interruptionBlendBehavior = Continue;
        EInterruptionMode                               m_interruptionMode = AlwaysAllowed;
    };
}   // namespace EMotionFX
