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
