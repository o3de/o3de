/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzToolsFramework/Manipulators/LinearManipulator.h>

namespace AzToolsFramework
{
    class LinearManipulator;

    /// Wraps 6 linear manipulators, providing a viewport experience for 
    /// modifying the extents of a box
    class BoxViewportEdit
    {
    public:
        BoxViewportEdit() = default;

        void Setup(const AZ::EntityComponentIdPair& entityComponentIdPair, bool allowAsymmetricalEditing = false);
        void Teardown();
        void UpdateManipulators();

    private:
        AZ::EntityComponentIdPair m_entityComponentIdPair;
        using BoxManipulators = AZStd::array<AZStd::shared_ptr<LinearManipulator>, 6>;
        BoxManipulators m_linearManipulators; ///< Manipulators for editing box size.
        bool m_allowAsymmetricalEditing = false; ///< Whether moving individual faces independently is allowed.
    };

    /// Calculates the position of the manipulator in its own reference frame.
    /// Removes the effects of the manipulator local transform, and accounts for world transform scale in
    /// the action local offset.
    AZ::Vector3 GetPositionInManipulatorFrame(float worldUniformScale, const AZ::Transform& manipulatorLocalTransform,
        const LinearManipulator::Action& action);
} // namespace AzToolsFramework
