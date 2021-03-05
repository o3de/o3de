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
