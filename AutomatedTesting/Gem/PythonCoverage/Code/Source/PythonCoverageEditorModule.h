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

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

namespace PythonCoverage
{
    class PythonCoverageEditorModule
        : public AZ::Module
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL
        AZ_RTTI(PythonCoverageEditorModule, "{32C0FFEA-09A7-460F-9257-5BDEF74FCD5B}", AZ::Module);

        PythonCoverageEditorModule();
        ~PythonCoverageEditorModule();

        // PythonCoverageModule ...
        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
} // namespace WhiteBox
