/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>

namespace AZ
{
    namespace Render
    {
        //! A class for DiffuseComposite to allow for disabling
        class DiffuseCompositePass final
            : public RPI::FullscreenTrianglePass
        {
            AZ_RPI_PASS(DiffuseCompositePass);

        public:
            AZ_RTTI(DiffuseCompositePass, "{F3DBEBCB-66F8-465C-A06B-DFA76B9D4856}", AZ::RPI::FullscreenTrianglePass);
            AZ_CLASS_ALLOCATOR(DiffuseCompositePass, SystemAllocator, 0);

            ~DiffuseCompositePass() = default;
            static RPI::Ptr<DiffuseCompositePass> Create(const RPI::PassDescriptor& descriptor);

            bool IsEnabled() const override;

        private:
            DiffuseCompositePass(const RPI::PassDescriptor& descriptor);

        };
    } // namespace Render
} // namespace AZ