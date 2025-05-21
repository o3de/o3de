/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ImportContextRegistryManager.h"

#include <SceneAPI/SceneBuilder/ImportContexts/AssImpImportContextProvider.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneBuilder
        {
            ImportContextRegistryManager::ImportContextRegistryManager()
            {
                // register AssImp default provider
                auto assimpContextProvider = aznew AssImpImportContextProvider();
                m_importContextProviders.push_back(AZStd::unique_ptr<ImportContextProvider>(assimpContextProvider));
            };

            void ImportContextRegistryManager::RegisterContextProvider(ImportContextProvider* provider)
            {
                if (provider)
                {
                    m_importContextProviders.push_back(AZStd::unique_ptr<ImportContextProvider>(provider));
                }
            }

            void ImportContextRegistryManager::UnregisterContextProvider(ImportContextProvider* provider)
            {
                AZ_TracePrintf("SceneAPI", "Unregistered ImportContextProvider %s", provider->GetImporterName().data());
                for (auto it = m_importContextProviders.begin(); it != m_importContextProviders.end(); ++it)
                {
                    if (it->get() == provider)
                    {
                        m_importContextProviders.erase(it);
                        break; // Assuming only one instance can be registered at a time
                    }
                }
            }

            ImportContextProvider* ImportContextRegistryManager::SelectImportProvider(AZStd::string_view fileExtension) const
            {
                AZ_TracePrintf(
                    "SceneAPI",
                    "Finding ImportContextProvider (registered %d) suitable for extension: %.*s",
                    m_importContextProviders.size(),
                    static_cast<int>(fileExtension.length()),
                    fileExtension.data());
                // search in reverse order since the default AssImp Provider can handle all extenstions
                for (auto it = m_importContextProviders.rbegin(); it != m_importContextProviders.rend(); ++it)
                {
                    if (it->get()->CanHandleExtension(fileExtension))
                    {
                        return it->get();
                    }
                    else
                    {
                        AZ_TracePrintf(
                            "SceneAPI",
                            "Importer %s cannot handle %.*s",
                            it->get()->GetImporterName().data(),
                            static_cast<int>(fileExtension.length()),
                            fileExtension.data());
                    }
                }
                return nullptr; // No provider found
            }
            ImportContextRegistryManager& ImportContextRegistryManager::GetInstance()
            {
                static ImportContextRegistryManager instance;
                return instance;
            }

        } // namespace SceneBuilder
    } // namespace SceneAPI
} // namespace AZ
