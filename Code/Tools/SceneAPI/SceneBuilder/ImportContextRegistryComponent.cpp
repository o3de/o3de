/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ImportContextRegistryComponent.h"
#include "ImportContexts/AssImpImportContextProvider.h"

#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneBuilder
        {
            void ImportContextRegistryComponent::Activate()
            {
                AZ_Info("SceneAPI", "Activate SceneBuilderSystemComponent.\n");
                // Get the import context registy
                if (auto* registry = ImportContextRegistryInterface::Get())
                {
                    // Create and register the ImportContextProvider for AssImp
                    // It should be always present and be used as a fallback if no specialized provider is available
                    auto assImpContextProvider = aznew AssImpImportContextProvider();
                    registry->RegisterContextProvider(assImpContextProvider);
                    AZ_Info("SceneAPI", "AssImp Import Context was registered.\n");
                }
                else
                {
                    AZ_Error("SceneAPI", false, "ImportContextRegistryInterface not found. AssImp Import Context was not registered.");
                }
            }

            void ImportContextRegistryComponent::Deactivate()
            {
            }

            void ImportContextRegistryComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
            {
                provided.push_back(AZ_CRC_CE("ImportContextRegistryService"));
            }

            void ImportContextRegistryComponent::Reflect(ReflectContext* context)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<ImportContextRegistryComponent, SceneCore::SceneSystemComponent>()->Version(1)->Attribute(
                        Edit::Attributes::SystemComponentTags,
                        AZStd::vector<Crc32>({ Events::AssetImportRequest::GetAssetImportRequestComponentTag() }));
                }
            }
        } // namespace SceneBuilder
    } // namespace SceneAPI
} // namespace AZ
