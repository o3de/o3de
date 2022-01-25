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
    /// Sub component mode for modifying the height and radius on a capsule collider.
    class ColliderCapsuleMode
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

        void SetupRadiusManipulator(const AZ::EntityComponentIdPair& idPair, const AZ::Transform& worldTransform);
        void SetupHeightManipulator(const AZ::EntityComponentIdPair& idPair, const AZ::Transform& worldTransform);
        void OnRadiusManipulatorMoved(const AzToolsFramework::LinearManipulator::Action& action, const AZ::EntityComponentIdPair& idPair);
        void OnHeightManipulatorMoved(const AzToolsFramework::LinearManipulator::Action& action, const AZ::EntityComponentIdPair& idPair);
        void AdjustRadiusManipulator(const AZ::EntityComponentIdPair& idPair, const float capsuleHeight);
        void AdjustHeightManipulator(const AZ::EntityComponentIdPair& idPair, const float capsuleRadius);

        AZStd::shared_ptr<AzToolsFramework::LinearManipulator> m_radiusManipulator;
        AZStd::shared_ptr<AzToolsFramework::LinearManipulator> m_heightManipulator;
    };
} //namespace PhysX
