/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Settings/SettingsRegistry.h>

namespace AZ
{
    class BehaviorContext;
}


namespace AZ::SettingsRegistryScriptUtils
{
    //! Reflects the SettingsRegistryInterface class to the BehaviorContext.
    //! Furthermore reflects a global property that wraps the AZ::SettingsRegistry Interface<T> instance to provide access
    //! to the global settings registry
    void ReflectSettingsRegistryToBehaviorContext(AZ::BehaviorContext& behaviorContext);
    void ReflectSettingsRegistry(AZ::ReflectContext* reflectContext);
}

namespace AZ::SettingsRegistryScriptUtils::Internal
{
    // SettingsRegistryImplScriptProxy is used to encapsulate a SettingsRegistry instance
    struct SettingsRegistryScriptProxy
    {
        AZ_CLASS_ALLOCATOR(SettingsRegistryScriptProxy, AZ::SystemAllocator);
        AZ_TYPE_INFO(SettingsRegistryScriptProxy, "{795C80A0-D243-473B-972A-C32CA487BAA5}");

        // NotifyEventProxy is used to forward an update to an entry in the SettingsRegistry
        // using the RegisterNotifier function to the BehaviorContext in order to create
        // a behavior method that returns an AZ::Event reference or pointer
        // This allows ScriptCanvas to create an AZ Event Handler using the information
        // reflected to the BehaviorMethod
        using ScriptNotifyEvent = AZ::Event<AZStd::string_view>;
        struct NotifyEventProxy
        {
            ScriptNotifyEvent m_scriptNotifyEvent;
            AZ::SettingsRegistryInterface::NotifyEventHandler m_settingsUpdatedHandler;
        };

        SettingsRegistryScriptProxy();
        SettingsRegistryScriptProxy(AZStd::nullptr_t);

        // Stores a SettingsRegistryInterface which will use the provided shared_ptr deleter when the reference count hits zero
        SettingsRegistryScriptProxy(AZStd::shared_ptr<AZ::SettingsRegistryInterface> settingsRegistry);
        // Stores a SettingsRegistryInterface which will perform a no-op deleter when the reference count hits zero
        SettingsRegistryScriptProxy(AZ::SettingsRegistryInterface* settingsRegistry);

        // Returns if the m_settingsRegistry shared pointer is non-nullptr
        bool IsValid() const;

        AZStd::shared_ptr<AZ::SettingsRegistryInterface> m_settingsRegistry;
        AZStd::shared_ptr<NotifyEventProxy> m_notifyEventProxy;
    };
}
