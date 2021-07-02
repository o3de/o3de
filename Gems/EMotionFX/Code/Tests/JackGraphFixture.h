/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "SystemComponentFixture.h"
#include <Tests/TestAssetCode/AnimGraphFactory.h>

namespace EMotionFX
{
    class Actor;
    class ActorInstance;
    class AnimGraph;
    class AnimGraphInstance;
    class MotionSet;

    class JackGraphFixture : public SystemComponentFixture
    {
    public:
        virtual void SetUp() override;
        virtual void TearDown() override;

        virtual void OnPostActorCreated() {}
        virtual void ConstructGraph();

        void Evaluate(float timeDelta);
        void AddValueParameter(const AZ::TypeId& typeId, const AZStd::string& name);

    protected:       
        AZStd::unique_ptr<Actor> m_actor;
        ActorInstance* m_actorInstance = nullptr;
        AZStd::unique_ptr<EmptyAnimGraph> m_animGraph = nullptr;
        AnimGraphInstance* m_animGraphInstance = nullptr;
        MotionSet* m_motionSet = nullptr;        
    };
}
