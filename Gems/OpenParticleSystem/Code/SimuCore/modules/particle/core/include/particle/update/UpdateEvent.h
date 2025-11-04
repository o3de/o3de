/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "particle/core/Particle.h"

namespace SimuCore::ParticleCore {
    struct UpdateLocationEvent {
        using DataType = UpdateLocationEvent;
        static void Execute(const UpdateLocationEvent* data, const EventInfo& info);

        bool whetherSendEvent = true;
    };

    struct UpdateDeathEvent {
        using DataType = UpdateDeathEvent;
        static void Execute(const UpdateDeathEvent* data, const EventInfo& info);

        bool whetherSendEvent = true;
    };

    struct UpdateCollisionEvent {
        using DataType = UpdateCollisionEvent;
        static void Execute(const UpdateCollisionEvent* data, const EventInfo& info);

        bool whetherSendEvent = true;
    };

    struct UpdateInheritanceEvent {
        using DataType = UpdateInheritanceEvent;
        static void Execute(const UpdateInheritanceEvent* data, const EventInfo& info);

        bool whetherSendEvent = true;
    };
}

