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
