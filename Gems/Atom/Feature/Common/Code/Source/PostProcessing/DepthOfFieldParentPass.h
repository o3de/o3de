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

#include <Atom/RPI.Public/Pass/ParentPass.h>

namespace AZ
{
    namespace Render
    {
        //! Parent pass for all Depth of Field related passes.
        //! The only difference between this and ParentPass is that this will check for available depth of field settings
        //! and disable itself if no settings are found. See the IsEnabled override.
        class DepthOfFieldParentPass final
            : public RPI::ParentPass
        {
            AZ_RPI_PASS(DepthOfFieldParentPass);

        public:
            AZ_RTTI(AZ::Render::DepthOfFieldParentPass, "{6033066A-CA95-422E-9BF2-8C203171C1A8}", AZ::RPI::ParentPass);
            AZ_CLASS_ALLOCATOR(DepthOfFieldParentPass, SystemAllocator, 0);
            virtual ~DepthOfFieldParentPass() = default;

            //! Creates a DepthOfFieldParentPass
            static RPI::Ptr<DepthOfFieldParentPass> Create(const RPI::PassDescriptor& descriptor);

            bool IsEnabled() const override;

        private:
            DepthOfFieldParentPass(const RPI::PassDescriptor& descriptor);
        };
    }   // namespace Render
}   // namespace AZ
