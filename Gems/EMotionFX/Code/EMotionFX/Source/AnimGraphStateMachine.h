/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/containers/vector.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphNodeData.h>
#include <EMotionFX/Source/AnimGraphRefCountedData.h>


namespace EMotionFX
{
    // forward declarations
    class AnimGraphInstance;
    class ActorInstance;
    class AnimGraphStateTransition;

    class EMFX_API AnimGraphStateMachine
        : public AnimGraphNode
    {
    public:
        AZ_RTTI(AnimGraphStateMachine, "{272E90D2-8A18-46AF-AD82-6A8B7EC508ED}", AnimGraphNode)
        AZ_CLASS_ALLOCATOR_DECL

        enum
        {
            OUTPUTPORT_POSE     = 0
        };

        enum
        {
            PORTID_OUTPUT_POSE  = 0
        };

        class EMFX_API UniqueData
            : public AnimGraphNodeData
            , public NodeDataAutoRefCountMixin
        {
        public:
            AZ_CLASS_ALLOCATOR_DECL

            UniqueData(AnimGraphNode* node, AnimGraphInstance* animGraphInstance);
            void Reset() override;
            void Update() override;
            const AZStd::vector<AnimGraphNode*>& GetActiveStates(); // TODO: Make constant see if there is a way to keep it up to date and allow access to the array directly.

            uint32 Save(uint8* outputBuffer) const override;
            uint32 Load(const uint8* dataBuffer) override;

        public:
            AZStd::vector<AnimGraphStateTransition*>    m_activeTransitions;    /**< Stack of active transitions. */
            AnimGraphNode*                              m_currentState;          /**< The current state. */
            AnimGraphNode*                              m_previousState;         /**< The previously used state, so the one used before the current one, the one from which we transitioned into the current one. */
            bool                                        m_reachedExitState;      /**< True in case the state machine's current state is an exit state, false it not. */
            AnimGraphRefCountedData                     m_prevData;
            bool                                        m_switchToEntryState;

        private:
            AZStd::vector<AnimGraphNode*>               m_activeStates;         // TODO: See function comment.
        };

        AnimGraphStateMachine();
        ~AnimGraphStateMachine() override;

        void RecursiveReinit() override;
        bool InitAfterLoading(AnimGraph* animGraph) override;

        AnimGraphObjectData* CreateUniqueData(AnimGraphInstance* animGraphInstance) override { return aznew UniqueData(this, animGraphInstance); }
        void RecursiveInvalidateUniqueDatas(AnimGraphInstance* animGraphInstance) override;

        void OnRemoveNode(AnimGraph* animGraph, AnimGraphNode* nodeToRemove) override;
        void RecursiveOnChangeMotionSet(AnimGraphInstance* animGraphInstance, MotionSet* newMotionSet) override;
        void Rewind(AnimGraphInstance* animGraphInstance) override;

        void RecursiveResetUniqueDatas(AnimGraphInstance* animGraphInstance) override;
        void RecursiveResetFlags(AnimGraphInstance* animGraphInstance, uint32 flagsToDisable = 0xffffffff) override;

        const char* GetPaletteName() const override                     { return "State Machine"; }
        AnimGraphObject::ECategory GetPaletteCategory() const override { return AnimGraphObject::CATEGORY_SOURCES; }
        bool GetIsDeletable() const override;
        bool GetCanActAsState() const override                          { return true; }
        bool GetHasVisualGraph() const override                         { return true; }
        bool GetCanHaveChildren() const override                        { return true; }
        bool GetSupportsDisable() const override                        { return true; }
        bool GetSupportsVisualization() const override                  { return true; }
        bool GetHasOutputPose() const override                          { return true; }
        AZ::Color GetHasChildIndicatorColor() const override            { return AZ::Color(0.25f, 0.38f, 0.97f, 1.0f); }

        AnimGraphPose* GetMainOutputPose(AnimGraphInstance* animGraphInstance) const override             { return GetOutputPose(animGraphInstance, OUTPUTPORT_POSE)->GetValue(); }

        void RecursiveCollectObjects(AZStd::vector<AnimGraphObject*>& outObjects) const override;

        void RecursiveCollectObjectsOfType(const AZ::TypeId& objectType, AZStd::vector<AnimGraphObject*>& outObjects) const override;

        //----------------------------------------------------------------------------------------------------------------------------

        /**
         * Add the given transition to the state machine.
         * The state machine will take care of destructing the memory of the transition once it gets destructed itself or by using the RemoveTransition() function.
         * @param[in] transition A pointer to the state machine transition to add.
         */
        void AddTransition(AnimGraphStateTransition* transition);

        /**
         * Get the number of transitions inside this state machine. This includes all kinds of transitions, so also wildcard transitions.
         * @result The number of transitions inside the state machine.
         */
        size_t GetNumTransitions() const                                            { return m_transitions.size(); }

        /**
         * Get a pointer to the state machine transition of the given index.
         * @param[in] index The index of the transition to return.
         * @result A pointer to the state machine transition at the given index.
         */
        AnimGraphStateTransition* GetTransition(size_t index) const                 { return m_transitions[index]; }

        /**
         * Remove the state machine transition at the given index.
         * @param[in] transitionIndex The index of the transition to remove.
         * @param delFromMem Set to true (default) when you wish to also delete the specified transition from memory.
         */
        void RemoveTransition(size_t transitionIndex, bool delFromMem = true);

        /**
         * Remove all transitions from the state machine and also get rid of the allocated memory. This will automatically be called in the state machine destructor.
         */
        void RemoveAllTransitions();

        void ReserveTransitions(size_t numTransitions);

        /**
         * Find the transition index for the given transition id.
         * @param[in] transitionId The identification number to search for.
         * @result The index of the transition.
         */
        AZ::Outcome<size_t> FindTransitionIndexById(AnimGraphConnectionId transitionId) const;

        /**
         * Find the transition index by comparing pointers.
         * @param[in] transition A pointer to the transition to search the index for.
         * @result The index of the transition.
         */
        AZ::Outcome<size_t> FindTransitionIndex(const AnimGraphStateTransition* transition) const;

        /**
         * Find the transition by the given transition id.
         * @param[in] transitionId The identification number to search for.
         * @result A pointer to the transition. nullptr in case no transition has been found.
         */
        AnimGraphStateTransition* FindTransitionById(AnimGraphConnectionId transitionId) const;

        /**
         * Check if there is a wildcard transition with the given state as target node. Each state can only have one wildcard transition. The wildcard transition will be used in case there
         * is no other connection between the current state and the given state.
         * @param[in] state A pointer to the state to check.
         * @result True in case the given state has a wildcard transition, false if not.
         */
        bool CheckIfHasWildcardTransition(AnimGraphNode* state) const;

        /**
         * Check if the state machine is transitioning at the moment.
         * @param[in] animGraphInstance The anim graph instance to check.
         * @return True in case the state machine is transitioning at the moment, false in case a state is fully active and blended in.
         */
        bool IsTransitioning(AnimGraphInstance* animGraphInstance) const;

        /**
         * Check if the given transition is currently active.
         * @param[in] transition The transition to check.
         * @param[in] animGraphInstance The anim graph instance to check.
         * @return True in case the transition is active, on the transition stack and currently transitioning, false if not.
         */
        bool IsTransitionActive(const AnimGraphStateTransition* transition, AnimGraphInstance* animGraphInstance) const;

        /**
         * Get the latest active transition. The latest active transition is the one that got started most recently, is still transitioning
         * and defines where the state machine is actually going. All other transitions on the transition stack got interrupted.
         * @param[in] animGraphInstance The instance for the state machine to check.
         * @result The latest active transition.
         */
        AnimGraphStateTransition* GetLatestActiveTransition(AnimGraphInstance* animGraphInstance) const;

        /**
         * Get all currently active transitions.
         * @param[in] animGraphInstance The anim graph instance to check.
         * @return Transition stack containing all active transitions. An empty stack means that there is no transition active currently.
         */
        const AZStd::vector<AnimGraphStateTransition*>& GetActiveTransitions(AnimGraphInstance* animGraphInstance) const;

        AnimGraphStateTransition* FindTransition(AnimGraphInstance* animGraphInstance, AnimGraphNode* currentState, AnimGraphNode* targetState) const;

        uint32 CalcNumIncomingTransitions(AnimGraphNode* state) const;
        uint32 CalcNumOutgoingTransitions(AnimGraphNode* state) const;
        uint32 CalcNumWildcardTransitions(AnimGraphNode* state) const;

        /**
         * In case blend times are set to 0.0, there are scenarios where the state machine starts and ends multiple transitions, going forward
         * multiple states within a single frame. This function returns the maximum number of possible passes.
         */
        static AZ::u32 GetMaxNumPasses();

        static AnimGraphStateMachine* GetGrandParentStateMachine(const AnimGraphNode* state);

        //----------------------------------------------------------------------------------------------------------------------------

        const AZStd::vector<AnimGraphNode*>& GetActiveStates(AnimGraphInstance* animGraphInstance) const;

        /**
         * Get the initial state of the state machine.
         * The start state is usually shown drawn with an arrow "pointing at it from any where".
         * @return A pointer to the start state of the state machine.
         */
        AnimGraphNode* GetEntryState();

        AZ_FORCE_INLINE AnimGraphNodeId GetEntryStateId() const                     { return m_entryStateId; }

        /**
         * Set the initial state of the state machine.
         * The start state is usually shown drawn with an arrow "pointing at it from any where".
         * @param[in] entryState A pointer to the start state of the state machine.
         */
        void SetEntryState(AnimGraphNode* entryState);

        /**
         * Get the currently active state.
         * @param[in] animGraphInstance The anim graph instance to use.
         * @return A pointer to the currently active state. In case the state machine is transitioning this will be the source state.
         */
        AnimGraphNode* GetCurrentState(AnimGraphInstance* animGraphInstance);

        /**
         * Check if the state machine has reached an exit state.
         * @param[in] animGraphInstance The anim graph instance to check.
         * @return True in case the state machine has reached an exit state, false in case not.
         */
        bool GetExitStateReached(AnimGraphInstance* animGraphInstance) const;

        /**
         *
         *
         *
         *
         */
        void SwitchToState(AnimGraphInstance* animGraphInstance, AnimGraphNode* targetState);
        void TransitionToState(AnimGraphInstance* animGraphInstance, AnimGraphNode* targetState);

        void RecursiveSetUniqueDataFlag(AnimGraphInstance* animGraphInstance, uint32 flag, bool enabled) override;
        void RecursiveCollectActiveNodes(AnimGraphInstance* animGraphInstance, AZStd::vector<AnimGraphNode*>* outNodes, const AZ::TypeId& nodeType) const override;
        void RecursiveCollectActiveNetTimeSyncNodes(AnimGraphInstance* animGraphInstance, AZStd::vector<AnimGraphNode*>* outNodes) const override;

        //----------------------------------------------------------------------------------------------------------------------------

        void SetAlwaysStartInEntryState(bool alwaysStartInEntryState);
        void SetEntryStateId(AnimGraphNodeId entryStateNodeId);

        static void Reflect(AZ::ReflectContext* context);

        void EndAllActiveTransitions(AnimGraphInstance* animGraphInstance);

    private:
        AZStd::vector<AnimGraphStateTransition*>    m_transitions; /**< The higher the index, the older the active transtion, the more time passed since it got started. Index = 0 is the most recent transition and the one with the highest global influence.*/
        AnimGraphNode*                              m_entryState;                /**< A pointer to the initial state, so the state where the machine starts. */
        size_t                                      m_entryStateNodeNr;          /**< Used only in the legacy file format. Remove after the legacy file format will be removed. */
        AZ::u64                                     m_entryStateId;             /**< The node id of the entry state. */
        bool                                        m_alwaysStartInEntryState;

        static AZ::u32                              s_maxNumPasses;

        /**
         * Reset all conditions from wild card and outgoing transitions of the given state.
         * @param[in] animGraphInstance A pointer to the anim graph instance.
         * @param[in] state All conditions of transitions starting from this node will get reset.
         */
        void ResetOutgoingTransitionConditions(AnimGraphInstance* animGraphInstance, AnimGraphNode* state);

        bool IsTransitioning(const UniqueData* uniqueData) const;

        bool IsLatestActiveTransitionDone(AnimGraphInstance* animGraphInstance, const UniqueData* uniqueData) const;

        void UpdateExitStateReachedFlag(AnimGraphInstance* animGraphInstance, UniqueData* uniqueData);
        void StartTransition(AnimGraphInstance* animGraphInstance, UniqueData* uniqueData, AnimGraphStateTransition* transition, bool calledFromWithinUpdate = false);

        /**
         * This function is only allowed to be called within the state machine Update() call.
         */
        void CheckConditions(AnimGraphNode* sourceNode, AnimGraphInstance* animGraphInstance, UniqueData* uniqueData, bool allowTransition = true);
        void UpdateConditions(AnimGraphInstance* animGraphInstance, AnimGraphNode* animGraphNode, float timePassedInSeconds);
        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void PostUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void Output(AnimGraphInstance* animGraphInstance) override;
        void TopDownUpdate(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        void SkipPostUpdate(AnimGraphInstance* animGraphInstance) override;
        void SkipOutput(AnimGraphInstance* animGraphInstance) override;

        void LogTransitionStack(const char* stateDescription, AnimGraphInstance* animGraphInstance, const UniqueData* uniqueData) const;

        void PushTransitionStack(UniqueData* uniqueData, AnimGraphStateTransition* transition);

        /**
         * Get the latest active transition. The latest active transition is the one that got started most recently, is still transitioning
         * and defines where the state machine is actually going. All other transitions on the transition stack got interrupted.
         * @param[in] uniqueData The instance data for the state machine to check.
         * @result The latest active transition.
         */
        AnimGraphStateTransition* GetLatestActiveTransition(const UniqueData* uniqueData) const;

        void EndTransition(AnimGraphStateTransition* transition, AnimGraphInstance* animGraphInstance, UniqueData* uniqueData);
        void EndAllActiveTransitions(AnimGraphInstance* animGraphInstance, UniqueData* uniqueData);
    };
} // namespace EMotionFX
