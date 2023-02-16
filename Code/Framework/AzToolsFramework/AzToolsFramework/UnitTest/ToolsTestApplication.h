/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>
#include <AzToolsFramework/Application/ToolsApplication.h>

namespace UnitTest
{
    class ToolsTestApplication
        : public AzToolsFramework::ToolsApplication
    {
    public:
        AZ_CLASS_ALLOCATOR(ToolsTestApplication, AZ::SystemAllocator)
        explicit ToolsTestApplication(AZStd::string applicationName);
        ToolsTestApplication(AZStd::string applicationName, int argc, char** argv);
        void SetSettingsRegistrySpecializations(AZ::SettingsRegistryInterface::Specializations& specializations) override;

    protected:
        AZStd::string m_applicationName;
    };
} // namespace UnitTest
