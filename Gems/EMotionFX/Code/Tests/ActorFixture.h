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
#include <EMotionFX/Source/AutoRegisteredActor.h>


namespace EMotionFX
{
    class Actor;
    class ActorInstance;
    class SimulatedObjectSetup;

    class ActorFixture
        : public SystemComponentFixture
    {
    public:
        void SetUp() override;
        void TearDown() override;

        AZStd::string SerializePhysicsSetup(const Actor* actor) const;
        AZStd::string SerializeSimulatedObjectSetup(const Actor* actor) const;
        SimulatedObjectSetup* DeserializeSimulatedObjectSetup(const AZStd::string& data) const;
        AZStd::vector<AZStd::string> GetTestJointNames() const;

    protected:
        AutoRegisteredActor m_actor{};
        ActorInstance* m_actorInstance = nullptr;
    };
} // namespace EMotionFX
