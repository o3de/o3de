/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Interface/Interface.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/RTTI/RTTI.h>

#include <Integration/Rendering/RenderBackend.h>


namespace EMotionFX
{
    namespace Integration
    {
        class RenderBackendManager
        {
        public:
            AZ_RTTI(EMotionFX::Integration::RenderBackendManager, "{D4C67563-0BFC-49CA-A3FC-40363F5BFC79}")
            AZ_CLASS_ALLOCATOR_DECL

            RenderBackendManager();
            virtual ~RenderBackendManager();

            void SetRenderBackend(RenderBackend* backend);
            RenderBackend* GetRenderBackend() const;

        private:
            AZStd::unique_ptr<RenderBackend> m_renderBackend;
        };
    } // namespace Integration
} // namespace EMotionFX
