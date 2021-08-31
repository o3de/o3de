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
#include <AzToolsFramework/Manipulators/TranslationManipulators.h>
#include <Editor/Source/ComponentModes/PhysXSubComponentModeBase.h>

namespace AZ
{
    class Vector3;
} // namespace AZ

namespace PhysX
{
    class JointsSubComponentModeTranslation final
        : public PhysXSubComponentModeBase
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL;

        JointsSubComponentModeTranslation();

        // PhysXSubComponentModeBase ...
        void Setup(const AZ::EntityComponentIdPair& idPair) override;
        void Refresh(const AZ::EntityComponentIdPair& idPair) override;
        void Teardown(const AZ::EntityComponentIdPair& idPair) override;
        void ResetValues(const AZ::EntityComponentIdPair& idPair) override;

    private:
        void OnManipulatorMoved(const AZ::Vector3& position, const AZ::EntityComponentIdPair& idPair);

        AZ::Vector3 m_resetValue = AZ::Vector3::CreateZero();
        AzToolsFramework::TranslationManipulators m_manipulator;
    };
}
