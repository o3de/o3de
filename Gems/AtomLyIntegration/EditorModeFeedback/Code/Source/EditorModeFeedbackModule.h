/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

namespace AZ
{
    namespace Render
    {
        //! Module for the editor mode visual feedback gem.
        class EditorModeFeedbackModule
            : public AZ::Module
        {
        public:
            AZ_CLASS_ALLOCATOR_DECL
            AZ_RTTI(EditorModeFeedbackModule, "{9164AB79-409C-4650-922C-F10BA3A0C175}", AZ::Module);

            EditorModeFeedbackModule();

            // GetRequiredSystemComponents overrides ...
            AZ::ComponentTypeList GetRequiredSystemComponents() const override;
        };
    } // namespace Render
} // namespace AZ
