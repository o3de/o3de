/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ColliderBoxMode.h"
#include <AzToolsFramework/Manipulators/BoxManipulatorRequestBus.h>
#include <AzToolsFramework/ComponentModes/BoxComponentMode.h>

namespace PhysX
{
    AZ_CLASS_ALLOCATOR_IMPL(ColliderBoxMode, AZ::SystemAllocator);

    ColliderBoxMode::ColliderBoxMode()
    {
        const bool allowAsymmetricalEditing = true;
        m_boxEdit = AZStd::make_unique<AzToolsFramework::BoxViewportEdit>(allowAsymmetricalEditing);
    }

    void ColliderBoxMode::Setup(const AZ::EntityComponentIdPair& idPair)
    {
        AzToolsFramework::InstallBaseShapeViewportEditFunctions(m_boxEdit.get(), idPair);
        AzToolsFramework::InstallBoxViewportEditFunctions(m_boxEdit.get(), idPair);
        m_boxEdit->Setup(AzToolsFramework::g_mainManipulatorManagerId);
        m_boxEdit->AddEntityComponentIdPair(idPair);
    }

    void ColliderBoxMode::Refresh([[maybe_unused]] const AZ::EntityComponentIdPair& idPair)
    {
        m_boxEdit->UpdateManipulators();
    }

    void ColliderBoxMode::Teardown([[maybe_unused]] const AZ::EntityComponentIdPair& idPair)
    {
        m_boxEdit->Teardown();
    }

    void ColliderBoxMode::ResetValues([[maybe_unused]] const AZ::EntityComponentIdPair& idPair)
    {
        m_boxEdit->ResetValues();
    }
}
