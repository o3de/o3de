/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace UnitTest
{
    class ShapeOffsetFixture
    {
    public:
        void SetUp();
        void TearDown();

    private:
        AZStd::unique_ptr<AZ::SettingsRegistryInterface> m_settingsRegistry;
        AZ::SettingsRegistryInterface* m_oldSettingsRegistry = nullptr;
    };
} // namespace UnitTest
