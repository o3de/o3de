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

            //! @brief ImportContextRegistry realizes ImportContextProvider's Abstract Factory Pattern.
            //!
            //! ImportContextRegistry provides a family of objects related to the Import Context of a particular Asset Import library.
            //! Those include ImportContexts for different stages of the Import pipeline as well as Scene and Node wrappers.
            //! ImportContexts are typically aware of their provider, so they could issue additional contexts in the same family,
            //! e.g., SceneNodeAppendedContext can be used to issue corresponding SceneAttributeDataPopulatedContext...
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
