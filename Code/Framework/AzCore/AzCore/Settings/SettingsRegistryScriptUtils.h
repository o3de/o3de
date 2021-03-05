/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

namespace AZ
{
    class BehaviorContext;
    class SettingsRegistryInterface;
}

namespace AZ::SettingsRegistryScriptUtils
{
    //! Reflects the SettingsRegistryInterface class to the BehaviorContext.
    //! Furthermore reflects a global property that wraps the AZ::SettingsRegistry Interface<T> instance to provide access
    //! to the global settings registry
    void ReflectSettingsRegistryToBehaviorContext(AZ::BehaviorContext& behaviorContext);
}

namespace AZ::SettingsRegistryScriptUtils::Internal
{
    // SettingsRegistryImplScriptProxy is used to encapsulate a SettingsRegistry instance
    struct SettingsRegistryScriptProxy
    {
        AZ_CLASS_ALLOCATOR(SettingsRegistryScriptProxy, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(SettingsRegistryScriptProxy, "{795C80A0-D243-473B-972A-C32CA487BAA5}");

        SettingsRegistryScriptProxy();
        // Stores a SettingsRegistryInterface which will use the provided shared_ptr deleter when the reference count hits zero
        SettingsRegistryScriptProxy(AZStd::shared_ptr<AZ::SettingsRegistryInterface> settingsRegistry);
        // Stores a SettingsRegistryInterface which will perform a no-op deleter when the reference count hits zero
        SettingsRegistryScriptProxy(AZ::SettingsRegistryInterface* settingsRegistry);

        // Returns if the m_settingsRegistry shared pointer is non-nullptr
        bool IsValid() const;

        AZStd::shared_ptr<AZ::SettingsRegistryInterface> m_settingsRegistry;
    };
}
