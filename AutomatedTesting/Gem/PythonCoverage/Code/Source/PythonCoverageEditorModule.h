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
