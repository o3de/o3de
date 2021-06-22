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

#include <Atom/RPI.Public/Pass/Pass.h>
#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>

namespace AZ
{
    namespace RPI
    {
        //! This pass allows the use of a custom fullscreen MSAA resolve pass shader.
        class MSAAResolveFullScreenPass
            : public RPI::FullscreenTrianglePass
        {
            AZ_RPI_PASS(MSAAResolveFullScreenPass);

        public:
            AZ_RTTI(RPI::MSAAResolveFullScreenPass, "{998FCF6D-5441-4C59-89EC-795CFB803912}", FullscreenTrianglePass);
            AZ_CLASS_ALLOCATOR(RPI::MSAAResolveFullScreenPass, SystemAllocator, 0);

            //! Creates a new pass without a PassTemplate
            static RPI::Ptr<MSAAResolveFullScreenPass> Create(const RPI::PassDescriptor& descriptor);

        private:
            explicit MSAAResolveFullScreenPass(const RPI::PassDescriptor& descriptor);

            // Pass Overrides...
            bool IsEnabled() const override;
        };
    }   // namespace RPI
}   // namespace AZ
