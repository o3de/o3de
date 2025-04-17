/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <Atom/RPI.Public/Buffer/Buffer.h>
#include <PostProcessing/DepthOfFieldWriteFocusDepthFromGpuPass.h>
#include <PostProcessing/DepthOfFieldCopyFocusDepthToCpuPass.h>

namespace AZ
{
    namespace Render
    {
        class DepthOfFieldSettings;

        //! This pass is used to get the depth value for the specified screen coordinates.
        class DepthOfFieldReadBackFocusDepthPass final
            : public RPI::ParentPass
        {
        public:
            AZ_RTTI(AZ::Render::DepthOfFieldReadBackFocusDepthPass, "{8738691C-1D8C-4F96-8B4F-2152A4550470}", AZ::RPI::ParentPass);
            AZ_CLASS_ALLOCATOR(DepthOfFieldReadBackFocusDepthPass, SystemAllocator);

            static RPI::Ptr<DepthOfFieldReadBackFocusDepthPass> Create(const RPI::PassDescriptor& descriptor);

            DepthOfFieldReadBackFocusDepthPass(const RPI::PassDescriptor& descriptor);
            ~DepthOfFieldReadBackFocusDepthPass();

            // Set pass parameter interfaces...
            void SetScreenPosition(const AZ::Vector2& screenPosition);
            float GetFocusDepth();
            float GetNormalizedFocusDistanceForAutoFocus() const;

        protected:
            // Pass behavior overrides...
            void CreateChildPassesInternal() override;
            void FrameBeginInternal(FramePrepareParams params) override;

        private:
            void UpdateAutoFocus(DepthOfFieldSettings& dofSettings);

            // Used to avoid buffer name conflicts when creating multiple DepthOfFieldReadBackFocusDepthPass
            static uint32_t s_bufferInstance;
            AZ::Data::Instance<RPI::Buffer> m_buffer;

            RPI::Ptr<DepthOfFieldWriteFocusDepthFromGpuPass> m_getDepthPass;
            RPI::Ptr<DepthOfFieldCopyFocusDepthToCpuPass> m_readbackPass;

            float m_normalizedFocusDistanceForAutoFocus = 0.0f;
            float m_delayTimer = 0.0f;
            bool m_isMovingFocus = false;
        };
    }   // namespace Render
}   // namespace AZ
