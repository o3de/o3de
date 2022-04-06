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
#include <AzToolsFramework/ComponentModes/CapsuleViewportEdit.h>

namespace PhysX
{
    /// Sub component mode for modifying the height and radius on a capsule collider.
    class ColliderCapsuleMode
        : public PhysXSubComponentModeBase
        , private AzFramework::EntityDebugDisplayEventBus::Handler
        , private AzToolsFramework::CapsuleViewportEdit
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

        // CapsuleViewportEdit ...
        AZ::Transform GetCapsuleWorldTransform() const override;
        AZ::Transform GetCapsuleLocalTransform() const override;
        AZ::Vector3 GetCapsuleNonUniformScale() const override;
        float GetCapsuleRadius() const override;
        float GetCapsuleHeight() const override;
        void SetCapsuleRadius(float radius) override;
        void SetCapsuleHeight(float height) override;

        AZ::EntityComponentIdPair m_entityComponentIdPair;
    };
} //namespace PhysX
