/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Configuration.h>
#include <Atom/RPI.Public/Pass/Pass.h>
#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>

namespace AZ
{
    namespace RPI
    {
        //! This pass allows the use of a custom fullscreen MSAA resolve pass shader.
        class ATOM_RPI_PUBLIC_API MSAAResolveFullScreenPass
            : public RPI::FullscreenTrianglePass
        {
            AZ_RPI_PASS(MSAAResolveFullScreenPass);

        public:
            AZ_RTTI(RPI::MSAAResolveFullScreenPass, "{998FCF6D-5441-4C59-89EC-795CFB803912}", FullscreenTrianglePass);
            AZ_CLASS_ALLOCATOR(RPI::MSAAResolveFullScreenPass, SystemAllocator);

            //! Creates a new pass without a PassTemplate
            static RPI::Ptr<MSAAResolveFullScreenPass> Create(const RPI::PassDescriptor& descriptor);

        private:
            explicit MSAAResolveFullScreenPass(const RPI::PassDescriptor& descriptor);

            // Pass Overrides...
            bool IsEnabled() const override;
        };
    }   // namespace RPI
}   // namespace AZ
