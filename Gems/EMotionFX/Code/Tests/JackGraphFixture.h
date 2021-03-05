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
