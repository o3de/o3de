/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AtomToolsFramework/AtomToolsFrameworkSystemRequestBus.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Settings/SettingsRegistry.h>

namespace AtomToolsFramework
{
    class AtomToolsFrameworkSystemComponent
        : public AZ::Component
        , public AtomToolsFrameworkSystemRequestBus::Handler
    {
    public:
        AZ_COMPONENT(AtomToolsFrameworkSystemComponent, "{4E0307A7-5EF4-4E00-BFF1-9B77F2401D2E}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        // AZ::Component interface overrides...
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        // AtomToolsFrameworkSystemRequestBus::Handler overrides...
        bool IsPathIgnored(const AZStd::string& path) const override;
        bool IsPathEditable(const AZStd::string& path) const override;
        bool IsPathPreviewable(const AZStd::string& path) const override;

    private:
        void ReadSettings();
        bool m_ignoreCacheFolder{ true };
        AZStd::vector<AZStd::string> m_ignoredPathRegexPatterns;
        AZStd::unordered_map<AZStd::string, bool> m_editablePathSettings;
        AZStd::unordered_map<AZStd::string, bool> m_previewablePathSettings;
        AZStd::string m_cacheFolder;

        AZ::SettingsRegistryInterface::NotifyEventHandler m_settingsNotifyEventHandler;
    };
} // namespace AtomToolsFramework
