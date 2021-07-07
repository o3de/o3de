/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/optional.h>
#include <EMotionFX/Source/SimulatedObjectSetup.h>
#include <QModelIndexList>


namespace MCore { class CommandGroup; }

namespace EMotionFX
{
    class SimulatedObjectHelpers
    {
    public:
        static bool AddSimulatedObject(AZ::u32 actorID, AZStd::optional<AZStd::string> name = AZStd::nullopt, MCore::CommandGroup* commandGroup = nullptr);
        static bool AddSimulatedJoints(const QModelIndexList& modelIndices /* SkeletonModel */, size_t objectIndex, bool addChildren, MCore::CommandGroup* commandGroup = nullptr);
        static void RemoveSimulatedObject(const QModelIndex& modelIndex /* SimulatedObjectModel */);
        static void RemoveSimulatedJoint(const QModelIndex& modelIndex /* SimulatedObjectModel */, bool removeChildren);
        static void RemoveSimulatedJoints(const QModelIndexList& modelIndices /* SimulatedObjectModel */, bool removeChildren);
    };
} // namespace EMotionFX
