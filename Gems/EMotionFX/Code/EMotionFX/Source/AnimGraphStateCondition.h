/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/EventHandler.h>
#include <EMotionFX/Source/AnimGraphTransitionCondition.h>

namespace EMotionFX
{
    // forward declarations
    class AnimGraphInstance;
    class AnimGraphNode;

    /**
     *
     *
     *
     */
    class EMFX_API AnimGraphStateCondition
        : public AnimGraphTransitionCondition
    {
    private:
        class EventHandler;

    public:
        AZ_RTTI(AnimGraphStateCondition, "{8C955719-5D14-4BB5-BA64-F2A3385CAF7E}", AnimGraphTransitionCondition)
        AZ_CLASS_ALLOCATOR_DECL

        enum TestFunction : AZ::u8
        {
            FUNCTION_EXITSTATES         = 0,
            FUNCTION_ENTERING           = 1,
            FUNCTION_ENTER              = 2,
            FUNCTION_EXIT               = 3,
            FUNCTION_END                = 4,
            FUNCTION_PLAYTIME           = 5,
            FUNCTION_NONE               = 6
        };

        // the unique data
        class EMFX_API UniqueData
            : public AnimGraphObjectData
        {
            EMFX_ANIMGRAPHOBJECTDATA_IMPLEMENT_LOADSAVE

        public:
            AZ_CLASS_ALLOCATOR_DECL

            UniqueData(AnimGraphObject* object, AnimGraphInstance* animGraphInstance);
            virtual ~UniqueData();

            void CreateEventHandler();
            void DeleteEventHandler();

        public:
            // The anim graph instance pointer shouldn't change. If it were to
            // change, we'd need to remove an existing event handler and create
            // a new one in the new anim graph instance.
            AnimGraphInstance* const                    mAnimGraphInstance;
            AnimGraphStateCondition::EventHandler*      mEventHandler;
            bool                                        mTriggered;
        };

        AnimGraphStateCondition();
        AnimGraphStateCondition(AnimGraph* animGraph);
        ~AnimGraphStateCondition();

        void Reinit() override;
        bool InitAfterLoading(AnimGraph* animGraph) override;

        AnimGraphObjectData* CreateUniqueData(AnimGraphInstance* animGraphInstance) override { return aznew UniqueData(this, animGraphInstance); }
        void OnRemoveNode(AnimGraph* animGraph, AnimGraphNode* nodeToRemove) override;

        void Reset(AnimGraphInstance* animGraphInstance) override;

        void GetSummary(AZStd::string* outResult) const override;
        void GetTooltip(AZStd::string* outResult) const override;
        const char* GetPaletteName() const override;
        ECategory GetPaletteCategory() const override;

        bool TestCondition(AnimGraphInstance* animGraphInstance) const override;

        void SetStateId(AnimGraphNodeId stateId);
        AnimGraphNodeId GetStateId() const;
        AnimGraphNode* GetState() const;

        void SetPlayTime(float playTime);
        float GetPlayTime() const;

        void SetTestFunction(TestFunction testFunction);
        TestFunction GetTestFunction() const;
        const char* GetTestFunctionString() const;

        void GetAttributeStringForAffectedNodeIds(const AZStd::unordered_map<AZ::u64, AZ::u64>& convertedIds, AZStd::string& attributesString) const override;

        static void Reflect(AZ::ReflectContext* context);

    private:
        // the event handler
        class EMFX_API EventHandler
            : public AnimGraphInstanceEventHandler
        {
        public:
            AZ_CLASS_ALLOCATOR_DECL

            EventHandler(AnimGraphStateCondition* condition, UniqueData* uniqueData);
            virtual ~EventHandler();

            const AZStd::vector<EventTypes> GetHandledEventTypes() const override;

            // overloaded
            void OnStateEnter(AnimGraphInstance* animGraphInstance, AnimGraphNode* state) override;
            void OnStateEntering(AnimGraphInstance* animGraphInstance, AnimGraphNode* state) override;
            void OnStateExit(AnimGraphInstance* animGraphInstance, AnimGraphNode* state) override;
            void OnStateEnd(AnimGraphInstance* animGraphInstance, AnimGraphNode* state) override;

        private:
            bool IsTargetState(const AnimGraphNode* state) const;
            void OnStateChange(AnimGraphInstance* animGraphInstance, AnimGraphNode* state, TestFunction targetFunction);

            AnimGraphStateCondition*    mCondition;
            UniqueData*                 mUniqueData;
        };

        AZ::Crc32 GetTestFunctionVisibility() const;
        AZ::Crc32 GetPlayTimeVisibility() const;

        static const char* s_functionExitStateReached;
        static const char* s_functionStartedTransitioning;
        static const char* s_functionStateFullyBlendedIn;
        static const char* s_functionLeavingState;
        static const char* s_functionStateFullyBlendedOut;
        static const char* s_functionHasReachedSpecifiedPlaytime;

        AZ::u64             m_stateId;
        AnimGraphNode*      m_state;
        float               m_playTime;
        TestFunction        m_testFunction;
    };
} // namespace EMotionFX
