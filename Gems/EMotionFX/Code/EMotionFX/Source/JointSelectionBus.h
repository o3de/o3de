/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/containers/unordered_set.h>

namespace EMotionFX
{
    class JointSelectionRequests
        : public AZ::EBusTraits
    {
    public:
        virtual const AZStd::unordered_set<size_t>* FindSelectedJointIndices(EMotionFX::ActorInstance* instance) const = 0;
    };
    using JointSelectionRequestBus = AZ::EBus<JointSelectionRequests>;
}
