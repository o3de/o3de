/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/DocumentPropertyEditor/DocumentAdapter.h>
#include <AzFramework/DocumentPropertyEditor/AdapterBuilder.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryOriginTracker.h>

namespace AZ::DocumentPropertyEditor
{
    class SettingsRegistryAdapter : public DocumentAdapter
    {
    public:
        SettingsRegistryAdapter();
        ~SettingsRegistryAdapter();

        bool BuildField(AZStd::string_view path, AZStd::string_view fieldName, AZ::SettingsRegistryInterface::Type type);

    protected:
        Dom::Value GenerateContents() override;

    private:
        AdapterBuilder m_builder;
       /* AZ::SettingsRegistryInterface& m_settingsRegistry;
        AZ::SettingsRegistryOriginTracker& m_originTracker;
        AZStd::unique_ptr<AZ::SettingsRegistryOriginTracker> m_originTracker;*/
    };
}
