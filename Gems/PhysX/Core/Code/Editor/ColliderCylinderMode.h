/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/ComponentModes/CapsuleViewportEdit.h>
#include <Editor/Source/ComponentModes/PhysXSubComponentModeBase.h>

namespace PhysX
{
    //! Sub component mode for modifying the height and radius on a cylinder collider.
    class ColliderCylinderMode
        : public PhysXSubComponentModeBase
        , private AzFramework::EntityDebugDisplayEventBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        // PhysXSubComponentModeBase overrides ...
        void Setup(const AZ::EntityComponentIdPair& idPair) override;
        void Refresh(const AZ::EntityComponentIdPair& idPair) override;
        void Teardown(const AZ::EntityComponentIdPair& idPair) override;
        void ResetValues(const AZ::EntityComponentIdPair& idPair) override;

    private:
        // AzFramework::EntityDebugDisplayEventBus overrides ...
        void DisplayEntityViewport(
            const AzFramework::ViewportInfo& viewportInfo,
            AzFramework::DebugDisplayRequests& debugDisplay) override;

        AZ::EntityComponentIdPair m_entityComponentIdPair;

        // cylinder is equivalent to capsule for the purposes of viewport editing
        AZStd::unique_ptr<AzToolsFramework::CapsuleViewportEdit> m_capsuleViewportEdit;
    };
} //namespace PhysX
