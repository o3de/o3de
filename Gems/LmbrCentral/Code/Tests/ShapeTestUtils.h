/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AZ
{
    class Entity;
    class Vector3;
} // namespace AZ

namespace UnitTest
{
    class ShapeOffsetTestsBase
    {
    public:
        void SetUp();
        void TearDown();

    private:
        AZStd::unique_ptr<AZ::SettingsRegistryInterface> m_settingsRegistry;
        AZ::SettingsRegistryInterface* m_oldSettingsRegistry = nullptr;
    };

    bool IsPointInside(const AZ::Entity& entity, const AZ::Vector3& point);
} // namespace UnitTest
