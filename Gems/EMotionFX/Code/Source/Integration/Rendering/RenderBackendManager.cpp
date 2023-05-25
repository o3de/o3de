/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/Actor.h>
#include <Integration/Rendering/RenderBackendManager.h>
#include <Integration/System/SystemCommon.h>

namespace EMotionFX
{
    namespace Integration
    {
        AZ_CLASS_ALLOCATOR_IMPL(RenderBackendManager, EMotionFXAllocator);

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
