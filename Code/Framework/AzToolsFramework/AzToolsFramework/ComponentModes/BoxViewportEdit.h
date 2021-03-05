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

        void Setup(const AZ::EntityComponentIdPair& entityComponentIdPair);
        void Teardown();
        void UpdateManipulators();

    private:
        AZ::EntityComponentIdPair m_entityComponentIdPair;
        using BoxManipulators = AZStd::array<AZStd::shared_ptr<LinearManipulator>, 6>;
        BoxManipulators m_linearManipulators; ///< Manipulators for editing box size.
    };
} // namespace AzToolsFramework
