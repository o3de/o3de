/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Pass/ComputePass.h>
#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>

#include <PostProcessing/PostProcessingShaderOptionBase.h>

namespace AZ
{
    namespace Render
    {
        // Technique
        // 
        // 1. This Depth of Field technique starts by downsampling the lighting buffer and calculating
        //    the circle of confusion (CoC) for each downsampled pixel.
        // 
        // 2. It then computes the min and max CoC for tiles of 16x16 pixels
        // 
        // 3. It expands the min and max in a 3x3 region (twice, so 5x5 at the end) so that each tile
        //    tile pixel has the min and max CoCs of the 5x5 tile region around it
        // 
        // 4. We perform a 48 tap scatter-as-gather blur around each pixel
        // 
        // 5. We perform a follow up 8 tap scatter-as-gather blur to fill the holes from the first blur
        // 
        // 6. We composite the blurred half resolution image onto the full resolution lighting buffer
        // 
        // See http://advances.realtimerendering.com/s2013/Sousa_Graphics_Gems_CryENGINE3.pptx
        // for a more detailed explanation.
        // 
        // Notes: The name NewDepthOfField is in contrast to the previously implemented depth of field method
        // That method will be removed in a follow up change and at that point NewDepthOfField will be renamed
        // to simple DepthOfField.

        //! Parent pass for the new depth of field technique
        //! Main updates the view srg via the depth of field settings
        //! And enables/disables all depth of field passes based on component activation
        class NewDepthOfFieldParentPass final
            : public RPI::ParentPass
        {
            AZ_RPI_PASS(NewDepthOfFieldParentPass);

        public:
            AZ_RTTI(AZ::Render::NewDepthOfFieldParentPass, "{71F4998B-447C-4BAC-A5BE-2D2850FABB57}", AZ::RPI::ParentPass);
            AZ_CLASS_ALLOCATOR(NewDepthOfFieldParentPass, SystemAllocator);
            virtual ~NewDepthOfFieldParentPass() = default;

            static RPI::Ptr<NewDepthOfFieldParentPass> Create(const RPI::PassDescriptor& descriptor);

            bool IsEnabled() const override;

        protected:
            // Behavior functions override...
            void FrameBeginInternal(FramePrepareParams params) override;

        private:
            NewDepthOfFieldParentPass(const RPI::PassDescriptor& descriptor);
        };


        //! Need a class for the tile reduce pass because it dispatches a non-trivial number of threads
        class NewDepthOfFieldTileReducePass final
            : public RPI::ComputePass
        {
            AZ_RPI_PASS(NewDepthOfFieldTileReducePass);

        public:
            AZ_RTTI(AZ::Render::NewDepthOfFieldTileReducePass, "{2E072695-0847-43A6-9BE4-D6D85CFFBA41}", AZ::RPI::ComputePass);
            AZ_CLASS_ALLOCATOR(NewDepthOfFieldTileReducePass, SystemAllocator);
            virtual ~NewDepthOfFieldTileReducePass() = default;

            static RPI::Ptr<NewDepthOfFieldTileReducePass> Create(const RPI::PassDescriptor& descriptor);

        protected:
            // Behavior functions override...
            void FrameBeginInternal(FramePrepareParams params) override;

        private:
            NewDepthOfFieldTileReducePass(const RPI::PassDescriptor& descriptor);
        };


        //! Filter pass used to render the bokeh blur effect on downsampled image buffer
        //! This class is used for both the large filter and the small filter
        //! It's main purpose is calculating the sample positions and setting srg constants
        class NewDepthOfFieldFilterPass final
            : public RPI::FullscreenTrianglePass
        {
            AZ_RPI_PASS(NewDepthOfFieldFilterPass);

        public:
            AZ_RTTI(AZ::Render::NewDepthOfFieldFilterPass, "{F8A98E53-1A50-4178-A6EB-2BD0148C038B}", AZ::RPI::FullscreenTrianglePass);
            AZ_CLASS_ALLOCATOR(NewDepthOfFieldFilterPass, SystemAllocator);
            virtual ~NewDepthOfFieldFilterPass() = default;

            static RPI::Ptr<NewDepthOfFieldFilterPass> Create(const RPI::PassDescriptor& descriptor);

        protected:
            // Behavior functions override...
            void FrameBeginInternal(FramePrepareParams params) override;

        private:
            NewDepthOfFieldFilterPass(const RPI::PassDescriptor& descriptor);

            // SRG binding indices...
            AZ::RHI::ShaderInputNameIndex m_constantsIndex = "m_dofConstants";
        };


    }   // namespace Render
}   // namespace AZ
