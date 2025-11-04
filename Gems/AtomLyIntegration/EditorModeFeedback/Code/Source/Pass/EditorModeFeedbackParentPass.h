/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Pass/ParentPass.h>

namespace AZ
{
    namespace Render
    {
        //! Parent pass for the editor mode feedback system.
        class EditorModeFeedbackParentPass 
        : public AZ::RPI::ParentPass
        {
        public:
            AZ_RTTI(EditorModeFeedbackParentPass, "{890482AB-192E-45B6-866C-76CB7C799CF3}", AZ::RPI::ParentPass);
            AZ_CLASS_ALLOCATOR(EditorModeFeedbackParentPass, SystemAllocator);

            virtual ~EditorModeFeedbackParentPass() = default;

            //! Creates a EditorModeFeedbackParentPass
            static RPI::Ptr<EditorModeFeedbackParentPass> Create(const RPI::PassDescriptor& descriptor);

        protected:
            EditorModeFeedbackParentPass(const RPI::PassDescriptor& descriptor);

            //! Pass behavior overrides
            bool IsEnabled() const override;

        private:
        };
    } // namespace Render
} // namespace AZ
