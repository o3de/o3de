/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <Atom/RPI.Public/Pass/RenderPass.h>

namespace AZ
{
    namespace RPI
    {
        //! A simple pass to clear a render target
        class ClearPass
            : public RenderPass
        {
            AZ_RPI_PASS(ClearPass);

        public:
            AZ_RTTI(ClearPass, "{31CBAD6C-108F-4F3F-B498-ED968DFCFCE2}", RenderPass);
            AZ_CLASS_ALLOCATOR(ClearPass, SystemAllocator, 0);
            virtual ~ClearPass() = default;

            //! Creates a ClearPass
            static Ptr<ClearPass> Create(const PassDescriptor& descriptor);

        protected:
            ClearPass(const PassDescriptor& descriptor);
            void InitializeInternal() override;

        private:
            RHI::ClearValue m_clearValue;
        };

    }   // namespace RPI
}   // namespace AZ
