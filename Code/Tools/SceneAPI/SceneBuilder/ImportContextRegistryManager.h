/*
* Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <SceneAPI/SceneBuilder/ImportContextRegistry.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/std/containers/vector.h>
#include <SceneAPI/SceneBuilder/ImportContexts/ImportContextProvider.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneBuilder
        {
            // Implementation of the ImportContextRegistryInterface.
            class ImportContextRegistryManager : public ImportContextRegistryInterface::Registrar
            {
            public:
                AZ_RTTI(ImportContextRegistryManager, "{d3107473-4f99-4421-b4a8-ece66a922191}", ImportContextRegistry);
                AZ_CLASS_ALLOCATOR(ImportContextRegistryManager, AZ::SystemAllocator, 0);

                ImportContextRegistryManager();
                ImportContextRegistryManager(const ImportContextRegistryManager&) = delete;
                ImportContextRegistryManager& operator=(const ImportContextRegistryManager&) = delete;

                ~ImportContextRegistryManager() override = default;

                void RegisterContextProvider(ImportContextProvider* provider) override;
                void UnregisterContextProvider(ImportContextProvider* provider) override;
                ImportContextProvider* SelectImportProvider(AZStd::string_view fileExtension) const override;

                static ImportContextRegistryManager& GetInstance();
            private:
                AZStd::vector<AZStd::unique_ptr<ImportContextProvider>> m_importContextProviders;
            };
        } // namespace SceneBuilder
    } // namespace SceneAPI
} // namespace AZ
