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

#include <EMotionFX/Source/Actor.h>
#include <Integration/Rendering/RenderBackendManager.h>
#include <Integration/System/SystemCommon.h>

namespace EMotionFX
{
    namespace Integration
    {
        AZ_CLASS_ALLOCATOR_IMPL(RenderBackendManager, EMotionFXAllocator, 0);

        RenderBackendManager::RenderBackendManager()
        {
            AZ::Interface<RenderBackendManager>::Register(this);
        }

        RenderBackendManager::~RenderBackendManager()
        {
            AZ::Interface<RenderBackendManager>::Unregister(this);
        }

        void RenderBackendManager::SetRenderBackend(RenderBackend* backend)
        {
            m_renderBackend.reset(backend);
        }

        RenderBackend* RenderBackendManager::GetRenderBackend() const
        {
            return m_renderBackend.get();
        }
    } // namespace Integration
} // namespace EMotionFX
