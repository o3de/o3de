/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Interface/Interface.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneBuilder
        {
            struct ImportContextProvider;

            // ImportContextRegistry realizes Abstract Factory Pattern.
            // It provides a family of objects related to a particular Import Context.
            // Those include ImportContext specializations for different stages of the Import pipeline
            // as well as Scene and Node wrappers.
            // To add a new library for importing scene assets:
            // - specialize and implement the ImportContextProvider
            // - register specialization with this interface
            class ImportContextRegistry
            {
            public:
                AZ_RTTI(ImportContextRegistry, "{5faaaa8a-2497-41d7-8b5c-5af4390af776}");
                AZ_CLASS_ALLOCATOR(ImportContextRegistry, AZ::SystemAllocator, 0);

                virtual ~ImportContextRegistry() = default;

                virtual void RegisterContextProvider(ImportContextProvider* provider) = 0;
                virtual void UnregisterContextProvider(ImportContextProvider* provider) = 0;
                virtual ImportContextProvider* SelectImportProvider(AZStd::string_view fileExtension) const = 0;
            };

            using ImportContextRegistryInterface = AZ::Interface<ImportContextRegistry>;
        } // namespace SceneBuilder
    } // namespace SceneAPI
} // namespace AZ
