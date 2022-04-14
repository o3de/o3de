/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzTest/AzTest.h>

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/containers/vector.h>


namespace UnitTest
{
    class AutomationApplicationFixture
        : public AllocatorsFixture
    {
    public:
        void SetUp() override;
        void TearDown() override;

    protected:
        AZ::ComponentApplication* CreateApplication(AZStd::vector<const char*>&& args);
        void DestroyApplication();

    private:
        AZStd::vector<const char*> m_args;

        AZ::ComponentDescriptor* m_automationComponentDescriptor = nullptr;
        AZ::ComponentApplication* m_application = nullptr;

        AZ::Entity* m_systemEntity = nullptr;
    };
} // namespace UnitTest
