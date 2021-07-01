/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>

#include <SceneAPI/SceneData/SceneDataStandaloneAllocator.h>

namespace AZ
{
    namespace SceneAPI
    {
        bool SceneDataStandaloneAllocator::m_allocatorInitialized = false;

        void SceneDataStandaloneAllocator::Initialize(AZ::EnvironmentInstance environment)
        {
            AZ::Environment::Attach(environment);
            if (!AZ::AllocatorInstance<AZ::SystemAllocator>().IsReady())
            {
                AZ::AllocatorInstance<AZ::SystemAllocator>().Create();
                m_allocatorInitialized = true;
            }
        }

        void SceneDataStandaloneAllocator::TearDown()
        {
            if (m_allocatorInitialized)
            {
                AZ::AllocatorInstance<AZ::SystemAllocator>().Destroy();
            }
            AZ::Environment::Detach();
        }
    }
}
