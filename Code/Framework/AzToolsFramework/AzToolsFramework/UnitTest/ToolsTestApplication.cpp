/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UnitTest/ToolsTestApplication.h>

#include <AzCore/Console/IConsole.h>

namespace UnitTest
{
    ToolsTestApplication::ToolsTestApplication(AZStd::string applicationName)
        :ToolsTestApplication(AZStd::move(applicationName), 0, nullptr)
    {
        ;
    }

    ToolsTestApplication::ToolsTestApplication(AZStd::string applicationName, int argc, char** argv)
        : AzToolsFramework::ToolsApplication(&argc, &argv)
        , m_applicationName(AZStd::move(applicationName))
    {
        // Connection polling can be slow, disable for Tools Tests
        const auto console = AZ::Interface<AZ::IConsole>::Get();
        console->PerformCommand("target_autoconnect false");
    }

    void ToolsTestApplication::SetSettingsRegistrySpecializations(AZ::SettingsRegistryInterface::Specializations& specializations)
    {
        Application::SetSettingsRegistrySpecializations(specializations);
        specializations.Append("test");
        specializations.Append(m_applicationName);
    }
} // namespace UnitTest
