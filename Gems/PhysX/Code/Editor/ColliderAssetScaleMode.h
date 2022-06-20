/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Editor/Source/ComponentModes/PhysXSubComponentModeBase.h>
#include <AzToolsFramework/Manipulators/ScaleManipulators.h>

namespace PhysX
{
    /// Sub component mode for modifying the asset scale on a collider in the viewport.
    class ColliderAssetScaleMode : public PhysXSubComponentModeBase
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        ColliderAssetScaleMode();

        // PhysXSubComponentModeBase ...
        void Setup(const AZ::EntityComponentIdPair& idPair) override;
        void Refresh(const AZ::EntityComponentIdPair& idPair) override;
        void Teardown(const AZ::EntityComponentIdPair& idPair) override;
        void ResetValues(const AZ::EntityComponentIdPair& idPair) override;

    private:

        void OnManipulatorDown(const AZ::EntityComponentIdPair& idPair);
        void OnManipulatorMoved(const AZ::Vector3& scale, const AZ::EntityComponentIdPair& idPair);

        AZ::Vector3 m_initialScale;
        AzToolsFramework::ScaleManipulators m_dimensionsManipulators;
    };
} //namespace PhysX
