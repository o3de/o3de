/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <Atom/RPI.Public/Configuration.h>
#include <Atom/RPI.Public/Pass/RenderPass.h>

namespace AZ
{
    namespace RPI
    {
        //! Only use this for debug purposes and edge cases
        //! The correct and efficient way to clear a pass is through the LoadStoreAction on the pass slot
        //! This will clear a given image attachment to the specified clear value.
        class ATOM_RPI_PUBLIC_API SlowClearPass
            : public RenderPass
        {
            AZ_RPI_PASS(SlowClearPass);

        public:
            AZ_RTTI(SlowClearPass, "{31CBAD6C-108F-4F3F-B498-ED968DFCFCE2}", RenderPass);
            AZ_CLASS_ALLOCATOR(SlowClearPass, SystemAllocator);
            virtual ~SlowClearPass() = default;

            //! Creates a SlowClearPass
            static Ptr<SlowClearPass> Create(const PassDescriptor& descriptor);

            void SetClearValue(float red, float green, float blue, float alpha);
        protected:
            SlowClearPass(const PassDescriptor& descriptor);
            void InitializeInternal() override;

        private:
            RHI::ClearValue m_clearValue;
        };

    }   // namespace RPI
}   // namespace AZ
