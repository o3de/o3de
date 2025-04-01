/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/BlendTree.h>
#include <EMotionFX/Source/BlendTreeFloatConstantNode.h>
#include <EMotionFX/Source/BlendSpace1DNode.h>
#include <EMotionFX/Source/BlendSpace2DNode.h>
#include <EMotionFX/Source/EventHandler.h>
#include <EMotionFX/Source/EventInfo.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/MotionInstance.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/Motion.h>
#include <EMotionFX/Source/Skeleton.h>
#include <EMotionFX/Source/Transform.h>
#include <Tests/AnimGraphFixture.h>
#include <Tests/TestAssetCode/ActorFactory.h>

namespace EMotionFX
{
    class BlendSpaceFixture
        : public AnimGraphFixture
    {
    public:
        void ConstructGraph() override;
        void SetUp() override;
        void TearDown() override;

        size_t CreateSubMotion(Motion* motion, const std::string& name, const Transform& transform);
        void CreateMotions();

        MotionSet::MotionEntry* FindMotionEntry(Motion* motion) const;
        void AddEvent(Motion* motion, float time);

        class TestEventHandler
            : public EventHandler
        {
        public:
            AZ_CLASS_ALLOCATOR_DECL
            const AZStd::vector<EventTypes> GetHandledEventTypes() const
            {
                return
                {
                    EVENT_TYPE_ON_EVENT,
                    EVENT_TYPE_ON_HAS_LOOPED,
                    EVENT_TYPE_ON_STATE_ENTERING,
                    EVENT_TYPE_ON_STATE_ENTER,
                    EVENT_TYPE_ON_STATE_END,
                    EVENT_TYPE_ON_STATE_EXIT,
                    EVENT_TYPE_ON_START_TRANSITION,
                    EVENT_TYPE_ON_END_TRANSITION
                };
            }

            MOCK_METHOD1(OnEvent, void(const EventInfo&));
        };

    public:
        BlendTree* m_blendTree = nullptr;
        BlendTreeFinalNode* m_finalNode = nullptr;

        std::string m_rootJointName;

        TestEventHandler* m_eventHandler = nullptr;

        Motion* m_idleMotion = nullptr;
        Motion* m_forwardMotion = nullptr;
        Motion* m_runMotion = nullptr;
        Motion* m_strafeMotion = nullptr;
        Motion* m_rotateLeftMotion = nullptr;
        Motion* m_forwardStrafe45 = nullptr;
        Motion* m_forwardSlope45 = nullptr;
        std::vector<Motion*> m_motions;
    };

    class BlendSpace1DFixture
        : public BlendSpaceFixture
    {
    public:
        void ConstructGraph() override;
        void SetUp() override;

    public:
        BlendSpace1DNode* m_blendSpace1DNode = nullptr;
        BlendTreeFloatConstantNode* m_floatNodeX = nullptr;
    };

    class BlendSpace2DFixture
        : public BlendSpaceFixture
    {
    public:
        void ConstructGraph() override;
        void SetUp() override;

    public:
        BlendSpace2DNode* m_blendSpace2DNode = nullptr;
        BlendTreeFloatConstantNode* m_floatNodeX = nullptr;
        BlendTreeFloatConstantNode* m_floatNodeY = nullptr;
    };
}
