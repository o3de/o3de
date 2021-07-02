/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PhysX_precompiled.h>

#include "ColliderBoxMode.h"
#include <AzToolsFramework/Manipulators/BoxManipulatorRequestBus.h>

namespace PhysX
{
    AZ_CLASS_ALLOCATOR_IMPL(ColliderBoxMode, AZ::SystemAllocator, 0);

    void ColliderBoxMode::Setup(const AZ::EntityComponentIdPair& idPair)
    {
        m_boxEdit.Setup(idPair);
    }

    void ColliderBoxMode::Refresh([[maybe_unused]] const AZ::EntityComponentIdPair& idPair)
    {
        m_boxEdit.UpdateManipulators();
    }

    void ColliderBoxMode::Teardown([[maybe_unused]] const AZ::EntityComponentIdPair& idPair)
    {
        m_boxEdit.Teardown();
    }

    void ColliderBoxMode::ResetValues(const AZ::EntityComponentIdPair& idPair)
    {
        AzToolsFramework::BoxManipulatorRequestBus::Event(
            idPair, &AzToolsFramework::BoxManipulatorRequests::SetDimensions,
            AZ::Vector3::CreateOne());
    }
}
