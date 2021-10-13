/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "SystemComponentFixture.h"
#include <Integration/Assets/ActorAsset.h>

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
        Actor* GetActor() const;

        AZ::Data::Asset<Integration::ActorAsset> m_actorAsset;
        ActorInstance* m_actorInstance = nullptr;
    };
} // namespace EMotionFX
