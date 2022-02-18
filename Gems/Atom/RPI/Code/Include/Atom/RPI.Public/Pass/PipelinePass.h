/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Pass/Pass.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>

namespace AZ
{
    namespace RPI
    {
        struct PipelinePassData;

        //! A Pipeline Pass is a specialization of the Parent Pass designed to be the root pass of a pipeline
        class PipelinePass
            : public ParentPass
        {
            AZ_RPI_PASS(PipelinePass);

        public:

            AZ_RTTI(PipelinePass, "{4F0258DA-44DE-42BD-8C8F-9916AA0B6906}", ParentPass);
            AZ_CLASS_ALLOCATOR(PipelinePass, SystemAllocator, 0);

            virtual ~PipelinePass() = default;

            static Ptr<PipelinePass> Create(const PassDescriptor& descriptor);

            //! Creates a new PipelinePass pass reusing the same parameters used to create this pass
            //! This is used in scenarios like hot reloading where some of the templates in the pass library might have changed.
            Ptr<ParentPass> Recreate() const override;

        protected:
            explicit PipelinePass(const PassDescriptor& descriptor);

            // --- Pass Behaviour Overrides ---
            void BuildInternal() override;

            // Virtual function derived passes can use to add attachments and connections
            // to the pipeline for global reference before any child passes are built. 
            virtual void AddPipelineAttachmentsAndConnectionsInternal() { }

        private:
            // Creates attachments specified in PipelinePassData for the pipeline for global reference
            void CreatePipelineAttachmentsFromPassData(const PipelinePassData& passData);

            // Builds child passes while checking for pipeline connections with each pass
            void BuildChildPassesWithPipelineConnections(const PipelinePassData& passData);

            // Builds child passes without checking for any pipeline connections
            void BuildChildPasses();
        };

    }   // namespace RPI
}   // namespace AZ
