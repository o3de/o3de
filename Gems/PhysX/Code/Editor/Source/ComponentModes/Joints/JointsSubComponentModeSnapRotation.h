/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector3.h>
#include <AzCore/Memory/Memory.h>
#include <Editor/Source/ComponentModes/Joints/JointsSubComponentModeSnap.h>

namespace PhysX
{
    class JointsSubComponentModeSnapRotation final
        : public JointsSubComponentModeSnap
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL;

        JointsSubComponentModeSnapRotation() = default;

        // JointsSubComponentModeSnap ...
        void Setup(const AZ::EntityComponentIdPair& idPair) override;
        void ResetValues(const AZ::EntityComponentIdPair& idPair) override;

    protected:
        // JointsSubComponentModeSnap ...
        void DisplaySpecificSnapType(
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay,
            const AZ::Vector3& jointPosition,
            const AZ::Vector3& snapDirection,
            float snapLength) override;

    private:
        AZ::Vector3 m_resetRotation;
    };
}
