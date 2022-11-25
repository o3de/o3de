/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>

#include <SceneAPI/SceneCore/SceneCoreStandaloneAllocator.h>

namespace AZ
{
    namespace SceneAPI
    {
        bool SceneCoreStandaloneAllocator::m_allocatorInitialized = false;

        void SceneCoreStandaloneAllocator::Initialize()
        {
            if (!AZ::AllocatorInstance<AZ::SystemAllocator>::IsReady())
            {
                AZ::AllocatorInstance<AZ::SystemAllocator>::Create();
                m_allocatorInitialized = true;
            }
        }

        void SceneCoreStandaloneAllocator::TearDown()
        {
            if (m_allocatorInitialized)
            {
                AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
            }
        }
    }
}
