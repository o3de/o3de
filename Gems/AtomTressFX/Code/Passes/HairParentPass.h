/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <AzCore/std/containers/span.h>
#include <AzCore/std/containers/fixed_vector.h>

namespace AZ
{
    namespace Render
    {
        //! HairParentPass owns the hair passes.
        //! Currently they are all defined via the pipeline configuration, making the HairParent
        //! class somewhat redundant, but moving forward, such class can be required to control
        //! passes activation and usage based on user activation options.
        class HairParentPass final
            : public RPI::ParentPass
        {
            using Base = RPI::ParentPass;
            AZ_RPI_PASS(HairParentPass);

        public:
            AZ_RTTI(HairParentPass, "80C7E869-2513-4201-8C1E-D2E39DDE1244", Base);
            AZ_CLASS_ALLOCATOR(HairParentPass, SystemAllocator);

            virtual ~HairParentPass();
            static RPI::Ptr<HairParentPass> Create(const RPI::PassDescriptor& descriptor);

        private:
            HairParentPass() = delete;
            explicit HairParentPass(const RPI::PassDescriptor& descriptor);

            // RPI::Pass overrides...
            void BuildAttachmentsInternal() override;

            void UpdateChildren();

            bool m_updateChildren = true;
        };
    } // namespace Render
} // namespace AZ
