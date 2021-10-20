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
        //! 
        class NewDepthOfFieldParentPass final
            : public RPI::ParentPass
        {
            AZ_RPI_PASS(NewDepthOfFieldParentPass);

        public:
            AZ_RTTI(AZ::Render::NewDepthOfFieldParentPass, "{71F4998B-447C-4BAC-A5BE-2D2850FABB57}", AZ::RPI::ParentPass);
            AZ_CLASS_ALLOCATOR(NewDepthOfFieldParentPass, SystemAllocator, 0);
            virtual ~NewDepthOfFieldParentPass() = default;

            static RPI::Ptr<NewDepthOfFieldParentPass> Create(const RPI::PassDescriptor& descriptor);

            bool IsEnabled() const override;

        protected:
            // Behavior functions override...
            void FrameBeginInternal(FramePrepareParams params) override;

        private:
            NewDepthOfFieldParentPass(const RPI::PassDescriptor& descriptor);
        };



        //! 
        class NewDepthOfFieldTileReducePass final
            : public RPI::ComputePass
        {
            AZ_RPI_PASS(NewDepthOfFieldTileReducePass);

        public:
            AZ_RTTI(AZ::Render::NewDepthOfFieldTileReducePass, "{2E072695-0847-43A6-9BE4-D6D85CFFBA41}", AZ::RPI::ComputePass);
            AZ_CLASS_ALLOCATOR(NewDepthOfFieldTileReducePass, SystemAllocator, 0);
            virtual ~NewDepthOfFieldTileReducePass() = default;

            static RPI::Ptr<NewDepthOfFieldTileReducePass> Create(const RPI::PassDescriptor& descriptor);

        protected:
            // Behavior functions override...
            void FrameBeginInternal(FramePrepareParams params) override;

        private:
            NewDepthOfFieldTileReducePass(const RPI::PassDescriptor& descriptor);
        };



        //! 
        class NewDepthOfFieldFilterPass final
            : public RPI::FullscreenTrianglePass
        {
            AZ_RPI_PASS(NewDepthOfFieldFilterPass);

        public:
            AZ_RTTI(AZ::Render::NewDepthOfFieldFilterPass, "{F8A98E53-1A50-4178-A6EB-2BD0148C038B}", AZ::RPI::FullscreenTrianglePass);
            AZ_CLASS_ALLOCATOR(NewDepthOfFieldFilterPass, SystemAllocator, 0);
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



        //! 
        class NewDepthOfFieldCompositePass final
            : public RPI::ComputePass
        {
            AZ_RPI_PASS(NewDepthOfFieldCompositePass);

        public:
            AZ_RTTI(AZ::Render::NewDepthOfFieldCompositePass, "{63270A3A-EAE5-4C0C-98AA-43CA55279613}", AZ::RPI::ComputePass);
            AZ_CLASS_ALLOCATOR(NewDepthOfFieldCompositePass, SystemAllocator, 0);
            virtual ~NewDepthOfFieldCompositePass() = default;

            static RPI::Ptr<NewDepthOfFieldCompositePass> Create(const RPI::PassDescriptor& descriptor);

        protected:
            // Behavior functions override...
            void FrameBeginInternal(FramePrepareParams params) override;

        private:
            NewDepthOfFieldCompositePass(const RPI::PassDescriptor& descriptor);
        };


    }   // namespace Render
}   // namespace AZ
