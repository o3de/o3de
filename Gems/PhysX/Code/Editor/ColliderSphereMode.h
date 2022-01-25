/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Editor/Source/ComponentModes/PhysXSubComponentModeBase.h>
#include <AzToolsFramework/ComponentModes/BoxViewportEdit.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>

namespace PhysX
{
    /// Sub component mode for modifying the box dimensions on a collider.
    class ColliderSphereMode
        : public PhysXSubComponentModeBase
        , private AzFramework::EntityDebugDisplayEventBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        // PhysXSubComponentModeBase ...
        void Setup(const AZ::EntityComponentIdPair& idPair) override;
        void Refresh(const AZ::EntityComponentIdPair& idPair) override;
        void Teardown(const AZ::EntityComponentIdPair& idPair) override;
        void ResetValues(const AZ::EntityComponentIdPair& idPair) override;

    private:
        // AzFramework::EntityDebugDisplayEventBus ...
        void DisplayEntityViewport(
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay) override;

        void OnManipulatorMoved(const AzToolsFramework::LinearManipulator::Action& action, const AZ::EntityComponentIdPair& idPair);

        AZ::Vector3 m_colliderOffset;
        AZStd::shared_ptr<AzToolsFramework::LinearManipulator> m_radiusManipulator;
    };
} //namespace PhysX
