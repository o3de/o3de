/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Editor/Source/ComponentModes/PhysXSubComponentModeBase.h>
#include <AzToolsFramework/Manipulators/TranslationManipulators.h>

namespace PhysX
{
    /// Sub component mode for modifying offset on a collider in the viewport.
    class ColliderOffsetMode : public PhysXSubComponentModeBase
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        ColliderOffsetMode();

        // PhysXSubComponentModeBase ...
        void Setup(const AZ::EntityComponentIdPair& idPair) override;
        void Refresh(const AZ::EntityComponentIdPair& idPair) override;
        void Teardown(const AZ::EntityComponentIdPair& idPair) override;
        void ResetValues(const AZ::EntityComponentIdPair& idPair) override;

    private:
        void OnManipulatorMoved(
            const AZ::Vector3& startPosition, const AZ::Vector3& offset, const AZ::EntityComponentIdPair& idPair);

        AzToolsFramework::TranslationManipulators m_translationManipulators;
    };
} //namespace PhysX
