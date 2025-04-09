/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzTest/AzTest.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/containers/fixed_vector.h>

#include <AzFramework/Application/Application.h>


namespace UnitTest
{
    class ScriptAutomationApplicationFixture
        : public LeakDetectionFixture
    {
    public:
        void TearDown() override;

    protected:
        AzFramework::Application* CreateApplication(const char* scriptPath = nullptr, bool exitOnFinish = true);
        void DestroyApplication();

    private:
        using ArgumentContainer = AZStd::fixed_vector<const char*, 8>;

        ArgumentContainer m_args = { nullptr };
        AZStd::string m_enginePath;

        AzFramework::Application* m_application = nullptr;
    };
} // namespace UnitTest
