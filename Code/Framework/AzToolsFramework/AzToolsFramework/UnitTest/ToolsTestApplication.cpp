/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UnitTest/ToolsTestApplication.h>

namespace UnitTest
{
    ToolsTestApplication::ToolsTestApplication(AZStd::string applicationName)
        : ToolsApplication()
        , m_applicationName(AZStd::move(applicationName))
    {
    }

    void ToolsTestApplication::SetSettingsRegistrySpecializations(AZ::SettingsRegistryInterface::Specializations& specializations)
    {
        Application::SetSettingsRegistrySpecializations(specializations);
        specializations.Append("test");
        specializations.Append(m_applicationName);
    }
} // namespace UnitTest
