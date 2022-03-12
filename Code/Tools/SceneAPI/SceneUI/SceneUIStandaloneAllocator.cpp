/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>

#include <SceneAPI/SceneUI/SceneUIStandaloneAllocator.h>

namespace AZ
{
    namespace SceneAPI
    {
        bool SceneUIStandaloneAllocator::m_allocatorInitialized = false;

        void SceneUIStandaloneAllocator::Initialize()
        {
            if (!AZ::AllocatorInstance<AZ::SystemAllocator>().IsReady())
            {
                AZ::AllocatorInstance<AZ::SystemAllocator>().Create();
                m_allocatorInitialized = true;
            }
        }

        void SceneUIStandaloneAllocator::TearDown()
        {
            if (m_allocatorInitialized)
            {
                AZ::AllocatorInstance<AZ::SystemAllocator>().Destroy();
            }
        }
    }
}
