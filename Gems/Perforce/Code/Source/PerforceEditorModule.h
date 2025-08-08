/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <IGem.h>

namespace Perforce
{
    class PerforceEditorModule : public CryHooksModule
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL
        AZ_RTTI(PerforceEditorModule, "{7B6D6056-1C3C-4B0B-B7CF-B1D18956A069}", CryHooksModule);

        PerforceEditorModule();
        ~PerforceEditorModule();

        // AZ::Module ...
        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
} // namespace WhiteBox
