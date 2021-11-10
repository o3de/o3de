/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/DOM/DomBackendRegistry.h>

#include <AzCore/Interface/Interface.h>

namespace AZ::DOM
{
    BackendRegistry* BackendRegistry::s_instance = nullptr;

    BackendRegistryInterface* BackendRegistry::Get()
    {
        return AZ::Interface<BackendRegistryInterface>::Get();
    }

    void BackendRegistry::Create()
    {
        AZ_Assert(!s_instance, "Attempted to register BackendRegistry when it's already registered");
        s_instance = aznew BackendRegistry;
        AZ::Interface<BackendRegistryInterface>::Register(s_instance);
    }

    void BackendRegistry::Destroy()
    {
        AZ_Assert(s_instance, "Attempted to unregister a non-existent BackendRegistry");
        AZ::Interface<BackendRegistryInterface>::Unregister(s_instance);
        delete s_instance;
        s_instance = nullptr;
    }

    AZStd::unique_ptr<Backend> BackendRegistry::GetBackendByName(AZStd::string_view name)
    {
        auto backendIterator = m_nameToBackend.find(name);
        if (backendIterator != m_nameToBackend.end())
        {
            return backendIterator->second();
        }
        return nullptr;
    }

    AZStd::unique_ptr<Backend> BackendRegistry::GetBackendForExtension(AZStd::string_view extension)
    {
        auto extensionIterator = m_extensionToName.find(extension);
        if (extensionIterator != m_extensionToName.end())
        {
            return GetBackendByName(extensionIterator->second);
        }
        return nullptr;
    }

    void BackendRegistry::RegisterBackendInternal(
        BackendRegistryInterface::BackendFactory factory, AZStd::string name, AZStd::vector<AZStd::string> extensions)
    {
        AZ_Assert(m_nameToBackend.find(name) == m_nameToBackend.end(), "DOM Backend %s already registered", name.c_str());
        m_nameToBackend.insert({name, factory});
        for (AZStd::string& extension : extensions)
        {
            AZ_Assert(m_extensionToName.find(extension) == m_extensionToName.end(), "DOM Extensions already registered", extension.c_str());
            m_extensionToName.insert({AZStd::move(extension), name});
        }
    }
}
