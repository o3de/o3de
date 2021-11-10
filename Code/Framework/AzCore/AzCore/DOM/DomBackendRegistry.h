/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/DOM/DomBackendRegistryInterface.h>
#include <AzCore/std/containers/unordered_map.h>

namespace AZ::DOM
{
    //! DOM backend registry implementation.
    //! \see BackendRegistryInterface
    class BackendRegistry final : public BackendRegistryInterface
    {
    public:
        AZ_RTTI(BackendRegistry, "{95533861-6201-4E45-BD03-097A20850C48}", BackendRegistryInterface);
        AZ_CLASS_ALLOCATOR(BackendRegistry, AZ::SystemAllocator, 0);

        AZStd::unique_ptr<Backend> GetBackendByName(AZStd::string_view name) override;
        AZStd::unique_ptr<Backend> GetBackendForExtension(AZStd::string_view extension) override;

        //! Convenience method, gets the current instance of the BackendRegistry via AZ::Interface.
        static BackendRegistryInterface* Get();
        //! Creates a singleton BackendRegistry and registers it via AZ::Interface.
        static void Create();
        //! Destroys the singleton BackendRegistry created by Create and unregisters it from AZ::Interface.
        static void Destroy();

    protected:
        void RegisterBackendInternal(BackendFactory factory, AZStd::string name, AZStd::vector<AZStd::string> extensions) override;

    private:
        using BackendFactory = AZStd::function<AZStd::unique_ptr<Backend>()>;
        AZStd::unordered_map<AZStd::string, BackendFactory> m_nameToBackend;
        AZStd::unordered_map<AZStd::string, AZStd::string> m_extensionToName;

        static BackendRegistry* s_instance;
    };
} // namespace AZ::DOM
