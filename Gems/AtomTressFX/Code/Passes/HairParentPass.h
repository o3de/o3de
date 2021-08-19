/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <AtomCore/std/containers/array_view.h>
#include <AzCore/std/containers/fixed_vector.h>

namespace AZ
{
    namespace Render
    {
        //! HairParentPass owns and manages ShadowmapPasses.
        class HairParentPass final
            : public RPI::ParentPass
        {
            using Base = RPI::ParentPass;
            AZ_RPI_PASS(HairParentPass);

        public:
            AZ_RTTI(HairParentPass, "80C7E869-2513-4201-8C1E-D2E39DDE1244", Base);
            AZ_CLASS_ALLOCATOR(HairParentPass, SystemAllocator, 0);

            virtual ~HairParentPass();
            static RPI::Ptr<HairParentPass> Create(const RPI::PassDescriptor& descriptor);

            //! This returns pipeline view tag for children.
            const AZStd::array_view<RPI::PipelineViewTag> GetPipelineViewTags();

        private:
            HairParentPass() = delete;
            explicit HairParentPass(const RPI::PassDescriptor& descriptor);

            // RPI::Pass overrides...
            void BuildAttachmentsInternal() override;

            void UpdateChildren();

            const Name m_slotName{ "HairParentPass" };

            bool m_updateChildren = true;
        };
    } // namespace Render
} // namespace AZ
