/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ShapeOffsetFixture.h>
#include <LmbrCentral/Shape/ShapeComponentBus.h>

namespace UnitTest
{
    void ShapeOffsetFixture::SetUp()
    {
        m_oldSettingsRegistry = AZ::SettingsRegistry::Get();
        if (m_oldSettingsRegistry)
        {
            AZ::SettingsRegistry::Unregister(m_oldSettingsRegistry);
        }

        m_settingsRegistry = AZStd::make_unique<AZ::SettingsRegistryImpl>();
        m_settingsRegistry->Set(LmbrCentral::ShapeComponentTranslationOffsetEnabled, true);
        AZ::SettingsRegistry::Register(m_settingsRegistry.get());
    }

    void ShapeOffsetFixture::TearDown()
    {
        AZ::SettingsRegistry::Unregister(m_settingsRegistry.get());
        if (m_oldSettingsRegistry)
        {
            AZ::SettingsRegistry::Register(m_oldSettingsRegistry);
            m_oldSettingsRegistry = nullptr;
        }
        m_settingsRegistry.reset();
    }
} // namespace UnitTest
