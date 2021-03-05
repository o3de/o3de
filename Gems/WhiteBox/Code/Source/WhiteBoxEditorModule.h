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

#include "WhiteBoxModule.h"

namespace WhiteBox
{
    class WhiteBoxEditorModule : public WhiteBoxModule
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL
        AZ_RTTI(EditorWhiteBoxModule, "{DAB2F46E-29A1-4898-9D4B-EB0EA41BDA32}", WhiteBoxModule);

        WhiteBoxEditorModule();
        ~WhiteBoxEditorModule();

        // WhiteBoxModule ...
        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
} // namespace WhiteBox
