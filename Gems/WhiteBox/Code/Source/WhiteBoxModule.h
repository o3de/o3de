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

#include <IGem.h>

namespace WhiteBox
{
    class WhiteBoxModule : public CryHooksModule
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL
        AZ_RTTI(WhiteBoxModule, "{7B6D6056-1C3C-4B0B-B7CF-B1D18956A069}", CryHooksModule);

        WhiteBoxModule();
        ~WhiteBoxModule();

        // AZ::Module ...
        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
} // namespace WhiteBox
