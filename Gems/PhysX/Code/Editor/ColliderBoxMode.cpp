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
